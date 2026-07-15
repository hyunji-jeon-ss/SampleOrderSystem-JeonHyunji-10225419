# SampleOrderSystem-JeonHyunji-10225419

반도체 시료 생산주문관리 시스템 개인과제 — **[미션2] 프로젝트 개발**

## 목적
S-Semi 사의 반도체 시료 생산주문관리 시스템. 시료 등록, 주문 접수/승인/거절, 생산라인 처리, 출고, 모니터링을 콘솔 환경에서 처리한다.

## 기술 스택
- C++20, Visual Studio (MSBuild, .vcxproj)
- nlohmann/json (NuGet, `nlohmann.json`) — JSON 파일 영속성
- gmock (NuGet, v1.11.0) 기반 단위 테스트

## 구조 (Phase 5 — 기반 골격)
```
SampleOrderSystem.sln
SampleOrderSystemLib/
  model/           Sample, Order, OrderStatus
  clock/           IClock, SystemClock (시간 의존성 분리 — 생산라인/주문번호 채번에서 재사용)
  repository/      ISampleRepository/JsonSampleRepository, IOrderRepository/JsonOrderRepository
  view/            IMainView, ConsoleMainView
  controller/      IInputReader/ConsoleInputReader, MainController
SampleOrderSystemApp/    # 콘솔 실행 파일 (main.cpp), Lib 참조
SampleOrderSystemTest/   # gmock 테스트, Lib 참조
```
- 시료 ID는 `S-001` 형식, 주문번호는 `ORD-YYYYMMDD-NNNN` 형식으로 Repository가 자동 채번한다 (주문번호는 `IClock`을 주입받아 날짜를 결정하므로 테스트에서 시간을 고정/목킹 가능).
- 현재 메인 메뉴는 표시·종료만 동작하며, 1~6번 메뉴는 이후 Phase(6~11)에서 실제 기능으로 채워진다.

## 빌드 방법 (Visual Studio)
1. `SampleOrderSystem.sln`을 Visual Studio로 연다.
2. NuGet이 자동으로 `nlohmann.json`(3.12.0), `gmock`(1.11.0)을 복원한다 (안 되면 솔루션 우클릭 → NuGet 패키지 복원).
3. 구성을 **Debug / x64**로 맞추고 `Ctrl+Shift+B` 빌드.

## 실행 방법
- **앱**: `SampleOrderSystemApp`을 시작 프로젝트로 설정 후 `Ctrl+F5` → 메인 메뉴 확인, `0`으로 종료
- **테스트**: `SampleOrderSystemTest`를 시작 프로젝트로 설정 후 `Ctrl+F5` 실행

## 문서
- `PRD.md`: 요구사항 정의
- `PLAN.md`: Phase별 구현 계획
- `CLAUDE.md`: 개발 가이드
