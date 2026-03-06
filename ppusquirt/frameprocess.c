#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>
#include <libusb-1.0/libusb.h>
#include "nesstuff.h"

// Bgcolor is always black
int BgColor = 0x0f;

// This is generated in GFXSetup
Color NesPalette[NESCOLORCOUNT];

struct palmusdata *pmdata;
double contrastFactor, contrast = 50;
#define BRIGHTNESS 30
Colmatch MostCommonColorInFrame[NESCOLORCOUNT];

// 16-bit lookup tables to keep runtime conversion fast while supporting dynamic palette updates.
// RGB565 keeps memory footprint manageable while allowing very quick palette refresh.
signed char PaletteLookup565[65536];       // for matching a color to a palette
signed char ColorLookup565[65536][4];      // for matching to a color within each palette
unsigned short ErrorLookup565[65536][4];   // quantized color error for each palette

unsigned char ActivePalettes[4][3];
unsigned char ActiveBgColor = 0x0f;
unsigned int FrameCounter = 0;
unsigned int LastPaletteUpdateFrame = 0;

#define PALETTE_UPDATE_INTERVAL 8
#define PALETTE_UPDATE_HYSTERESIS 0.92

unsigned char SatAdd8(signed short n1, signed short n2);
signed char FindBestPalForPixel(Color currPix);

// Blue-noise-like dither map (0..63), less structured artifacts than classic Bayer.
unsigned char map[8][8] = {
	{0, 40, 12, 52, 3, 43, 15, 55},
	{32, 24, 44, 20, 35, 27, 47, 23},
	{8, 56, 4, 60, 11, 59, 7, 63},
	{36, 16, 48, 28, 39, 19, 51, 31},
	{2, 42, 14, 54, 1, 41, 13, 53},
	{34, 26, 46, 22, 33, 25, 45, 21},
	{10, 58, 6, 62, 9, 57, 5, 61},
	{38, 18, 50, 30, 37, 17, 49, 29}};

// Nes Palette in string format, will be converted later on
char NesPaletteStrings[NESCOLORCOUNT][6] =
	{
		"7C7C7C",
		"0000FC",
		"0000BC",
		"4428BC",
		"940084",
		"A80020",
		"A81000",
		"881400",
		"503000",
		"007800",
		"006800",
		"005800",
		"004058",
		"000000",
		"000000",
		"000000",
		"BCBCBC",
		"0078F8",
		"0058F8",
		"6844FC",
		"D800CC",
		"E40058",
		"F83800",
		"E45C10",
		"AC7C00",
		"00B800",
		"00A800",
		"00A844",
		"008888",
		"000000",
		"000000",
		"000000",
		"F8F8F8",
		"3CBCFC",
		"6888FC",
		"9878F8",
		"F878F8",
		"F85898",
		"F87858",
		"FCA044",
		"F8B800",
		"B8F818",
		"58D854",
		"58F898",
		"00E8D8",
		"787878",
		"000000",
		"000000",
		"FCFCFC",
		"A4E4FC",
		"B8B8F8",
		"D8B8F8",
		"F8B8F8",
		"F8A4C0",
		"F0D0B0",
		"FCE0A8",
		"F8D878",
		"D8F878",
		"B8F8B8",
		"B8F8D8",
		"00FCFC",
		"F8D8F8",
		"000000",
		"000000"};
		

// Square a number
float S(float i)
{
	return i * i;
}

void getpixel(char *frameBuf, unsigned int x, unsigned int y, unsigned char *r, unsigned char *g, unsigned char *b)
{
	unsigned int flx = 0;
	flx = (unsigned int)((float)1.25 * (float)x);

	unsigned int off = ((320 * y) + flx) * 4;
	*b = frameBuf[off];
	*g = frameBuf[off + 1];
	*r = frameBuf[off + 2];
}

void setpixel(char *frameBuf, unsigned int x, unsigned int y, unsigned char r, unsigned char g, unsigned char b)
{
	unsigned int flx = 0;
	flx = (unsigned int)((float)1.25 * (float)x);
	unsigned int off = ((320 * y) + flx) * 4;
	*(frameBuf + off) = b;
	*(frameBuf + off + 1) = g;
	*(frameBuf + off + 2) = r;
}

