# Phase 12 설계 문서 — 통합 테스트 및 마무리

`PLAN.md` Phase 12에 대응. 이 문서는 다른 Phase 문서들과 달리 **새 기능을 설계하는 문서가 아니라, "무엇을 확인하고 정리할 것인가"를 미리 정해두는 체크리스트**다. Phase 5~11에서 이미 모든 기능(시료 관리/주문/승인/생산/출고/모니터링)이 구현·테스트·커밋된 상태이므로, Phase 12의 산출물은 새 코드가 아니라 **검증 결과와 정리(필요 시 소규모 수정 커밋)**다.

## 목표 (PLAN.md)
- 전체 기능 통합 검증
- gmock 테스트 커버리지 점검
- Clean Code 리뷰
- 문서 최종화
- 커밋 이력 정리
- 5개 저장소 모두 최신 상태로 push, 제출 준비 완료

## 점검 항목

### 1. E2E 시나리오 수동 검증
아래 시나리오들을 실제 콘솔 앱으로 순서대로 실행하며 각 단계의 기대 결과를 확인한다 (평균 생산시간을 짧게 설정한 시료로 진행하면 실제 대기 없이 빠르게 검증 가능 — Phase 9에서 쓰던 방식과 동일).

| # | 시나리오 | 확인 포인트 |
|---|---|---|
| 1 | 시료 등록 → 재고 충분한 상태에서 주문 → 승인 → 출고 | `available_stock`은 승인 시 즉시 차감, `physical_stock`은 출고 시에만 차감되는지 |
| 2 | 재고 0인 시료에 주문 → 승인(부족 분기) → 생산라인 조회에서 진행률 확인 → 생산 완료 → 출고 → 모니터링에서 재고 상태 확인 | 실생산량(수율 보정)이 정확히 계산되고, 출고 시엔 주문 수량만 차감되어 여분이 남는지 |
| 3 | 주문 접수 후 거절 | 주문 상태만 `REJECTED`로 바뀌고 재고(가용/화면 모두)는 전혀 변하지 않는지 |
| 4 | 동일 시료에 부족 주문 2건 이상 접수 → 순차 승인 | FIFO 큐에 순서대로 들어가고, 생산라인 조회에서 대기 목록 순서/예상 완료 시각이 올바른지 |
| 5 | 생산 중 앱 종료 → 완료 시각이 지난 뒤 재실행 | 재기동 시 밀린 완료 건이 백데이팅 체인으로 순서대로 자동 완료 처리되는지 (재고 반영 + 상태 전환) |
| 6 | 시나리오 1~5 진행 중간중간 모니터링(`[1]`/`[2]`) 조회 | 매 단계마다 주문 건수/재고 상태가 그 시점 기준으로 정확히 반영되는지, REJECTED는 집계에서 빠지는지 |
| 7 | 잘못된 입력(숫자 아닌 수량, 존재하지 않는 시료 ID, 목록에 없는 번호 선택 등) | 각 메뉴가 죽지 않고 오류 메시지 후 정상적으로 메뉴로 복귀하는지 |

### 2. gmock 테스트 커버리지 점검
현재 테스트 현황(참고용 — Phase 12 시작 시점 기준):

| 테스트 파일 | 개수 | 대상 |
|---|---|---|
| JsonSampleRepositoryTest | 9 | 시료 JSON 영속성 |
| JsonOrderRepositoryTest | 6 | 주문 JSON 영속성 |
| SampleControllerTest | 8 | 시료 등록/조회/검색 |
| OrderControllerTest | 6 | 주문 접수 |
| ApprovalControllerTest | 6 | 승인/거절, 재고 이중 관리 |
| ProductionCalculatorTest | 6 | 실생산량/소요시간 계산 |
| ProductionQueueProcessorTest | 4 | FIFO 큐 폴링/백데이팅 |
| ProductionControllerTest | 5 | 생산라인 조회 화면 |
| ReleaseControllerTest | 6 | 출고 처리 |
| MonitoringControllerTest | 9 | 모니터링 |
| MainControllerTest | 16 | 메인 메뉴 위임/요약 |
| **합계** | **81** | |

