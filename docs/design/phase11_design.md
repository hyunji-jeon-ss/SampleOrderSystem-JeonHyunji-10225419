# Phase 11 설계 문서 — 모니터링 기능

`PLAN.md` Phase 11, `PRD.md` 6.5에 대응. 상태별 주문 건수와 시료별 재고 현황(화면 표시용 재고 기준)을 조회하는 메뉴다.

> **개정 이력**: 최초 설계/구현 시점에는 "주문 현황+재고 현황 통합 뷰"로 만들었으나, 착수 전 확인 답변을 잘못 해석한 것으로 확인되어(사용자가 실제로 원한 것은 `[1]`/`[2]`로 나뉜 개별 조회) 이 문서와 구현을 아래 내용으로 다시 정정했다.

## 목표
- 주문 상태별 건수(REJECTED 제외)를 집계해서 보여준다.
- 시료별 화면 표시용 재고(`physical_stock`)와 "여유/부족/고갈" 상태를 보여준다.
- **주문 현황 조회**와 **재고 현황 조회**를 `[1]`/`[2]`로 나뉜 별도 선택지로 제공한다 (통합 화면이 아니다).

## 착수 전 확인한 사항 (결정 기록)
1. **메뉴 구성**: `[1] 주문 현황`, `[2] 재고 현황`, `[0] 뒤로가기`로 분리된 선택형 메뉴로 제공한다.
2. **새로고침/누적 방식**: Phase 9 생산라인 조회(화면 클리어 + `[R]`/`[0]` 루프)와 **다르게**, **화면을 클리어하지 않는다.** `[1]`/`[2]`를 누를 때마다 해당 조회 결과가 기존 내용 아래에 계속 이어붙고, 메뉴 선택지는 매번 다시 출력해서 반복적으로 `1`/`2`/`0`을 입력받을 수 있게 한다.
3. **재고 상태(여유/부족/고갈) 판정 기준**: PRD에 "주문 대비 재고"라고만 되어 있어 모호했던 부분을 다음과 같이 확정했다 — 해당 시료에 대해 **아직 RELEASE되지 않은 주문(`RESERVED`/`PRODUCING`/`CONFIRMED`) 수량의 합계("미출고 수요")**와 `physical_stock`을 비교한다.
   - `physical_stock == 0` → **고갈**
   - `physical_stock < 미출고 수요 합계` → **부족**
   - 그 외 → **여유**
   - (`RESERVED`는 아직 승인 전이라 `available_stock`엔 반영되지 않았지만, "이 시료를 필요로 하는 미해결 요청"이라는 의미에서 미출고 수요에는 포함시킨다.)

## 메뉴 흐름 (누적형 — 화면 클리어 없음)
```
[4] 모니터링 선택
  → "[4] 모니터링   현재시각 {현재시각}" 타이틀 + "[1] 주문 현황   [2] 재고 현황   [0] 뒤로가기 > " 표시
  → 입력이 "1": 주문 현황(상태별 건수)을 조회해서 기존 내용 아래에 이어붙임 → 위 메뉴로 돌아가 반복
  → 입력이 "2": 재고 현황(시료별 재고/상태)을 조회해서 기존 내용 아래에 이어붙임 → 위 메뉴로 돌아가 반복
  → 입력이 "0": 반복 종료 (뒤로가기)
  → 그 외 입력: "알 수 없는 명령입니다." 오류 메시지 → 위 메뉴로 돌아가 반복
```
- 화면을 지우지 않으므로 `1`/`2`를 여러 번 눌러볼수록 이전 조회 결과들이 위에 그대로 남고 최신 결과가 맨 아래 추가된다 — 시간 경과에 따른 변화를 스크롤해서 비교해볼 수 있다는 점이 이 메뉴의 의도된 특징이다 (Phase 9 생산라인 조회의 "덮어쓰기" 방식과 반대).
- `SampleController`(Phase 6)의 "선택지 → 처리 → 같은 화면 계속 반복" 루프 패턴과 같은 계열이지만, 여기는 조회 전용이라 상태 변경이 없다.

