# os macOS 빌드 환경 (Linux VM 권장)

Buildroot는 macOS 네이티브에서도 가능하지만, 패키지/케이스 민감성 이슈로 Linux VM을 권장합니다.

## 권장 구성

- UTM/Parallels/VMware Fusion에 Ubuntu VM 설치
- VM 안에서 `os/linux-buildroot.md` 절차 수행

## macOS 네이티브 시도 시

- Xcode Command Line Tools 설치
- Homebrew로 GNU 유틸 보완 설치 필요 가능
- 일부 패키지 버전 차이로 Buildroot 빌드 실패 가능성 존재

## 결론

- 재현 가능한 빌드가 중요하다면 macOS에서는 Linux VM 기반 워크플로를 표준으로 삼는 것이 안전합니다.
