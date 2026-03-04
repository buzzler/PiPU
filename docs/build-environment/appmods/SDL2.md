# appmods/SDL2 빌드 환경 구성

## 목적

`appmods/SDL2`의 파일을 SDL2 소스 트리 `src/video/dummy`에 복사해 PiPU용 더미 비디오 경로를 사용합니다.

## 공통 준비물

- SDL2 소스 코드
- C 컴파일러(`gcc`/`clang`)
- 빌드 도구(`make` 또는 `cmake` + `ninja`)

## Linux

```bash
git clone https://github.com/libsdl-org/SDL.git
cd SDL
cp /workspace/PiPU/appmods/SDL2/SDL_null*.c src/video/dummy/
mkdir build && cd build
cmake .. -DSDL_VIDEO=ON -DSDL_RENDER=ON
cmake --build . -j
sudo cmake --install .
```

## macOS

```bash
brew install cmake ninja
git clone https://github.com/libsdl-org/SDL.git
cd SDL
cp /workspace/PiPU/appmods/SDL2/SDL_null*.c src/video/dummy/
cmake -S . -B build -G Ninja
cmake --build build
```

## Windows (MSYS2)

```bash
pacman -S --needed mingw-w64-ucrt-x86_64-toolchain mingw-w64-ucrt-x86_64-cmake ninja
git clone https://github.com/libsdl-org/SDL.git
cd SDL
cp /workspace/PiPU/appmods/SDL2/SDL_null*.c src/video/dummy/
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## 적용 확인

- SDL 앱 실행 시 `SDL_VIDEODRIVER=dummy`를 지정해 동작하는지 확인
- 예: `SDL_VIDEODRIVER=dummy chocolate-doom`
