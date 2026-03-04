# fx2firmware Windows 빌드/배포 환경

FX2LP 펌웨어는 Cypress/Keil 기반 워크플로가 일반적이므로 Windows 환경을 기본으로 합니다.

## 필수 도구

- Cypress FX2LP Development Kit (Control Center 포함)
- Keil uVision C51 (프로젝트 파일: `slave.uvproj`)

## 빌드 절차

1. `fx2firmware/slave.uvproj`를 uVision으로 열기
2. 타깃 설정 확인 후 Build 실행
3. 출력된 펌웨어 산출물(`.hex`/`.iic`) 확인

## 디바이스 다운로드 절차(요약)

1. FX2 보드를 EEPROM 비활성 상태로 연결
2. Control Center에서 `Vend_Ax.hex`를 RAM 다운로드
3. EEPROM 활성화 상태로 전환
4. `slave.iic`를 EEPROM 다운로드
5. 보드 리셋 후 열거(Enumeration) 확인

## 드라이버/호환성

- 최신 Windows에서는 드라이버 서명 이슈가 있을 수 있으므로 Cypress 제공 드라이버 경로를 사용
- 보드별 EEPROM 점퍼 극성이 다를 수 있어 실제 보드 문서 확인 필요
