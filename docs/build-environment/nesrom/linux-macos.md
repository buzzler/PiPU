# nesrom Linux/macOS 빌드 환경

## 대상

- `nesrom/hello_world.nes`

## 필수 도구

- cc65 툴체인: `cc65`, `ca65`, `ld65`
- `make`

## Linux (Debian/Ubuntu)

```bash
sudo apt-get update
sudo apt-get install -y cc65 make
cd /workspace/PiPU/nesrom
make clean
make
```

## macOS (Homebrew)

```bash
brew install cc65
cd /workspace/PiPU/nesrom
make clean
make
```

## 산출물

- `nesrom/hello_world.nes`
- `nesrom/hello.map` (링커 맵)

## 검증 팁

- FCEUX/Mesen 같은 NES 에뮬레이터에서 부팅 확인
