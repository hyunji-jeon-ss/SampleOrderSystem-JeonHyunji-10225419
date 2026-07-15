# Phase 11 설계 문서 — 모니터링 기능

`PLAN.md` Phase 11, `PRD.md` 6.5에 대응. 상태별 주문 건수와 시료별 재고 현황(화면 표시용 재고 기준)을 한 화면에 통합해서 보여주는 조회 전용 메뉴다.

## 목표
- 주문 상태별 건수(REJECTED 제외)를 집계해서 보여준다.
- 시료별 화면 표시용 재고(`physical_stock`)와 "여유/부족/고갈" 상태를 보여준다.
- Phase 9~10에서 확정된 "개별 조회 메뉴"들과 달리, **주문 현황 + 재고 현황을 하나의 통합 화면**으로 보여준다 (착수 시 사용자와 확인해 개별 조회 메뉴는 만들지 않기로 결정).

## 착수 전 확인한 사항 (결정 기록)
Phase 9 설계 당시 남겨둔 사용자 요구사항 메모를 이번 Phase 착수 시점에 다시 확인해 아래와 같이 확정했다.
1. **메뉴 구성**: 개별 조회(주문 현황만/재고 현황만)는 만들지 않고, **통합 뷰 하나만 제공**한다.
2. **새로고침 방식**: Phase 9 생산라인 조회(화면 클리어 + `[R]`/`[0]` 루프)와 **다르게**, **화면을 클리어하지 않고 새로고침할 때마다 내용이 아래로 계속 쌓이는 방식**으로 한다. 서브메뉴 선택지(`[1]`/`[0]`)는 매번 다시 출력해 계속 입력받을 수 있게 한다.
3. **재고 상태(여유/부족/고갈) 판정 기준**: PRD에 "주문 대비 재고"라고만 되어 있어 모호했던 부분을 다음과 같이 확정했다 — 해당 시료에 대해 **아직 RELEASE되지 않은 주문(`RESERVED`/`PRODUCING`/`CONFIRMED`) 수량의 합계("미출고 수요")**와 `physical_stock`을 비교한다.
   - `physical_stock == 0` → **고갈**
   - `physical_stock < 미출고 수요 합계` → **부족**
   - 그 외 → **여유**
   - (`RESERVED`는 아직 승인 전이라 `available_stock`엔 반영되지 않았지만, "이 시료를 필요로 하는 미해결 요청"이라는 의미에서 미출고 수요에는 포함시킨다.)

## 메뉴 흐름 (Phase 9와 다른, "누적형" 새로고침 루프)
```
[4] 모니터링 선택
  → 화면 클리어 없이, 조회 시점의 스냅샷을 그대로 출력 (기존 내용 아래에 이어붙임)
  → "[1] 새로고침   [0] 뒤로가기 > " 표시
  → 입력이 "0": 반복 종료 (뒤로가기)
  → 그 외 입력("1" 포함): 다시 조회해서 아래에 새 스냅샷을 이어붙이고 반복
```
- 화면을 지우지 않으므로 새로고침할수록 이전 스냅샷들이 위에 그대로 남고 최신 스냅샷이 맨 아래 추가된다 — 시간 경과에 따른 변화를 스크롤해서 비교해볼 수 있다는 점이 이 메뉴의 의도된 특징이다 (Phase 9 생산라인 조회의 "덮어쓰기" 방식과 반대).
- `ProductionController`의 `"R"/그 외 입력 모두 반복` 패턴을 그대로 따르되, 화면 클리어 호출만 뺀다.

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

        virtual void showMonitoring(const std::string& current_time_text,
            const OrderStatusSummary& order_summary, const std::vector<StockStatusRow>& stock_rows) = 0;
        virtual void showMessage(const std::string& message) = 0;
};
```
- `REJECTED`는 `OrderStatusSummary`에 포함하지 않는다(PRD 6.5 "무효 주문이므로 제외").
- 재고가 비어 있는(등록된 시료가 없는) 경우 `stock_rows`가 비어 있을 수 있고, 이때는 "등록된 시료가 없습니다."를 표시한다.

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
        void display();
        OrderStatusSummary buildOrderStatusSummary();
        std::vector<StockStatusRow> buildStockStatusRows();

        IMonitoringView& view;
        IInputReader& input_reader;
        ISampleRepository& sample_repository;
        IOrderRepository& order_repository;
        IClock& clock;
};
```
- `buildOrderStatusSummary()`: `order_repository.findAll()`을 순회하며 상태별 카운트(REJECTED 제외).
- `buildStockStatusRows()`: `sample_repository.findAll()`의 각 시료에 대해, `order_repository.findAll()`에서 같은 `sample_id`이고 상태가 `RESERVED`/`PRODUCING`/`CONFIRMED`인 주문들의 `quantity`를 합산해 `pending_demand`를 구하고, 위 판정 기준으로 `status`를 계산한다.
- `run()`은 매 반복마다 `display()`(화면 클리어 없음) → 프롬프트 표시 → 입력 읽기 → `"0"`이면 종료, 아니면 계속.
- 이 컨트롤러는 상태를 변경하지 않는 **읽기 전용** 컨트롤러다 (Phase 9 `ProductionController`와 같은 성격 — 다만 새로고침 시 화면을 지우지 않는다는 점만 다르다).

