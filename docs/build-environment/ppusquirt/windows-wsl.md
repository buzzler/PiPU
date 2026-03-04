# ppusquirt Windows(WSL2) 빌드 환경

`ppusquirt`는 Linux API(`uinput`, POSIX shm, libusb)에 의존하므로 Windows 네이티브보다 **WSL2 빌드**를 권장합니다.

## 1) WSL2 준비

- Windows 기능에서 WSL/가상화 활성화
- Ubuntu 설치 후 터미널 진입

## 2) 패키지 설치

```bash
sudo apt-get update
sudo apt-get install -y build-essential libusb-1.0-0-dev
```

## 3) 빌드

```bash
cd /mnt/c/path/to/PiPU/ppusquirt
make clean
make
```

## 4) 주의사항

- WSL 내부에서는 실제 USB 패스스루가 제한될 수 있음
- 실제 실행/검증은 Raspberry Pi Linux 환경에서 재검증 필요
