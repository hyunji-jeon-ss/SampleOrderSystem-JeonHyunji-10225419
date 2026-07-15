# SampleOrderSystem-JeonHyunji-10225419

반도체 시료 생산주문관리 시스템 개인과제 — **[미션2] 프로젝트 개발**

## 목적
S-Semi 사의 반도체 시료 생산주문관리 시스템. 시료 등록, 주문 접수/승인/거절, 생산라인 처리, 출고, 모니터링을 콘솔 환경에서 처리한다.

## 기술 스택
- C++20, Visual Studio (MSBuild, .vcxproj)
- nlohmann/json (NuGet, `nlohmann.json`) — JSON 파일 영속성
- gmock (NuGet, v1.11.0) 기반 단위 테스트

## 구조 (Phase 11까지 반영)
```
SampleOrderSystem.sln
SampleOrderSystemLib/
  model/           Sample, Order, OrderStatus, OrderStatusText(상태↔문자열 공용 변환)
  clock/           IClock, SystemClock, TimeFormat (날짜/시각 포맷 공용 유틸)
  console/         ConsoleUtil (화면 클리어, padEnd/padStart 정렬, renderProgressBar 진행률 바 유틸)
  production/      ProductionCalculator(실생산량/예상시간 계산), ProductionQueueProcessor(FIFO 생산 큐 폴링 처리)
  repository/      JsonFileStore(파일 I/O 공용화), ISampleRepository/JsonSampleRepository, IOrderRepository/JsonOrderRepository
  view/            IMainView/ConsoleMainView, ISampleView/ConsoleSampleView,
                   IOrderView/ConsoleOrderView, IApprovalView/ConsoleApprovalView,
                   IProductionView/ConsoleProductionView, IReleaseView/ConsoleReleaseView,
                   IMonitoringView/ConsoleMonitoringView
  controller/      IInputReader/ConsoleInputReader, ISubMenuController(서브메뉴 위임 인터페이스),
                   MainController, SampleController, OrderController, ApprovalController,
                   ProductionController, ReleaseController, MonitoringController
SampleOrderSystemApp/    # 콘솔 실행 파일 (main.cpp), Lib 참조
SampleOrderSystemTest/   # gmock 테스트, Lib 참조
```
- 시료 ID는 `S-001` 형식, 주문번호는 `ORD-YYYYMMDD-NNNN` 형식으로 Repository가 자동 채번한다 (주문번호는 `IClock`을 주입받아 날짜를 결정하므로 테스트에서 시간을 고정/목킹 가능).
- `MainController`는 `ISubMenuController*`(시료 관리/시료 주문/주문 승인·거절/생산라인 조회/출고 처리/모니터링)를 nullable로 주입받아 위임하고, `ProductionQueueProcessor*`를 주입받아 매 루프마다 `advanceQueue()`를 호출한다 — 메인 메뉴 1~6번이 모두 연결되어 있다.
- 재고는 **화면 표시용(`physical_stock`)**과 **내부 판단용(`available_stock`)**으로 이원화되어 있다 (`docs/PRD.md` 6.4 참고) — 주문 승인 시 `available_stock`만 즉시 변하고, `physical_stock`은 생산 완료(Phase 9, `+= real_production_quantity`)와 출고(Phase 10, `-= order.quantity`)에서만 변경된다. 수율 보정으로 생긴 여분(`real_production_quantity - order.quantity`)은 출고 후에도 재고로 남는다.
- 생산라인은 별도 스레드 없이 **폴링 기반**으로 동작한다: `ProductionQueueProcessor::advanceQueue()`가 메뉴 진입/메인 루프마다 현재 시각과 저장된 완료 예정 시각을 비교해 상태를 갱신하며, 재기동 시 밀려 있던 완료 건도 순서대로("백데이팅 체인") 한 번에 복원한다.
- 모니터링(`[4]`)은 `[1]` 주문 현황(상태별 건수, REJECTED 제외) / `[2]` 재고 현황(여유/부족/고갈, 미출고 수요 합계 기준)으로 나뉜 선택형 메뉴다 — 생산라인 조회와 달리 화면을 클리어하지 않고 선택할 때마다 결과가 아래로 계속 이어붙는다.

## 빌드 방법 (Visual Studio)
1. `SampleOrderSystem.sln`을 Visual Studio로 연다.
2. NuGet이 자동으로 `nlohmann.json`(3.12.0), `gmock`(1.11.0)을 복원한다 (안 되면 솔루션 우클릭 → NuGet 패키지 복원).
3. 구성을 **Debug / x64**로 맞추고 `Ctrl+Shift+B` 빌드.

## 실행 방법
- **앱**: `SampleOrderSystemApp`을 시작 프로젝트로 설정 후 `Ctrl+F5` 실행
  - `[1]` 시료 관리(등록/조회/검색, 5개씩 페이지네이션) · `[2]` 시료 주문(예약, Y/N 확인) · `[3]` 주문 승인/거절(재고 이중 관리) · `[4]` 모니터링(`[1]` 주문 현황 / `[2]` 재고 현황) · `[5]` 생산라인 조회(FIFO 진행 현황, `[R]` 새로고침) · `[6]` 출고 처리(`CONFIRMED` 목록, Y/N 확인) 모두 사용 가능
  - `0`으로 종료
- **테스트**: `SampleOrderSystemTest`를 시작 프로젝트로 설정 후 `Ctrl+F5` 실행

## 문서
- `docs/PRD.md`: 요구사항 정의
- `docs/PLAN.md`: Phase별 구현 계획
- `docs/CODE_CONVENTION.md` / `docs/COMMIT_CONVENTION.md`: 코드/커밋 컨벤션
- `CLAUDE.md`: 개발 가이드 (Claude Code가 자동으로 인식하도록 저장소 루트에 유지)
- `docs/design/phase{NN}_design.md`: Phase별 상세 설계 문서 (예: [`docs/design/phase05_design.md`](docs/design/phase05_design.md))
