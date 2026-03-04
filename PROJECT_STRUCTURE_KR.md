# PiPU 프로젝트 구조 가이드 (초심자용)

이 문서는 **처음 PiPU를 보는 사람**이 빠르게 전체 구조를 이해할 수 있도록 정리한 안내서입니다.

---

## 1) PiPU가 무엇인가?

PiPU는 **NES 카트리지 내부에 Raspberry Pi를 넣어**, NES가 화면을 그리는 과정(PPU 버스)을 외부 장치가 보조하도록 만든 프로젝트입니다.

- Raspberry Pi: 게임(예: DOOM) 실행, 프레임 생성
- FX2LP USB 컨트롤러: Pi ↔ NES PPU 버스 중계
- NES ROM(카트리지의 PRG): 프레임 동기화, 패드 입력 전달, 음악 재생

즉, 역할을 한 줄로 요약하면:

> **Pi가 영상 프레임을 계산하고, FX2가 그 데이터를 NES PPU 버스로 흘려보내며, NES 쪽 ROM이 동기화/입력/음악을 담당**합니다.

---

## 2) 최상위 디렉터리 구조

```text
PiPU/
├─ README.md
├─ LICENSE
├─ appmods/
│  ├─ SDL2/
│  └─ chocolatedoom/
├─ fx2firmware/
├─ music/
├─ nesrom/
├─ os/
│  └─ PiPUoverlay/
└─ ppusquirt/
```

아래에서 각 디렉터리를 역할 중심으로 설명합니다.

---

## 3) 디렉터리별 상세 설명

## 3.1 `appmods/` — 상위 소프트웨어 패치 모음

PiPU는 표준 SDL2/Chocolate Doom을 그대로 쓰지 않고, 일부 파일을 교체해 PiPU 흐름에 맞춥니다.

- `appmods/SDL2/`
  - SDL dummy video 백엔드 쪽 수정 파일이 들어 있습니다.
  - SDL 애플리케이션이 실제 디스플레이 대신 공유 메모리 기반 출력 경로를 쓰도록 맞추는 용도입니다.
- `appmods/chocolatedoom/`
  - Chocolate Doom의 사운드/트랙 연동 관련 수정 파일이 들어 있습니다.

> 핵심: **기존 앱(SD2/DOOM)을 PiPU 파이프라인에 연결하는 접착층**입니다.

---

## 3.2 `ppusquirt/` — Pi 쪽 핵심 런타임

PiPU에서 가장 중요한 사용자 공간 프로그램입니다.

주요 파일:

- `main.c`
  - 공유 메모리(`/sdlrawout`)를 만들고,
  - 그래픽 변환 스레드들을 띄우며,
  - USB 송수신 스레드(`Squirt`)를 시작합니다.
  - 더블 버퍼(`outbuf[2]`)를 사용해 프레임 전송 타이밍을 맞춥니다.
- `frameprocess.c`
  - 입력 프레임(BGRA)을 NES PPU가 읽기 좋은 타일/팔레트 형식으로 변환합니다.
  - 실질적인 “그래픽 변환 엔진” 역할입니다.
- `ppusquirt.c`
  - `libusb`로 FX2 디바이스에 벌크 전송(엔드포인트 0x02)합니다.
  - NES 패드 입력 데이터를 역방향으로 읽어(엔드포인트 0x86) Linux 입력 이벤트(uinput)로 매핑합니다.
- `palmus.c`
  - 공유 메모리(`/palmusdata`)에 음악 트랙/팔레트 정보를 설정하는 유틸리티입니다.
- `nesstuff.h`
  - PPU 프레임 구조체(`PPUFrame`, `PPUScanline`, `PPUTile`) 및 공유 메모리 구조, 패드 비트 정의 등 공통 타입 선언.
- `Makefile`
  - `ppusquirt`, `palmus` 바이너리를 빌드합니다.

> 핵심: **Pi에서 생성된 화면을 NES PPU 버스용 데이터로 변환하고 USB로 “분사(squirt)”하는 모듈**입니다.

### ppusquirt의 타일/팔레트 구조 (핵심)

- **팔레트 개수**: 한 프레임에서 **배경 팔레트 4개**를 사용합니다.
  - 각 팔레트는 `pmdata->Palettes[p][0..2]`로 정의되는 **3색**을 가집니다.
  - 여기에 공통 배경색 `BgColor`가 더해져, 실제 NES 배경 팔레트 관점에서는 **팔레트당 4색(공통 1색 + 개별 3색)** 구조입니다.
- **팔레트 데이터 구성 방식**:
  - `FitFrame()`는 `OtherData[2..17]`에 배경색과 4개 팔레트 정보를 배치해 NES ROM으로 보냅니다.
  - NES 쪽(`nesrom/hello_world.c`)은 해당 16바이트를 `$3f00`부터 써서 팔레트를 갱신합니다.
- **타일 크기(전송 포맷)**:
  - 변환 단계에서 화면을 **가로 8픽셀 × 세로 1라인(8x1)** 단위 슬라이스로 처리합니다.
  - 각 슬라이스는 2bpp NES 비트플레인(`LowBG`, `HighBG`)으로 인코딩됩니다.
  - `AT`에는 선택된 팔레트 번호(0~3)가 들어가며, 코드에서는 2비트 값을 복제해 바이트를 구성합니다.
- **색 인덱스 인코딩**:
  - 픽셀마다 선택된 팔레트 내에서 가장 가까운 색을 찾고,
  - NES 2bpp 인덱스로 변환해 Low/High 비트에 분리 저장합니다.

즉, PiPU의 프레임 변환은 “**8x1 슬라이스별 팔레트 선택 → 2bpp 비트플레인 생성 → NES 팔레트 RAM 동기화**” 순서로 동작합니다.