// Compare the difference of two RGB values, weigh by CCIR 601 luminosity
float WeightedColorDistance(Color color1, Color color2)
{
	float luma1 = (color1.r * 299 + color1.g * 587 + color1.b * 114) / (255.0 * 1000);
	float luma2 = (color2.r * 299 + color2.g * 587 + color2.b * 114) / (255.0 * 1000);
	float lumadiff = luma1 - luma2;
	float diffR = (color1.r - color2.r) / 255.0, diffG = (color1.g - color2.g) / 255.0, diffB = (color1.b - color2.b) / 255.0;

	return (diffR * diffR * 0.299 + diffG * diffG * 0.587 + diffB * diffB * 0.114) * 0.75 + lumadiff * lumadiff;
}

// Find the best colour match from the entire NES palette, not really used
unsigned char FindBestColorMatch(Color theColor)
{
	double distance;
	double bestScoreSoFar = 99999;
	unsigned char bestColorSoFar = 0;
	for (int cnt = 0; cnt < NESCOLORCOUNT; cnt++)
	{
		distance = WeightedColorDistance(NesPalette[cnt], theColor);
		if (distance < bestScoreSoFar)
		{
			bestColorSoFar = cnt;
			bestScoreSoFar = distance;
		}
	}
	return bestColorSoFar;
}

// Find the best colour match from a specified palette
// Will return -1 if bgcolor is best match
int FindBestColorMatchFromPalette(Color theColor, unsigned char *Palette, int bgColor)
{
	float distance;
	float bestScoreSoFar = 999999999;
	int bestColorSoFar = 9999;
	int cnt = 0;

	bestColorSoFar = -1;
	bestScoreSoFar = WeightedColorDistance(NesPalette[bgColor], theColor);

	if (bestScoreSoFar != 0)
	{
		for (cnt = 0; cnt < 3; cnt++)
		{
			distance = WeightedColorDistance(NesPalette[Palette[cnt]], theColor);
			if (distance < bestScoreSoFar)
			{
				bestColorSoFar = cnt;
				bestScoreSoFar = distance;
			}
		}
	}
	return bestColorSoFar;
}