## `IMonitoringView` / `ConsoleMonitoringView`
```cpp
struct OrderStatusSummary
{
    int reserved_count = 0;
    int confirmed_count = 0;
    int producing_count = 0;
    int released_count = 0;
};

enum class StockStatus { SUFFICIENT, SHORTAGE, DEPLETED };

struct StockStatusRow
{
    std::string sample_id;
    std::string sample_name;
    int physical_stock = 0;
    int pending_demand = 0;   // 미출고 수요 합계 (RESERVED+PRODUCING+CONFIRMED 수량)
    StockStatus status = StockStatus::SUFFICIENT;
};

class IMonitoringView
{
    public:
        virtual ~IMonitoringView() = default;

        virtual void showMenu(const std::string& current_time_text) = 0;
        virtual void showOrderStatus(const OrderStatusSummary& order_summary) = 0;
        virtual void showStockStatus(const std::vector<StockStatusRow>& stock_rows) = 0;
        virtual void showMessage(const std::string& message) = 0;
};
```
- `REJECTED`는 `OrderStatusSummary`에 포함하지 않는다(PRD 6.5 "무효 주문이므로 제외").
- 재고가 비어 있는(등록된 시료가 없는) 경우 `stock_rows`가 비어 있을 수 있고, 이때는 "등록된 시료가 없습니다."를 표시한다.
- `showMenu()`는 반복마다 호출되어 타이틀+현재시각과 선택지를 다시 출력한다(화면 클리어 없이 이어붙임).

## `MonitoringController` 설계
```cpp
class MonitoringController : public ISubMenuController
{
    public:
        MonitoringController(IMonitoringView& view, IInputReader& input_reader,
            ISampleRepository& sample_repository, IOrderRepository& order_repository, IClock& clock);

        void run() override;
        bool processCommand(const std::string& command);

    private:
        OrderStatusSummary buildOrderStatusSummary();
        std::vector<StockStatusRow> buildStockStatusRows();

        IMonitoringView& view;
        IInputReader& input_reader;
        ISampleRepository& sample_repository;
        IOrderRepository& order_repository;
        IClock& clock;
};
```
- `run()`: 반복마다 `view.showMenu(현재시각)` → 입력 읽기 → `processCommand()` 결과로 계속/종료 판단. 화면 클리어 없음.
- `processCommand("1")`: `view.showOrderStatus(buildOrderStatusSummary())` 호출 후 `true`(계속).
- `processCommand("2")`: `view.showStockStatus(buildStockStatusRows())` 호출 후 `true`(계속).
- `processCommand("0")`: `false`(종료).
- 그 외: `view.showMessage("알 수 없는 명령입니다.")` 후 `true`(계속).
- `buildOrderStatusSummary()`: `order_repository.findAll()`을 순회하며 상태별 카운트(REJECTED 제외).
- `buildStockStatusRows()`: `sample_repository.findAll()`의 각 시료에 대해, `order_repository.findAll()`에서 같은 `sample_id`이고 상태가 `RESERVED`/`PRODUCING`/`CONFIRMED`인 주문들의 `quantity`를 합산해 `pending_demand`를 구하고, 위 판정 기준으로 `status`를 계산한다.
- 이 컨트롤러는 상태를 변경하지 않는 **읽기 전용** 컨트롤러다.

## `MainController` 연동
- 생성자에 `ISubMenuController* monitoring_menu = nullptr`를 **일곱 번째(마지막) trailing default 파라미터**로 추가 (`sample_menu`, `order_menu`, `approval_menu`, `production_menu`, `ProductionQueueProcessor*`, `release_menu` 다음).
- `processCommand("4")`: `monitoring_menu`가 있으면 `monitoring_menu->run()`, 없으면 기존 placeholder.

## 출력 화면 예시
(주문 현황 → 재고 현황 순서로 조회해본 모습 — 매번 화면이 지워지지 않고 아래로 이어붙는다)
```
--------------------------------------------------------------
[4] 모니터링   현재시각 2026-04-16 09:49:12
[1] 주문 현황   [2] 재고 현황   [0] 뒤로가기 > 1

주문 현황
  예약중(RESERVED)     2건
  승인완료(CONFIRMED)  1건
  생산중(PRODUCING)    3건
  출고완료(RELEASED)   5건

--------------------------------------------------------------
[4] 모니터링   현재시각 2026-04-16 09:49:20
[1] 주문 현황   [2] 재고 현황   [0] 뒤로가기 > 2

재고 현황
ID      이름                        재고        상태
S-001   실리콘 웨이퍼-8인치          380 ea      여유
S-003   SiC 파워기판-6인치             0 ea      고갈
S-005   산화막 웨이퍼-SiO2            46 ea      부족

--------------------------------------------------------------
[4] 모니터링   현재시각 2026-04-16 09:49:25
[1] 주문 현황   [2] 재고 현황   [0] 뒤로가기 >
```
(등록된 시료가 없을 때 `[2]`를 선택한 경우)
```
재고 현황
등록된 시료가 없습니다.
```

