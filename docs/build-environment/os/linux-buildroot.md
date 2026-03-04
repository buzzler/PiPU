# os Linux(Buildroot) 빌드 환경

`os/`는 Raspberry Pi용 커스텀 이미지 생성을 위한 Buildroot 설정/오버레이를 담고 있습니다.

## 준비물

- Linux x86_64 호스트
- Buildroot 소스 (저장소 외부에서 준비)
- 필수 패키지: gcc, make, bison, flex, rsync, bc, cpio, unzip 등

## 기본 절차

1. Buildroot 소스 준비
2. PiPU 설정을 `.config`에 반영
3. `os/PiPUoverlay`를 rootfs overlay로 지정
4. `make` 실행해 SD 카드 이미지 생성

## 예시 흐름

```bash
# 예시: Buildroot 작업 디렉터리에서
make menuconfig
# System configuration -> Root filesystem overlay directories 에
# /workspace/PiPU/os/PiPUoverlay 지정

make -j$(nproc)
```

## 배포

- 생성된 `sdcard.img` 또는 `boot.vfat`/`rootfs.ext*`를 SD 카드에 기록
- 필요 시 `DOOM1.WAD`, `DOOM2.WAD`를 FAT 파티션 루트에 복사

## 체크 포인트

- `config.txt`의 UART/클럭 설정이 하드웨어와 맞는지 확인
- 오버레이 내부 바이너리(`ppusquirt`, `palmus`)가 타깃 아키텍처(ARM)로 빌드됐는지 확인
