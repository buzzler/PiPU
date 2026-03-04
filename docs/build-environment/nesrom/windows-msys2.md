# nesrom Windows(MSYS2) 빌드 환경

## 1) MSYS2 설치

- https://www.msys2.org/ 에서 설치
- `MSYS2 UCRT64` 셸 사용

## 2) 도구 설치

```bash
pacman -Syu
pacman -S --needed make cc65
```

## 3) 빌드

```bash
cd /c/path/to/PiPU/nesrom
make clean
make
```

## 4) 산출물

- `hello_world.nes`

## 참고

- PowerShell/CMD 단독 환경보다 MSYS2가 Makefile 호환성이 높습니다.
