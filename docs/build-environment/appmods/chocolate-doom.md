# appmods/chocolatedoom 빌드 환경 구성

## 목적

`appmods/chocolatedoom/s_sound.c`를 Chocolate Doom 소스 `src/doom`에 반영해 PiPU 음악 트랙 전환 동작을 맞춥니다.

## 공통 준비물

- Chocolate Doom 소스 코드
- SDL2 개발 헤더/라이브러리
- autotools(`autoconf`, `automake`, `libtool`) 또는 프로젝트에서 요구하는 빌드 도구

## Linux

```bash
sudo apt-get install -y build-essential libsdl2-dev automake autoconf libtool
git clone https://github.com/chocolate-doom/chocolate-doom.git
cd chocolate-doom
cp /workspace/PiPU/appmods/chocolatedoom/s_sound.c src/doom/
./autogen.sh
./configure
make -j$(nproc)
```

## macOS

```bash
brew install sdl2 automake autoconf libtool
git clone https://github.com/chocolate-doom/chocolate-doom.git
cd chocolate-doom
cp /workspace/PiPU/appmods/chocolatedoom/s_sound.c src/doom/
./autogen.sh
./configure
make -j$(sysctl -n hw.ncpu)
```

## Windows

권장 방식은 **WSL2 또는 MSYS2**입니다.

- WSL2: Linux 절차와 동일
- MSYS2: `pacman -S mingw-w64-ucrt-x86_64-{gcc,SDL2}` 설치 후 autotools 빌드

## 적용 확인

- 빌드된 `chocolate-doom` 실행 후 게임 내 음악 전환 동작 확인
- PiPU 환경에서는 `SDL_VIDEODRIVER=dummy`와 함께 실행