## 테스트 계획 (gmock)
`MonitoringControllerTest`:
- **주문 상태별 건수 집계** → `RESERVED`/`PRODUCING`/`CONFIRMED`/`RELEASED` 각각 정확히 카운트, `REJECTED`는 어떤 카운트에도 포함되지 않는지 확인 (`processCommand("1")` 호출로 검증)
- **재고 상태 판정 — 여유** → `physical_stock`이 미출고 수요 합계보다 큰 시료가 `SUFFICIENT`로 분류되는지 (`processCommand("2")`)
- **재고 상태 판정 — 부족** → `physical_stock`이 0보다 크지만 미출고 수요 합계보다 작은 시료가 `SHORTAGE`로 분류되는지
- **재고 상태 판정 — 고갈** → `physical_stock == 0`인 시료가 (미출고 수요와 무관하게) `DEPLETED`로 분류되는지
- **미출고 수요 계산 시 `RESERVED`/`PRODUCING`/`CONFIRMED`만 합산되고 `REJECTED`/`RELEASED`는 제외되는지** 확인
- **등록된 시료가 없을 때** 빈 재고 목록 메시지가 표시되는지
- **"1"/"2" 입력 시 각각 대응하는 화면만 그려지고 다른 쪽은 호출되지 않는지** (`showOrderStatus`/`showStockStatus` 상호 배타적 호출 검증)
- **"0" 입력 시 뒤로가기(루프 종료)**, **알 수 없는 명령 입력 시 오류 메시지 후 계속(루프 종료 안 됨)**
- `MainControllerTest`에 `monitoring_menu` 위임 케이스 추가

## 완료 기준
- 예약 → 승인 → 생산 → 출고까지 각 상태 전이 시나리오를 실제로 진행한 뒤, `[1]`/`[2]`를 각각 조회했을 때 주문 건수와 재고 상태가 정확히 갱신되어 보이는지 확인
- REJECTED 주문이 어떤 집계에도 영향을 주지 않는지 확인
- 재고 0인 시료가 미출고 수요와 무관하게 항상 "고갈"로 표시되는지 확인
- `[1]`/`[2]`를 반복 선택해도 화면이 지워지지 않고 계속 아래로 쌓이는지 확인
- gmock 테스트 전체 통과, 로컬 빌드/실행으로 실제 콘솔 흐름 검증

## 다음 Phase로 이월되는 항목
- 없음 — Phase 11이 `PLAN.md`상 마지막 기능 Phase다. 이후 Phase 12(통합 테스트/마무리)로 이어진다.

## 구현 결과 (완료)
- 설계 그대로 구현됨(최초에는 통합 뷰로 잘못 구현했다가, 사용자가 착수 전 답변을 다시 확인해달라고 요청해 `[1]`/`[2]`/`[0]` 분리형으로 정정 — 위 "개정 이력" 참고).
- `IMonitoringView`(`OrderStatusSummary`/`StockStatus`/`StockStatusRow`), `ConsoleMonitoringView`, `MonitoringController` 구현.
- `run()`은 화면 클리어 없이 매 반복마다 `view.showMenu(현재시각)` → 입력 읽기 → `processCommand()`. `"1"`은 `showOrderStatus()`, `"2"`는 `showStockStatus()`를 호출한 뒤 계속 반복(둘 다 화면을 지우지 않으므로 선택할 때마다 결과가 아래로 이어붙는다), `"0"`은 종료, 그 외는 오류 메시지 후 계속.
- `buildStockStatusRows()`가 시료별로 `order_repository.findAll()`을 순회해 `RESERVED`/`PRODUCING`/`CONFIRMED` 주문의 수량 합계(미출고 수요)를 구하고, `physical_stock == 0` → 고갈, `physical_stock < 미출고 수요` → 부족, 그 외 → 여유로 판정한다.
- `MainController`에 `monitoring_menu`(일곱 번째, 마지막 trailing default)로 연결, `processCommand("4")`가 위임되도록 변경 — 메인 메뉴 1~6번이 모두 연결 완료.
- `main.cpp`에 `ConsoleMonitoringView`/`MonitoringController` 인스턴스 배선.
- gmock 테스트 80개 전체 통과 (`MonitoringControllerTest` 8개: 주문 상태별 집계 및 REJECTED 제외, 재고 상태 여유/부족/고갈 3가지 판정, 미출고 수요 계산 시 REJECTED/RELEASED 제외, 빈 시료 목록, 메뉴 반복 호출, 알 수 없는 명령, 뒤로가기 / `MainControllerTest` 2개: `monitoring_menu` 위임, 플레이스홀더).
- 실제 실행(수동 스모크 테스트)으로 검증: `[4]` 진입 → `"1"` 입력 시 주문 현황만 출력 → `"2"` 입력 시 재고 현황만 출력(둘 다 화면이 지워지지 않고 이전 내용 아래에 이어붙음) → 알 수 없는 명령 입력 시 오류 메시지 후 메뉴가 다시 표시됨 → `"0"` 입력 시 메인 메뉴로 정상 복귀.