점검할 것:
- 각 Phase 설계 문서의 "테스트 계획" 항목과 실제 테스트가 1:1로 대응하는지 재확인 (빠진 케이스가 있으면 이번 Phase에서 보완)
- 아직 명시적으로 테스트되지 않은 경계값 후보 검토(발견되면 `<TEST>` 커밋으로 추가, 없으면 "확인함"으로 기록):
  - 수량에 음수/0/비정상 문자열을 입력했을 때 각 컨트롤러의 처리
  - `avg_production_time_min`/`yield`에 극단값(0, 매우 큰 값)을 등록했을 때 계산 결과
  - 같은 시료에 대해 승인 대기 주문이 많을 때 FIFO 순서 안정성
  - JSON 파일이 비어있거나(최초 실행) 손상된 경우 Repository 동작
- 새 테스트가 필요 없다고 판단되면 그 이유를 이 문서에 기록해 "일부러 안 만든 것"과 "빠뜨린 것"을 구분한다.

### 3. Clean Code 리뷰
- **중복 코드**: `ApprovalController`/`ReleaseController`의 "목록 조회 → 번호 선택 → 처리" 루프 구조가 유사한데, 무리하게 공통화하지 않고 각자의 도메인 로직 차이(충분/부족 분기 vs 없음)를 고려해 리팩터링 여지만 판단한다.
- **메모리 관리**: `new`/`delete`가 코드베이스 어디에도 없는지(전부 참조/스택 기반 DI인지) 재확인.
- **코드 컨벤션 준수**: `CODE_CONVENTION.md`(네이밍, include 순서, public/protected/private 순서) 위반 여부 훑어보기.
- **불필요한 코드 정리**: 사용하지 않는 include, 죽은 코드, 남아있는 디버그용 흔적이 없는지.
- **SOLID 원칙 검토**: 각 원칙 관점에서 이 코드베이스가 실제로 어떻게 지켜지고 있는지(혹은 어긋난 부분이 있는지) 점검한다.
  - **SRP(단일 책임)**: Controller(흐름 제어)/View(출력)/Repository(영속성)/Calculator·QueueProcessor(도메인 계산)로 계층이 잘 분리되어 있는지. `MainController`가 여러 서브메뉴를 쥐고 있어 책임이 비대해지지 않았는지(단순 위임(`runSubMenuOrShowPlaceholder`)에 그치는지 확인).
  - **OCP(개방-폐쇄)**: 새 메뉴(Phase 6~11)를 추가할 때마다 기존 `MainController`의 시그니처(trailing default 파라미터 추가)만 건드리고 기존 로직은 안 건드렸는지 — 완전한 OCP는 아니지만(파라미터 추가 자체가 수정이므로) 트레이드오프를 문서에 기록.
  - **LSP(리스코프 치환)**: `ISubMenuController*`로 주입되는 5개 컨트롤러(Sample/Order/Approval/Production/Release/Monitoring)가 모두 `run()`만으로 상호 치환 가능한지, Mock 구현체들이 실제 구현체와 동일한 계약을 지키는지.
  - **ISP(인터페이스 분리)**: `IApprovalView`/`IReleaseView`/`IProductionView`/`IMonitoringView`처럼 View 인터페이스를 기능별로 잘게 나눴는지(하나의 거대한 `IView`로 뭉치지 않았는지) 재확인.
  - **DIP(의존성 역전)**: 모든 Controller가 구체 클래스(`JsonSampleRepository` 등)가 아니라 인터페이스(`ISampleRepository` 등)에 의존하는지, `main.cpp`에서만 구체 타입을 조립하는지.
  - 위반 사례가 발견되면 목록화하고, 실제 리팩터링할지 "의도된 트레이드오프"로 남길지 판단 근거를 함께 기록한다.

### 4. 문서 최종화
- `PRD.md`와 실제 구현 사이에 불일치가 없는지 최종 대조 (특히 6.4~6.7의 재고 이중 관리/생산/출고 로직).
- 각 `docs/**/phase{NN}_design.md`의 "구현 결과" 섹션이 최신 코드 상태와 맞는지 확인 (Phase 11은 설계 정정 이력까지 반영되어 있음).
- `README.md`의 구조/실행 방법 섹션이 Phase 11까지의 최종 상태를 반영하는지.
- `CLAUDE.md` 진행 상황 로그가 Phase 11까지 모두 기록되어 있는지, 아래 "문서 폴더 정리"로 경로가 바뀐 링크(`docs/phase{NN}_design.md` 등)가 있다면 함께 갱신.
- `PLAN.md`(루트 `Semiconductor/PLAN.md` + 저장소 내 사본) 양쪽이 서로 동기화되어 있고 Phase 0~11 모두 완료 표시가 되어 있는지, 문서 이동 후 경로 표기도 갱신.
- **Harness 섹션 점검**: `CLAUDE.md`의 "Harness — 커맨드라인 빌드/테스트 명령어" 절(MSBuild/nuget.exe 경로, `.sln` 기준 빌드, gmock 테스트 실행, 스모크 테스트 커맨드)이 지금 이 시점에도 실제로 그대로 동작하는지 재실행해서 검증하고, 문서 이동 후에도 경로 참조가 깨지지 않았는지 확인한다. (이 프로젝트 전체를 진행하며 Claude Code 세션에서 VS GUI 없이 빌드/테스트를 검증할 때 계속 이 섹션의 명령어를 사용해왔다 — 제출 전 최종적으로 다시 한번 정확성을 확인해야 할 항목.)