---

## 3.3 `fx2firmware/` — FX2LP 펌웨어

Cypress FX2LP(8051 기반)의 펌웨어 소스입니다.

주요 파일:

- `slave.c`
  - 엔드포인트/FIFO 설정(EP2, EP6 등), 인터럽트, 폴링 루프가 구현되어 있습니다.
  - EP2로 들어온 그래픽 데이터를 NES 타이밍(VBLANK 등)에 맞춰 커밋하고,
  - 컨트롤러 입력은 EP6 쪽으로 올려 보내도록 구성됩니다.
- `dscr.a51`, `isr.c`, `fw.c`, 헤더들
  - USB 디스크립터/저수준 핸들러/보조 코드.

> 핵심: **USB 패킷을 NES PPU 버스 타이밍에 맞게 흘려주는 하드웨어 브릿지의 두뇌**입니다.

---

## 3.4 `nesrom/` — 실제 NES에서 도는 PRG ROM 소스

카트리지의 NES CPU(6502) 쪽 코드입니다.

주요 파일:

- `hello_world.c`
  - NMI 동기 루프를 돌면서,
  - PPU로부터 특정 데이터 블록을 읽고(시그니처 확인),
  - 팔레트를 갱신하고,
  - 패드 입력을 PPU 경로를 통해 역전송하며,
  - 필요 시 음악 초기화 호출(`music_init`)을 수행합니다.
- `music.asm`, `sounds.s`, `famitone.s`, `nsfdriver/`
  - NES 오디오 재생 스택.
- `crt0.s`, `nes.cfg`, `Makefile`
  - cc65/ca65/ld65 기반 빌드 구성.

> 핵심: **NES 쪽 최소 런타임 + 음악 엔진 + 입력/동기 처리**입니다.

---

## 3.5 `music/` — 음악 소스/산출물

- `DOOM.ftm`: FamiTracker 프로젝트
- `DOOM.nsf`: NES Sound Format 산출물

> 핵심: **DOOM 음악을 NES PSG 형태로 재생하기 위한 자산**입니다.

---

## 3.6 `os/` — Raspberry Pi용 OS 이미지 구성

Buildroot 기반 설정과 오버레이 파일이 있습니다.

- `config.txt`
  - UART 활성화, 와이파이 비활성화, 클럭/전압 관련 설정 등 PiPU 동작에 맞춘 부팅 파라미터.
- `PiPUoverlay/opt/PiPU/`
  - 런타임 바이너리/데이터(`ppusquirt`, `palmus`, 팔레트 파일, 테스트 프레임, 스크립트 등) 배치.

> 핵심: **Pi에서 PiPU 런타임이 바로 동작하도록 만드는 배포 환경**입니다.

---

## 4) 요소 간 관계(데이터 흐름)

아래 흐름으로 이해하면 전체가 빠르게 잡힙니다.

1. **게임 실행 (Pi)**
   - (예) 패치된 SDL2를 사용하는 Chocolate Doom이 프레임을 생성.
2. **프레임 공유**
   - SDL dummy 경로를 통해 공유 메모리(`/sdlrawout`)에 raw 프레임 제공.
3. **프레임 변환 (`ppusquirt`)**
   - 변환 스레드가 BGRA 프레임을 NES PPU용 타일/팔레트 구조로 변환.
4. **USB 전송 (`ppusquirt` → FX2)**
   - `libusb` 벌크 전송으로 프레임 데이터를 FX2 EP2로 전송.
5. **버스 중계 (FX2 펌웨어)**
   - FX2가 NES의 읽기 타이밍(VBLANK 등)에 맞춰 FIFO 데이터를 커밋.
6. **NES 표시/동기 (`nesrom`)**
   - NES ROM이 프레임/팔레트 수신 보조, 음악 제어, NMI 동기 수행.
7. **입력 역방향 전달**
   - NES 패드 상태 → FX2 EP6 → Pi `ppusquirt` → uinput 키 이벤트 → 게임 입력 반영.

---

## 5) 처음 기여할 때 추천 진입 순서

1. `README.md`로 하드웨어/전체 목표 파악
2. `ppusquirt/main.c`, `ppusquirt/ppusquirt.c`, `ppusquirt/nesstuff.h` 읽기
3. `fx2firmware/slave.c`에서 EP2/EP6와 폴링 로직 확인
4. `nesrom/hello_world.c`로 NES 쪽 루프/팔레트/입력 처리 확인
5. 필요 시 `appmods/`로 SDL2/DOOM 연결 방식 확인

---

## 6) 빌드/운영 관점에서의 파일 책임 요약

- **앱 패치 계층**: `appmods/`
- **Pi 런타임 계층**: `ppusquirt/`
- **USB 브릿지 펌웨어 계층**: `fx2firmware/`
- **NES 런타임 계층**: `nesrom/`
- **콘텐츠 계층(음악)**: `music/`
- **배포/부팅 계층**: `os/`

이 6개 계층으로 보면, “어떤 문제가 어느 폴더 책임인지”를 분리해서 디버깅하기 쉽습니다.

---

## 7) 빠른 문제 분류 팁

- 화면 깨짐/색 이상: 우선 `ppusquirt/frameprocess.c` ↔ `nesrom/hello_world.c` 팔레트 경로 점검
- 입력 안 먹힘: `ppusquirt/ppusquirt.c`(uinput/EP6) + FX2 EP6 설정 점검
- 프레임 자체가 안 나감: FX2 EP2/FIFO 설정(`fx2firmware/slave.c`) 및 `ppusquirt` USB 전송 점검
- 부팅/자동 실행 문제: `os/` 오버레이와 런치 스크립트 점검