## `MainController` 연동
- 생성자에 `ISubMenuController* monitoring_menu = nullptr`를 **일곱 번째(마지막) trailing default 파라미터**로 추가 (`sample_menu`, `order_menu`, `approval_menu`, `production_menu`, `ProductionQueueProcessor*`, `release_menu` 다음).
- `processCommand("4")`: `monitoring_menu`가 있으면 `monitoring_menu->run()`, 없으면 기존 placeholder. (이제 모든 메뉴 1~6이 연결되어 `runSubMenuOrShowPlaceholder(nullptr)` 분기는 더 이상 호출되지 않게 된다.)

## 출력 화면 예시
(정상 조회 → 한 번 새로고침한 뒤의 모습 — 스냅샷 두 개가 아래로 이어붙어 쌓인 상태)
```
--------------------------------------------------------------
[4] 모니터링   현재시각 2026-04-16 09:49:12

주문 현황
  예약중(RESERVED)     2건
  승인완료(CONFIRMED)  1건
  생산중(PRODUCING)    3건
  출고완료(RELEASED)   5건

재고 현황
ID      이름                        재고        상태
S-001   실리콘 웨이퍼-8인치          380 ea      여유
S-003   SiC 파워기판-6인치             0 ea      고갈
S-005   산화막 웨이퍼-SiO2            46 ea      부족

[1] 새로고침   [0] 뒤로가기 > 1
--------------------------------------------------------------
[4] 모니터링   현재시각 2026-04-16 09:49:47

주문 현황
  예약중(RESERVED)     2건
  승인완료(CONFIRMED)  2건
  생산중(PRODUCING)    2건
  출고완료(RELEASED)   5건

재고 현황
ID      이름                        재고        상태
S-001   실리콘 웨이퍼-8인치          380 ea      여유
S-003   SiC 파워기판-6인치            33 ea      여유
S-005   산화막 웨이퍼-SiO2            46 ea      부족

[1] 새로고침   [0] 뒤로가기 >
```
(등록된 시료가 없을 때)
```
--------------------------------------------------------------
[4] 모니터링   현재시각 2026-04-16 09:49:12

주문 현황
  예약중(RESERVED)     0건
  승인완료(CONFIRMED)  0건
  생산중(PRODUCING)    0건
  출고완료(RELEASED)   0건

재고 현황
등록된 시료가 없습니다.

[1] 새로고침   [0] 뒤로가기 >
```

## 테스트 계획 (gmock)
`MonitoringControllerTest` 신규:
- **주문 상태별 건수 집계** → `RESERVED`/`PRODUCING`/`CONFIRMED`/`RELEASED` 각각 정확히 카운트, `REJECTED`는 어떤 카운트에도 포함되지 않는지 확인
- **재고 상태 판정 — 여유** → `physical_stock`이 미출고 수요 합계보다 큰 시료가 `SUFFICIENT`로 분류되는지
- **재고 상태 판정 — 부족** → `physical_stock`이 0보다 크지만 미출고 수요 합계보다 작은 시료가 `SHORTAGE`로 분류되는지
- **재고 상태 판정 — 고갈** → `physical_stock == 0`인 시료가 (미출고 수요와 무관하게) `DEPLETED`로 분류되는지
- **미출고 수요 계산 시 `RESERVED`/`PRODUCING`/`CONFIRMED`만 합산되고 `REJECTED`/`RELEASED`는 제외되는지** 확인
- **등록된 시료가 없을 때** 빈 재고 목록 메시지가 표시되는지
- **"0" 입력 시 뒤로가기(루프 종료)**, **"1"(또는 그 외) 입력 시 반복(재조회) 및 화면 클리어를 호출하지 않는지**(`ConsoleUtil::clearConsoleScreen`을 호출하지 않는다는 것은 직접 단위 테스트로 검증하기 어려우므로, 대신 `display()`가 반복마다 `order_repository.findAll()`/`sample_repository.findAll()`을 다시 호출하는지로 "실시간 재조회"만 검증한다)
- `MainControllerTest`에 `monitoring_menu` 위임 케이스 추가

## 완료 기준
- 예약 → 승인 → 생산 → 출고까지 각 상태 전이 시나리오를 실제로 진행한 뒤, 모니터링 화면에서 주문 건수와 재고 상태가 매 새로고침마다 정확히 갱신되어 보이는지 확인
- REJECTED 주문이 어떤 집계에도 영향을 주지 않는지 확인
- 재고 0인 시료가 미출고 수요와 무관하게 항상 "고갈"로 표시되는지 확인
- gmock 테스트 전체 통과, 로컬 빌드/실행으로 실제 콘솔 흐름(새로고침 시 화면이 쌓이는 것) 검증

## 다음 Phase로 이월되는 항목
- 없음 — Phase 11이 `PLAN.md`상 마지막 기능 Phase다. 이후 Phase 12(통합 테스트/마무리)로 이어진다.
