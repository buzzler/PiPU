# ppusquirt Linux 빌드 환경

## 대상

- `ppusquirt/ppusquirt` (프레임 전송/입력 처리)
- `ppusquirt/palmus` (음악/팔레트 공유 메모리 유틸)

## 의존성 설치 (Debian/Ubuntu)

```bash
sudo apt-get update
sudo apt-get install -y build-essential libusb-1.0-0-dev
```

## 빌드

```bash
cd /workspace/PiPU/ppusquirt
make clean
make
```

## 산출물

- `ppusquirt/ppusquirt`
- `ppusquirt/palmus`

## 실행 전 체크

- USB 디바이스 접근 권한(udev rules)
- `uinput` 사용 가능 여부(`/dev/uinput`)
- 공유 메모리 경로(`/sdlrawout`, `/palmusdata`) 접근 가능 여부
