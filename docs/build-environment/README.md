# PiPU 빌드 환경 문서

이 디렉터리는 PiPU의 각 구성요소별 빌드 환경 준비 방법을 정리한 문서 모음입니다.

## 구성요소별 문서

- `appmods/SDL2.md`: SDL2 더미 비디오 백엔드 수정본 적용/빌드
- `appmods/chocolate-doom.md`: Chocolate Doom 수정본 적용/빌드
- `ppusquirt/linux.md`: Pi 런타임(`ppusquirt`, `palmus`) Linux 네이티브 빌드
- `ppusquirt/windows-wsl.md`: Windows(WSL) 기반 `ppusquirt` 빌드
- `nesrom/linux-macos.md`: NES ROM(cc65) Linux/macOS 빌드
- `nesrom/windows-msys2.md`: Windows(MSYS2) NES ROM 빌드
- `fx2firmware/windows.md`: FX2LP 펌웨어(Keil/Cypress 툴체인) Windows 빌드/배포
- `fx2firmware/linux-vm.md`: Linux에서 VM/원격 Windows를 활용한 FX2LP 빌드 전략
- `os/linux-buildroot.md`: Buildroot 기반 Pi 이미지 Linux 빌드
- `os/windows-wsl.md`: Windows(WSL2)에서 Pi 이미지 빌드
- `os/macos-vm.md`: macOS에서 Linux VM 기반 Pi 이미지 빌드
- `music/windows-ftm.md`: FamiTracker 기반 음악 자산 편집/내보내기
- `music/linux-openmpt.md`: Linux에서 대체 툴 기반 음악 자산 관리 전략

## 작성 원칙

- 가능한 경우 OS별 문서를 분리했습니다.
- 네이티브 지원이 약한 구성요소는 우회 전략(WSL/VM/원격 빌드)을 함께 제시했습니다.
- 실제 소스 트리의 현재 상태(README/Makefile)와 일치하도록 최소 명령 위주로 구성했습니다.