static inline unsigned short RGBTo565(unsigned char r, unsigned char g, unsigned char b)
{
	return (unsigned short)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

static inline Color RGB565To888(unsigned short rgb565)
{
	Color c;
	c.r = (unsigned char)((rgb565 >> 8) & 0xF8);
	c.g = (unsigned char)((rgb565 >> 3) & 0xFC);
	c.b = (unsigned char)((rgb565 << 3) & 0xF8);
	return c;
}

unsigned short QuantizeError(float e)
{
	if (e <= 0)
		return 0;
	if (e >= 1.0f)
		return 65535;
	return (unsigned short)(e * 65535.0f);
}

float PaletteError(Color px, unsigned char paletteNo, unsigned char palettes[4][3], unsigned char bg)
{
	float bestScore = WeightedColorDistance(NesPalette[bg], px);

	for (int col = 0; col < 3; col++)
	{
		float dist = WeightedColorDistance(NesPalette[palettes[paletteNo][col]], px);
		if (dist < bestScore)
		{
			bestScore = dist;
		}
	}

	return bestScore;
}

void CopyActivePaletteToSharedMemory(void)
{
	BgColor = ActiveBgColor;
	for (int p = 0; p < 4; p++)
	{
		for (int c = 0; c < 3; c++)
		{
			pmdata->Palettes[p][c] = ActivePalettes[p][c];
		}
	}
}

void RefreshLookupTables(void)
{
	for (int i = 0; i < 65536; i++)
	{
		Color col = RGB565To888((unsigned short)i);
		PaletteLookup565[i] = FindBestPalForPixel(col);

		for (int p = 0; p < 4; p++)
		{
			ColorLookup565[i][p] = FindBestColorMatchFromPalette(col, ActivePalettes[p], ActiveBgColor);
			ErrorLookup565[i][p] = QuantizeError(PaletteError(col, (unsigned char)p, ActivePalettes, ActiveBgColor));
		}
	}
}

float EstimateFrameError(char *bmp, unsigned char palettes[4][3], unsigned char bg)
{
	float total = 0.0f;
	Color px;
	for (int y = 0; y < 240; y += 2)
	{
		for (int x = 0; x < 256; x += 2)
		{
			getpixel(bmp, x, y, &px.r, &px.g, &px.b);
			px.r = SatAdd8(px.r, BRIGHTNESS);
			px.g = SatAdd8(px.g, BRIGHTNESS);
			px.b = SatAdd8(px.b, BRIGHTNESS);
			px.r = SatAdd8(contrastFactor * ((double)px.r - 128), 128);
			px.g = SatAdd8(contrastFactor * ((double)px.g - 128), 128);
			px.b = SatAdd8(contrastFactor * ((double)px.b - 128), 128);

			float best = PaletteError(px, 0, palettes, bg);
			for (int p = 1; p < 4; p++)
			{
				float err = PaletteError(px, (unsigned char)p, palettes, bg);
				if (err < best)
					best = err;
			}
			total += best;
		}
	}
	return total;
}

void BuildCandidatePalette(char *bmp, unsigned char candidate[4][3], unsigned char *candidateBg)
{
	int freq[NESCOLORCOUNT] = {0};
	Color px;

	for (int y = 0; y < 240; y += 2)
	{
		for (int x = 0; x < 256; x += 2)
		{
			getpixel(bmp, x, y, &px.r, &px.g, &px.b);
			px.r = SatAdd8(px.r, BRIGHTNESS);
			px.g = SatAdd8(px.g, BRIGHTNESS);
			px.b = SatAdd8(px.b, BRIGHTNESS);
			px.r = SatAdd8(contrastFactor * ((double)px.r - 128), 128);
			px.g = SatAdd8(contrastFactor * ((double)px.g - 128), 128);
			px.b = SatAdd8(contrastFactor * ((double)px.b - 128), 128);
			freq[FindBestColorMatch(px)]++;
		}
	}

	int bestColor = ActiveBgColor;
	for (int i = 0; i < NESCOLORCOUNT; i++)
	{
		if (freq[i] > freq[bestColor])
		{
			bestColor = i;
		}
	}

	if (freq[bestColor] > (int)(freq[ActiveBgColor] * 1.15f))
		*candidateBg = (unsigned char)bestColor;
	else
		*candidateBg = ActiveBgColor;

	int picks[12];
	int pickCount = 0;
	for (int p = 0; p < 12; p++)
		picks[p] = -1;

	while (pickCount < 12)
	{
		int best = -1;
		for (int i = 0; i < NESCOLORCOUNT; i++)
		{
			if (i == *candidateBg)
				continue;
			int already = 0;
			for (int j = 0; j < pickCount; j++)
			{
				if (picks[j] == i)
				{
					already = 1;
					break;
				}
			}
			if (already)
				continue;

			if (best == -1 || freq[i] > freq[best])
				best = i;
		}

		if (best == -1)
			break;

		picks[pickCount++] = best;
	}

	for (int p = 0; p < 4; p++)
	{
		for (int c = 0; c < 3; c++)
		{
			int idx = p * 3 + c;
			if (idx < pickCount)
				candidate[p][c] = (unsigned char)picks[idx];
			else
				candidate[p][c] = ActivePalettes[p][c];
		}
	}
}

void MaybeRefreshDynamicPalette(char *bmp)
{
	if (FrameCounter - LastPaletteUpdateFrame < PALETTE_UPDATE_INTERVAL)
		return;

	unsigned char candidate[4][3];
	unsigned char candidateBg = ActiveBgColor;
	BuildCandidatePalette(bmp, candidate, &candidateBg);

	float currentErr = EstimateFrameError(bmp, ActivePalettes, ActiveBgColor);
	float candidateErr = EstimateFrameError(bmp, candidate, candidateBg);

	if (candidateErr < currentErr * PALETTE_UPDATE_HYSTERESIS)
	{
		for (int p = 0; p < 4; p++)
			for (int c = 0; c < 3; c++)
				ActivePalettes[p][c] = candidate[p][c];
		ActiveBgColor = candidateBg;
		CopyActivePaletteToSharedMemory();
		RefreshLookupTables();
	}

	LastPaletteUpdateFrame = FrameCounter;
}

signed char FindBestPalForPixel(Color currPix)
{
	int p;
	float bestDist;
	signed char bestPal;
	int col;

	bestDist = 999999999;
	bestPal = -1;

	// check the bgcolor first
	bestDist = WeightedColorDistance(NesPalette[ActiveBgColor], currPix);

	double dist;

	// if bgcolor wasn't an exact match, match other cols
	if (bestDist != 0)
	{
		for (p = 0; p < 4; p++)
		{
			for (col = 0; col < 3; col++)
			{
					dist = WeightedColorDistance(NesPalette[ActivePalettes[p][col]], currPix);
				if (dist < bestDist)
				{
					bestDist = dist;
					bestPal = p;
				}
			}
		}
	}
	return bestPal;
}

int FindBestPalForSlice(char *bmp, unsigned int xoff, unsigned int yoff)
{
	unsigned int x;
	unsigned int bestPal = 0;
	unsigned int bestError = 0xffffffff;
	Color currPix;

	for (int p = 0; p < 4; p++)
	{
		unsigned int total = 0;
		for (x = xoff; x < xoff + 8; x++)
		{
			getpixel(bmp, x, yoff, &currPix.r, &currPix.g, &currPix.b);
			total += ErrorLookup565[RGBTo565(currPix.r, currPix.g, currPix.b)][p];
		}

		if (total < bestError)
		{
			bestError = total;
			bestPal = (unsigned int)p;
		}
	}

	return (int)bestPal;
}

long CompareColMatch(const void *s1, const void *s2)
{
	Colmatch *e1 = (Colmatch *)s1;
	Colmatch *e2 = (Colmatch *)s2;
	return e1->frequency - e2->frequency;
}

unsigned char SatAdd8(signed short n1, signed short n2)
{
	signed int result = n1 + n2;
	if (result < 0x00)
		return 0;
	if (result > 0xFF)
		return 0xff;
	else
		return (unsigned char)result;
}

// Initialise palettes, set up memory regions, etc
void GFXSetup()
{

	int i;

	char curHex[3] = {0x00, 0x00, 0x00};
	char *curCol;
	char *dummy = 0;

	// Generate NES palette from the string info
	for (i = 0; i < 64; i++)
	{
		curCol = NesPaletteStrings[i];

		curHex[0] = curCol[0];
		curHex[1] = curCol[1];
		NesPalette[i].r = (unsigned char)strtoul(curHex, &dummy, 16);

		curHex[0] = curCol[2];
		curHex[1] = curCol[3];
		NesPalette[i].g = (unsigned char)strtoul(curHex, &dummy, 16);

		curHex[0] = curCol[4];
		curHex[1] = curCol[5];
		NesPalette[i].b = (unsigned char)strtoul(curHex, &dummy, 16);
	}

	// Open palette/music shared memory area
	int fd;
	fd = shm_open("/palmusdata", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
	if (fd == -1)
		exit(1);

	if (ftruncate(fd, sizeof(struct palmusdata)) == -1)
		exit(1);

	pmdata = mmap(NULL, sizeof(struct palmusdata), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (pmdata == MAP_FAILED)
		exit(1);


	// Default DOOM Palette

	//black for bg
	ActiveBgColor = 0x0f;

	//dark greys + green
	ActivePalettes[0][0] = 0x10;
	ActivePalettes[0][1] = 0x09;
	ActivePalettes[0][2] = 0x2d;

	//oranges
	ActivePalettes[1][0] = 0x07;
	ActivePalettes[1][1] = 0x28;
	ActivePalettes[1][2] = 0x18;

	//blues
	ActivePalettes[2][0] = 0x02;
	ActivePalettes[2][1] = 0x01;
	ActivePalettes[2][2] = 0x11;

	//reds and light grey
	ActivePalettes[3][0] = 0x06;
	ActivePalettes[3][1] = 0x16;
	ActivePalettes[3][2] = 0x3d;

	CopyActivePaletteToSharedMemory();

	// Pre-gen contrast factor
	contrastFactor = (259 * (contrast + 255)) / (255 * (259 - contrast));

	RefreshLookupTables();
}

// Convert a single frame segment to NES format
void FitFrame(char *bmp, PPUFrame *theFrame, int startline, int endline)
{
	int x, y, i, offset;
	Color currPix;
	int xoff;
	int palToUse;
	short bestcol;


	if (startline == 0)
	{
		FrameCounter++;
		MaybeRefreshDynamicPalette(bmp);

		theFrame->OtherData[0] = 0x54;
		theFrame->OtherData[1] = 0x17; // Magic value
		theFrame->OtherData[2] = ActiveBgColor;
		theFrame->OtherData[3] = ActivePalettes[0][0];
		theFrame->OtherData[4] = ActivePalettes[0][1];
		theFrame->OtherData[5] = ActivePalettes[0][2];
		theFrame->OtherData[6] = 0;
		theFrame->OtherData[7] = ActivePalettes[1][0];
		theFrame->OtherData[8] = ActivePalettes[1][1];
		theFrame->OtherData[9] = ActivePalettes[1][2];
		theFrame->OtherData[10] = 0;
		theFrame->OtherData[11] = ActivePalettes[2][0];
		theFrame->OtherData[12] = ActivePalettes[2][1];
		theFrame->OtherData[13] = ActivePalettes[2][2];
		theFrame->OtherData[14] = 0;
		theFrame->OtherData[15] = ActivePalettes[3][0];
		theFrame->OtherData[16] = ActivePalettes[3][1];
		theFrame->OtherData[17] = ActivePalettes[3][2];
		theFrame->OtherData[20] = pmdata->music; // Play E1M1 music
		theFrame->OtherData[30] = 0xBE;
		theFrame->OtherData[31] = 0xEF; // Magic Value
	}

	// First apply dithering and brightness/contrast adjustment
	for (y = startline; y < endline; y++)
	{
		unsigned char ditherStrength[32];
		for (i = 0; i < 32; i++)
		{
			unsigned char minLum = 255;
			unsigned char maxLum = 0;
			for (x = i * 8; x < (i * 8) + 8; x++)
			{
				getpixel(bmp, x, y, &currPix.r, &currPix.g, &currPix.b);
				unsigned char lum = (unsigned char)((currPix.r * 3 + currPix.g * 6 + currPix.b) / 10);
				if (lum < minLum)
					minLum = lum;
				if (lum > maxLum)
					maxLum = lum;
			}

			unsigned char range = maxLum - minLum;
			if (range < 18)
				ditherStrength[i] = 8;
			else if (range < 48)
				ditherStrength[i] = 18;
			else
				ditherStrength[i] = 30;
		}

		for (x = 0; x < 256; x++)
		{
			getpixel(bmp, x, y, &currPix.r, &currPix.g, &currPix.b);

			//bump up brightness
			currPix.r = SatAdd8(currPix.r, BRIGHTNESS);
			currPix.g = SatAdd8(currPix.g, BRIGHTNESS);
			currPix.b = SatAdd8(currPix.b, BRIGHTNESS);

			//bump up contrast
			currPix.r = SatAdd8(contrastFactor * ((double)currPix.r - 128), 128);
			currPix.g = SatAdd8(contrastFactor * ((double)currPix.g - 128), 128);
			currPix.b = SatAdd8(contrastFactor * ((double)currPix.b - 128), 128);

			// Adaptive ordered dither with blue-noise-like matrix.
			int dither = ((int)map[x % 8][y % 8] - 32) * ditherStrength[x / 8] / 32;
			currPix.r = SatAdd8(currPix.r, dither);
			currPix.g = SatAdd8(currPix.g, dither);
			currPix.b = SatAdd8(currPix.b, dither);
			setpixel(bmp, x, y, currPix.r, currPix.g, currPix.b);
		}
	}

	// Iterate through all the image's pixels 
	// for each scanline
	for (y = startline; y < endline; y++)
	{ 
		int currScanLine = y + 1;

		// For each 8x1 attribute slice
		for (i = 0; i < 32; i++)
		{ 

			//xoff is actual x coord, i is slice no in scanline
			xoff = i * 8;

			// Find the most suitable palette for this slice
			palToUse = FindBestPalForSlice(bmp, xoff, y);

			// Start building the 8x1 tile
			PPUTile currTile;
			currTile.NT = 0x00;																// Nametable is irrelevant
			currTile.AT = (palToUse) | (palToUse << 2) | (palToUse << 4) | (palToUse << 6); // Attribute table can just have 4 copies of the palette number to save calculating which quadrant we're in
			currTile.LowBG = 0;
			currTile.HighBG = 0;

			offset = 7;

			// So now we fit the pixels to the palette we chose
			// For each pixel in the slice
			for (x = xoff; x < xoff + 8; x++)
			{ 
				getpixel(bmp, x, y, &currPix.r, &currPix.g, &currPix.b);

				// find closest color match from palette we chose
				bestcol = ColorLookup565[RGBTo565(currPix.r, currPix.g, currPix.b)][palToUse]; // Quick and palette-refresh friendly

				// Shift the index up one, so -1 becomes zero, which is how it will appear in the nes palette
				bestcol++;

				// Bit 0 of bestcol goes to the low BG tile byte
				currTile.LowBG = currTile.LowBG | ((bestcol & 0x01) << offset);

				// Bit 1 of bestcol goes to the high BG tile byte
				currTile.HighBG = currTile.HighBG | (((bestcol >> 1) & 0x01) << offset);

				offset--;
			}

			// Store the first two tiles in the previous scanline
			if (i < 2)
			{
				if (currScanLine > 0)
					theFrame->ScanLines[currScanLine - 1].nexttiles[i] = currTile;
			}
			else
			{
				theFrame->ScanLines[currScanLine].tiles[i - 2] = currTile;
			}

		}
	}
}