### 5. 문서 폴더 정리 (신규)
현재 저장소 루트에 `CLAUDE.md`/`CODE_CONVENTION.md`/`COMMIT_CONVENTION.md`/`PLAN.md`/`PRD.md`/`README.md`가 전부 최상위에 흩어져 있어, 문서 성격에 따라 정리한다.

- **루트에 남기는 파일**: `README.md`(GitHub가 저장소 루트의 README만 자동으로 보여주므로 이동 불가), `CLAUDE.md`(Claude Code가 작업 디렉터리 기준으로 자동 탐색/로드하는 파일이라 루트에 있어야 세션에서 계속 인식됨 — 옮기면 이 프로젝트에서 지금까지 해온 방식 자체가 깨진다).
- **`docs/`로 이동**: `PRD.md`, `PLAN.md`, `CODE_CONVENTION.md`, `COMMIT_CONVENTION.md` — 설계/컨벤션 성격의 참고 문서이므로 루트에 있을 필요가 없다.
- **`docs/design/`로 이동**: 기존 `docs/phase05_design.md` ~ `docs/phase12_design.md` 전부 — Phase별 설계 문서만 모아두는 하위 폴더를 새로 만든다.
- **후속 작업**: 파일을 옮기면 아래 항목들의 상대경로 링크를 전부 갱신해야 한다.
  - `README.md`의 "문서" 절 (`PRD.md`, `PLAN.md`, `docs/phase{NN}_design.md` 링크)
  - `CLAUDE.md`의 "설계 문서"/"진행 상황" 절 (`docs/phase{NN}_design.md` 링크)
  - `PLAN.md` 자체 내부의 `SampleOrderSystem-JeonHyunji-10225419/docs/phase{NN}_design.md` 참조 문자열들(루트 `Semiconductor/PLAN.md` 사본 포함)
  - 각 `docs/design/phase{NN}_design.md` 문서 간의 상호 참조(있다면)
- 이동은 `git mv`로 수행해 히스토리(파일별 blame/log)가 끊기지 않게 한다.

### 6. JSON 예시 데이터 커밋 (신규)
지금까지 `samples.json`/`orders.json`은 런타임 파일이라는 이유로 `.gitignore`에 포함되어 한 번도 커밋되지 않았다. 채점/실행 편의를 위해 **예시 데이터로 커밋**하기로 결정한다.

- **`SampleOrderSystemApp/samples.json`**: 지금처럼 시료 10종 예시 데이터 유지(이미 만들어져 있는 것 재사용/다듬기).
- **`SampleOrderSystemApp/orders.json`**: `OrderStatus`의 각 상태(`RESERVED`/`PRODUCING`/`CONFIRMED`/`RELEASED`/`REJECTED`)별로 약 3건씩, 총 15건 내외의 예시 주문을 만들어 넣는다 — 실행하자마자 모니터링/생산라인/출고 화면에서 다양한 상태를 바로 확인할 수 있게 하려는 목적.
  - `PRODUCING` 예시는 `production_start_millis`/`production_end_millis`를 실행 시점 기준으로 자연스럽게(가까운 미래 완료 등) 잡아서, 실행하자마자 생산라인 조회에서 진행 중인 모습을 볼 수 있게 한다.
  - `enqueued_at_millis`(FIFO 정렬 기준)도 상태에 맞게 그럴듯한 순서로 채운다.
