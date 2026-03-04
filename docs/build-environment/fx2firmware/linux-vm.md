# fx2firmware Linux 환경 가이드 (VM/원격 Windows 권장)

FX2LP 펌웨어는 프로젝트 파일이 Keil uVision(`.uvproj`) 중심이라 Linux 네이티브 빌드는 현실적으로 제약이 큽니다.

## 권장 전략

## A. Linux + Windows VM

- Linux 호스트에 VirtualBox/VMware 설치
- Windows VM에서 `fx2firmware/windows.md` 절차 수행
- 산출물(`.iic`)만 Linux 호스트로 전달

## B. Linux + 원격 Windows 빌드 머신

- Git 공유 후 원격 Windows에서 빌드/프로그램
- USB 디바이스 프로그래밍은 Windows 물리 장치에 직접 연결

## C. 완전 대체 툴체인 검토 (고급)

- SDCC + fx2lib 기반으로 빌드 시스템 이식 가능
- 단, 현재 저장소의 프로젝트/소스 구조는 Keil 기준이므로 별도 포팅 작업 필요

## 결론

- 유지보수 비용 관점에서, FX2 펌웨어는 Windows 빌드 환경을 공식 기준으로 두는 것이 가장 안전합니다.
