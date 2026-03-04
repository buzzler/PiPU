# os Windows(WSL2) Buildroot 빌드 환경

Windows에서는 WSL2(Ubuntu) 위에서 Buildroot를 돌리는 방식을 권장합니다.

## 1) WSL2 준비

- Ubuntu 설치 후 개발 패키지 설치

```bash
sudo apt-get update
sudo apt-get install -y build-essential git rsync bc bison flex cpio unzip file wget
```

## 2) Buildroot 작업

- Buildroot 소스를 WSL 파일시스템(예: `~/buildroot`)에 위치
- overlay 경로로 `/workspace/PiPU/os/PiPUoverlay` 지정

## 3) 이미지 생성

```bash
cd ~/buildroot
make menuconfig
make -j$(nproc)
```

## 4) SD 기록

- 생성 이미지를 Windows로 복사해 balenaEtcher 사용 또는
- WSL에서 `dd` 사용(장치 선택 주의)

## 주의사항

- `/mnt/c` 경로에서 직접 빌드하면 I/O 성능이 낮을 수 있음
- USB SD 카드 리더 직접 접근은 Windows 도구가 더 안정적인 경우가 많음