- **`.gitignore` 갱신**: `samples.json`/`orders.json`을 전면 무시하던 규칙을, `SampleOrderSystemApp/` 아래의 두 파일만 추적하고 그 외(예: `x64/Debug/` 밑에 생기는 실행 중 사본, 로컬 스모크 테스트용 임시 복사본)는 계속 무시하도록 네거티브 패턴을 추가한다. `test_*.json`은 계속 무시(테스트가 생성하는 임시 파일).
- **주의**: 이 두 파일은 앱을 한 번이라도 실행하면 실제로 값이 바뀐다(주문 진행 등). 커밋 시점의 "예시 스냅샷"이라는 성격을 README/CLAUDE.md에 명시해서, 실행 후 `git status`에 변경이 뜨는 게 정상이라는 걸 알 수 있게 한다.

### 7. 커밋 이력 정리
- 전체 커밋 로그를 훑어보며 `COMMIT_CONVENTION.md` 형식(`<HEADER> 내용`)을 지켰는지 확인.
- 빌드 실패 상태로 커밋된 이력이 없는지 확인 — 있었다면 이후 `<FIX>` 커밋으로 정상적으로 이어졌는지만 확인하고, **과거 커밋을 억지로 되돌리거나 rebase하지 않는다** (히스토리 재작성 금지 원칙 유지).

### 8. 저장소 상태 점검 (5개 저장소 모두)
`ConsoleMVC-JeonHyunji-10225419` / `DataPersistence-JeonHyunji-10225419` / `DataMonitor-JeonHyunji-10225419` / `DummyDataGenerator-JeonHyunji-10225419` / `SampleOrderSystem-JeonHyunji-10225419` 각각에 대해:
- GitHub 저장소가 Public 상태인지
- 로컬 `master`와 `origin/master`가 정확히 일치하는지(누락된 push 없는지)
- `README.md`가 각 PoC의 목적/실행 방법을 정확히 설명하는지
- `.gitignore`가 런타임 산출물(JSON 데이터 파일 등)과 빌드 산출물(`x64/`, `.vs/` 등)을 올바르게 제외하고 있는지

## 완료 기준
- 위 8개 항목을 모두 실제로 점검하고, 이 문서(또는 별도 정리 커밋)에 결과를 기록한다.
- 점검 중 발견된 문제는 각각 알맞은 커밋 태그(`<FIX>`/`<REFACTOR>`/`<DOCS>`/`<TEST>`/`<CHORE>`)로 정리해서 커밋한다.
- 문서 폴더 정리(5번) 이후에도 빌드가 정상 동작하고(문서 이동은 코드에 영향 없음), 모든 문서 내부 링크가 깨지지 않았는지 확인한다.
- JSON 예시 데이터(6번) 커밋 후에도 앱이 정상 실행되고 각 화면에서 다양한 상태의 예시 데이터가 보이는지 확인한다.
- 5개 저장소 모두 최신 상태로 push 완료된 것을 최종 확인한다.

## 참고
- 이 문서는 "초안" 상태로 작성되었다 — 실제 점검을 시작하기 전에 위 체크리스트 항목/우선순위에 대해 사용자 확인을 받는다.
- **실행 순서**: 1) Clean Code 리뷰(+SOLID) → 2) gmock 커버리지 점검 → 3) E2E 시나리오 수동 검증 → 4) 문서 폴더 정리 → 5) 문서 최종화(+Harness 점검) → 6) JSON 예시 데이터 커밋 → 7) 커밋 이력 정리 → 8) 저장소 상태 점검. (이 문서 안의 번호와 실제 실행 순서는 다르다 — 문서는 주제별 분류, 아래 기록은 실행 순서를 따른다.)

## 점검 결과 기록

### 1) Clean Code 리뷰(+SOLID) — 완료
**SOLID**: SRP/LSP/ISP/DIP 모두 양호. OCP는 Phase마다 `MainController` 생성자에 trailing default 파라미터가 늘어나는 형태라 엄밀히는 위반이지만, 기존 호출부를 전혀 건드리지 않는 방식이라 **의도된 트레이드오프로 남긴다** (완전한 OCP를 위한 메뉴 레지스트리 구조로 바꾸는 건 이 시점에 리스크 대비 이득이 낮다고 판단).

**중복 코드 2건 발견**:
- `fetchProducingOrdersByFifo()`가 `production/ProductionQueueProcessor.cpp`(구 private 멤버)와 `controller/ProductionController.cpp`(익명 네임스페이스 함수)에 완전히 동일한 로직으로 중복되어 있었다. → **수정함**: `ProductionQueueProcessor.h`에 공용 자유 함수 `fetchProducingOrdersByFifo(IOrderRepository&)`로 노출하고, `ProductionQueueProcessor::advanceQueue()`와 `ProductionController::display()` 양쪽 모두 이 함수를 호출하도록 정리(중복 정의 제거). gmock 테스트 80개 재실행 통과 확인, `<REFACTOR>` 커밋으로 반영 예정.
- `ApprovalController`/`ReleaseController`의 "목록 조회 → 번호 선택 → Y/N 확인 → 처리" 스캐폴딩(`run()`/`processCommand()`의 번호 파싱+범위 체크, `buildXxxRows()`의 상태 필터+시료명 조회 패턴)이 구조적으로 유사함. → **수정하지 않기로 결정**: 실제 도메인 로직(재고 이중 관리 분기 vs 단순 출고)이 서로 달라서 억지로 공통 베이스/헬퍼로 뽑으면 오히려 각 컨트롤러의 의도가 흐려질 위험이 있다고 판단, 학교 프로젝트 규모상 지금 상태를 **의도된 트레이드오프로 유지**한다.

메모리 관리(`new`/`delete` 없음), 죽은 코드, `CODE_CONVENTION.md` 준수는 전부 문제 없이 확인됨.

### 2) gmock 테스트 커버리지 점검 — 완료
각 Phase 설계 문서의 "테스트 계획" 항목과 실제 테스트를 1:1로 대조한 결과 빠진 케이스는 없었다(모두 대응하는 `TEST(...)`가 존재). 체크리스트에 적어둔 경계값 후보들을 검토한 결과:

- **`production/ProductionCalculator.cpp`의 `yield <= 0.0` 분기(부족분을 그대로 반환하는 안전장치)가 테스트 커버리지 0이었다.** → 실제 코드에 존재하는 분기인데 검증이 전혀 없던 진짜 빈틈이라 판단해 **`ProductionCalculatorTest.ZeroOrNegativeYieldFallsBackToShortageQuantity`를 추가**(`yield==0`, `yield<0` 둘 다 검증). `<TEST>` 커밋으로 반영.
- **수량 음수 입력(`OrderController`/`ApprovalController`/`ReleaseController`의 `quantity`/번호 파싱)**: 이미 있는 `ZeroQuantityIsRejected`/`InvalidIndexShowsErrorAndDoesNotChangeAnything` 등과 완전히 같은 코드 분기(`quantity < 1`, `index < 1`)를 타므로, 별도 테스트를 추가하지 않고 **"동일 동치 클래스로 이미 커버됨"**으로 기록만 한다.
- **`avg_production_time_min`/`yield` 극단값(등록 화면 입력)**: `SampleController::handleRegister()`는 PRD 예시 프롬프트("수율(0~1)")와 무관하게 범위 검증을 하지 않는다 — 숫자 파싱만 되면 그대로 저장된다. 이건 버그라기보다 **PRD가 범위 검증을 요구하지 않는 항목**이라 스코프 밖으로 판단, 새 테스트/검증 로직을 추가하지 않고 "확인함, 의도적으로 미구현"으로 기록.
- **동일 시료에 대기 주문 다수(3건 이상)일 때 FIFO 순서 안정성**: `ProductionQueueProcessorTest.BackdatedChainCompletesMultipleOrdersInOneCall`가 2건 체인은 검증하지만 3건 이상은 없다. 다만 정렬 기준(`enqueued_at_millis` 오름차순)과 체인 로직 자체는 건수에 의존하지 않는 일반 로직이라 2건 검증으로 3건 이상도 같은 방식으로 동작함을 충분히 신뢰할 수 있다고 판단 — **추가 테스트 없이 "확인함"으로 기록**.
- **JSON 파일이 비어있는 경우**: `JsonSampleRepositoryTest.FindAllReturnsEmptyWhenFileDoesNotExist` 등으로 이미 커버됨.
- **JSON 파일이 손상된(문법 오류) 경우**: `repository/JsonFileStore.cpp`의 `readJsonArray()`는 파싱 실패 시 예외를 그대로 던지며 어디서도 catch하지 않는다 — 즉 손상된 파일이 있으면 앱이 즉시 종료(크래시)된다. 이 파일은 앱 자신이 전적으로 관리하는 영속 파일이고(사용자가 직접 수정하는 파일이 아님) PRD에도 손상 복구 요구사항이 없어 **스코프 밖의 의도적 미방어로 기록**하고 새 테스트/방어 코드를 추가하지 않는다.

결과: gmock 테스트 80개 → **81개**로 증가(신규 1개), 나머지 경계값 후보는 전부 "이미 커버됨" 또는 "스코프 밖"으로 근거를 기록하고 마무리.
