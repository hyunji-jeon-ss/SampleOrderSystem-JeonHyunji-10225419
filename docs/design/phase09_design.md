# Phase 9 설계 문서 — 생산라인 기능 (실시간 처리 + 재기동 복원)

`PLAN.md` Phase 9, `PRD.md` 6.6에 대응. `PRODUCING` 상태 주문을 단일 생산 라인(FIFO)에서 실시간으로 처리하고, 완료 시 화면 표시 재고(`physical_stock`)에 반영한다. 프로그램이 꺼져 있던 동안 완료됐어야 할 생산도 재기동 시 정확히 복원한다.

> **PDF 예시와의 관계**: 생산라인 화면은 PDF 예시(진행률 바 `진행 ████░░ 72%`, 완료 예정 시각 포함)를 그대로 따른다. 진행률(%)은 백그라운드 타이머 없이도 조회 시점에 `(현재시각 - 시작시각) / (완료시각 - 시작시각)`로 즉시 계산 가능하므로, 아래 "폴링 기반" 설계와 무리 없이 함께 간다 (실시간으로 화면이 저절로 움직이는 애니메이션은 아니고, 메뉴를 조회할 때마다 그 시점 기준 스냅샷을 보여주는 방식).
> (참고: 잔여율/진행률 표시를 뺀 것은 Phase 11 모니터링 화면 얘기였고, 이 Phase 9 생산라인 조회에는 해당하지 않는다.)

## 목표
- 재고 부족으로 `PRODUCING` 전환된 주문들을 하나의 생산 라인이 FIFO로 순차 처리한다.
- 생산 시간은 현실 시간 그대로 흐른다(배율 없음). 완료 시점에만 `physical_stock`에 실생산량 전량을 반영한다.
- 프로그램을 껐다 켜도, 꺼져 있던 동안 완료됐어야 할 생산은 재기동 시 자동으로 완료 처리된다(여러 건이 밀려 있어도 순서대로 전부 처리).
- 생산 현황(현재 처리 중 + 대기 목록)을 조회할 수 있다.

## 핵심 설계 결정: "폴링 기반" 실시간 처리
콘솔 앱은 사용자의 입력을 기다리는 동안만 실행되는 동기 프로그램이라, 백그라운드 타이머 스레드를 두지 않는다. 대신:

- **생산 큐 전진 로직(`ProductionQueueProcessor::advanceQueue()`)을 "메뉴가 그려질 때마다" 호출**해서, 그 시점의 실제 시각과 저장된 완료 예정 시각을 비교해 상태를 갱신한다.
- 호출 지점: ① `MainController`가 메인 메뉴를 그리기 직전(매 루프 반복마다, "생산라인 N건 대기" 카운트가 항상 최신 상태이도록), ② `ProductionController`(생산라인 조회 메뉴) 진입 시.
- 진행률(%)이 애니메이션처럼 부드럽게 움직이지는 않지만(메뉴를 다시 열어야 갱신됨), **완료 여부 판정과 진행률 계산 자체는 그 순간의 실제 경과 시간과 정확히 일치**한다 — 사용자가 언제 메뉴를 열어보든, 그 시점 기준으로 이미 지난 완료 시각은 즉시 완료 처리되고, 진행 중이면 그 시점 기준 정확한 진행률이 계산된다.

## 도메인 모델 변경: `Order`에 필드 추가
```cpp
struct Order
{
    ...
    int shortage_quantity = 0;          // Phase 8에서 추가됨
    long long enqueued_at_millis = 0;   // Phase 8에서 추가됨 (PRODUCING 전환 시각 = FIFO 정렬 기준)

    int real_production_quantity = 0;       // 생산 시작 시 계산·고정 (ceil(shortage/yield))
    long long production_start_millis = 0;  // 실제 생산 시작 시각. 0이면 아직 대기 중(생산 미시작)
    long long production_end_millis = 0;    // 생산 완료(예정) 시각
};
```
- `real_production_quantity`/`production_start_millis`/`production_end_millis`는 **생산이 실제로 시작될 때**(대기 큐의 맨 앞으로 와서 라인이 비었을 때) 계산·기록된다 — 승인 시점(Phase 8)이 아니다. 승인 시점엔 "부족분(shortage_quantity)"만 알 수 있고, "지금 이 주문을 생산할 차례가 언제 오는지"는 큐 앞의 다른 주문들이 언제 끝나느냐에 달려 있기 때문이다.
- 세 필드 모두 `JsonOrderRepository` 직렬화 대상에 추가한다.

## `ProductionQueueProcessor` (신규 서비스 계층, View 없음)
```cpp
class ProductionQueueProcessor
{
    public:
        ProductionQueueProcessor(ISampleRepository& sample_repository, IOrderRepository& order_repository, IClock& clock);

        void advanceQueue();

    private:
        std::vector<Order> fetchProducingOrdersByFifo();   // status==PRODUCING, enqueued_at_millis 오름차순
};
```

**`advanceQueue()` 알고리즘 (핵심)**:
```
last_completion_millis = 없음(-1)

반복:
    producing = fetchProducingOrdersByFifo()
    없으면 종료

    active = producing[0]   // FIFO 맨 앞

    if active.production_start_millis == 0:   // 아직 생산 시작 전
        sample = sample_repository.findById(active.sample_id)
        real_qty = computeRealProductionQuantity(active.shortage_quantity, sample.yield)
        total_min = computeTotalProductionTimeMin(real_qty, sample.avg_production_time_min)

        // 시작 시각: 방금 이 루프에서 앞 주문을 끝냈다면 그 종료 시각을 이어받고(끊김 없는 순차 처리),
        // 아니라면(앞 주문이 이미 이전에 다 처리돼 있던 경우) 현재 시각을 시작 시각으로 한다.
        start = (last_completion_millis 있음) ? last_completion_millis : clock.nowMillis()

        active.real_production_quantity = real_qty
        active.production_start_millis = start
        active.production_end_millis = start + total_min을 ms로 환산
        order_repository.save(active)

        if active.production_end_millis > clock.nowMillis(): 종료   // 방금 시작했고 아직 안 끝남

    if clock.nowMillis() < active.production_end_millis:
        종료   // 아직 생산 중

    // 완료 처리
    sample = sample_repository.findById(active.sample_id)
    sample.physical_stock += active.real_production_quantity   // 전량 정상 판정 (PRD 6.6 결정)
    sample_repository.save(sample)

    active.status = CONFIRMED
    order_repository.save(active)

    last_completion_millis = active.production_end_millis
    // 반복 계속 → 다음 대기 주문을 확인/시작 (재기동 시 여러 건 연쇄 완료 처리)
```
- **왜 "이어받기(백데이팅)"가 필요한가**: 프로그램이 오래 꺼져있다가 켜졌을 때, 큐에 주문이 3개 있고 셋 다 이미 완료됐어야 하는 상황을 생각해보자. 첫 번째를 완료 처리한 뒤 두 번째를 시작 시각을 "지금"으로 잡아버리면, 두 번째·세 번째의 완료 시각이 실제보다 뒤로 밀려버린다. 앞 주문의 종료 시각을 다음 주문의 시작 시각으로 그대로 이어받아야, 이번 `advanceQueue()` 한 번의 호출 안에서 밀려있던 완료를 전부 정확한 시점에 처리할 수 있다.
- 이 함수는 **부작용(재고/주문 저장)이 있는 상태 전이 함수**이며, 몇 번을 호출해도 같은 결과를 유지하도록(멱등) 설계되어 있다 — 이미 완료된 주문은 `PRODUCING` 목록에서 빠지므로 중복 처리되지 않는다.

## `MainController` 연동
- 생성자에 `ProductionQueueProcessor* production_queue_processor = nullptr`를 **추가 trailing default 파라미터**로 넣는다 (기존 호출/테스트 호환 유지).
- `run()`의 매 루프 시작 시(메뉴 그리기 직전) `production_queue_processor`가 있으면 `advanceQueue()`를 호출한다. 이렇게 하면 사용자가 어떤 메뉴에 있든 메인 메뉴로 돌아올 때마다 생산 상태가 최신으로 갱신된다.
- 생성자에 `ISubMenuController* production_menu = nullptr`도 추가(다섯 번째 서브메뉴), `processCommand("5")`에서 위임한다.

## `ProductionController` / `IProductionView` (읽기 전용, "새로고침 가능한" 반복 조회)
```cpp
struct ActiveProductionInfo
{
    Order order;
    std::string sample_name;
    int available_stock_at_approval;   // 승인 시점 재고 = order.quantity - order.shortage_quantity
    double yield;
    double total_production_time_min;  // computeTotalProductionTimeMin(...)
    int progress_percent;              // 0~100, 조회 시점 기준 (now-start)/(end-start)
};

struct QueuedProductionInfo
{
    Order order;
    std::string sample_name;
    int projected_real_production_quantity;   // 아직 미확정 — 조회 시점에 미리보기로 계산
    long long expected_completion_millis;     // 화면 표시 시점에 누적 계산 (아래 참고)
};

class IProductionView
{
    public:
        virtual ~IProductionView() = default;

        virtual void showLineStatus(bool is_running) = 0;   // 생산라인 현재 상태(RUNNING/IDLE)
        virtual void showActiveProduction(const std::optional<ActiveProductionInfo>& active) = 0;
        virtual void showQueue(const std::vector<QueuedProductionInfo>& queue) = 0;
        virtual void showMessage(const std::string& message) = 0;
};

class ProductionController : public ISubMenuController
{
    public:
        ProductionController(IProductionView& view, IInputReader& input_reader,
            ISampleRepository& sample_repository, IOrderRepository& order_repository,
            ProductionQueueProcessor& queue_processor, IClock& clock);

        void run() override;
        bool processCommand(const std::string& command);   // "R"/"r": 새로고침 계속, "0": 뒤로가기

    private:
        void display();   // advanceQueue() 호출 → 현재 상태/처리 중/대기 목록 표시
};
```

**`run()` — 새로고침 루프**:
```
반복:
    화면 클리어
    display()   // advanceQueue() 먼저 호출해 최신 상태 반영 후 조회
    "[R] 새로고침   [0] 뒤로가기 > " 표시
    command = 입력
    if command == "0": 반복 종료 (뒤로가기)
    // "R"/"r" 또는 그 외 입력 모두 다시 반복 → 화면이 클리어되고 최신 값으로 다시 그려짐
```
- **이 메뉴만 화면 클리어를 사용한다.** Phase 6~8에서는 "결과 메시지가 사용자가 읽기 전에 지워지는 문제" 때문에 서브메뉴 내부에서 클리어를 뺐지만, 생산라인 조회는 매번 갱신되는 "현재 상태 스냅샷"을 보여주는 대시보드 성격이라 정반대다 — 클리어하지 않으면 새로고침할 때마다 화면 아래로 같은 내용이 계속 쌓여서 오히려 보기 나빠진다. 따라서 여기서는 "새로고침 = 화면을 지우고 다시 그림"이 맞는 동작이다.
- `display()` 시작 시 `queue_processor.advanceQueue()`를 먼저 호출해 최신 상태를 반영한 뒤 조회한다. 그래서 "새로고침"을 누르면 시간이 흐른 만큼 진행률/완료 여부가 실제로 갱신된다.
- **생산라인 상태**: `PRODUCING` 주문이 하나라도 있으면 `RUNNING`, 없으면 `IDLE`.
- **현재 처리 중**(FIFO 맨 앞, 이미 `production_start_millis`가 기록된 주문): 주문번호 / 시료명 / 주문량(`order.quantity`) / 재고(`order.quantity - order.shortage_quantity`) → 부족(`order.shortage_quantity`) → 실생산량(`order.real_production_quantity`) / 수율 / 총 진행시간(`total_production_time_min`) / **진행률(%)** / 완료 예정 시각(`formatDateTime(production_end_millis)`)을 모두 보여준다.
  - 진행률 계산: `clamp(0, 100, (now - start) * 100 / (end - start))`. `end == start`(이론상 발생하지 않아야 함)인 경우 100으로 처리해 0-division을 방지한다.
  - 큐가 비어 있으면 "현재 생산 중인 항목이 없습니다."만 표시.
- **대기 중인 주문**: 나머지 `PRODUCING` 주문들을 FIFO 순서로 나열. 순서/주문번호/시료명/주문량/부족분/실생산량(미리보기)을 보여주고, "예상 완료 시각"은 화면 표시 시점에 다음과 같이 누적 계산한다 — 활성 주문의 `production_end_millis`부터 시작해서, 대기 목록을 순서대로 훑으며 각 주문의 `computeTotalProductionTimeMin(computeRealProductionQuantity(shortage, sample.yield), sample.avg_production_time_min)`을 누적한다. (이 값들은 저장하지 않고 조회 시점에만 계산 — 아직 시작 전이라 실제 생산량/시작시각이 확정되지 않았기 때문)
- 이 메뉴는 Phase 6~8과 달리 **매 새로고침마다 화면을 클리어한다** (위 "새로고침 루프" 참고).

## 출력 화면 예시
```
--------------------------------------------------------------
[5] 생산라인 조회   FIFO 방식   현재시각 2026-04-16 09:49:12
생산라인 1개 (단일 라인)   현재 상태: RUNNING

현재 처리 중
  주문번호   ORD-20260416-0038   시료   SiC 파워기판-6인치
  주문량   80 ea   재고   30 ea → 부족 50 ea → 실생산량 54 ea (수율 0.92 / 총 진행시간 49.0 min)
  진행   ██████████████░░░░░░  72%   완료 예정 2026-04-16 09:49:12

대기 중인 주문 (FIFO 순)
순서   주문번호            시료                     주문량   부족분   실생산량   예상완료
1      ORD-20260416-0040   산화막 웨이퍼-SiO2        150 ea   150 ea   163 ea     26/04/16 11:43
2      ORD-20260416-0043   SiC 파워기판-6인치        200 ea   170 ea   185 ea     26/04/16 14:28
3      ORD-20260416-0044   GaN 에피택셜-4인치        300 ea   80 ea    87 ea      26/04/16 15:02

[R] 새로고침   [0] 뒤로가기 >
```
(큐가 비어 있을 때)
```
--------------------------------------------------------------
[5] 생산라인 조회   FIFO 방식   현재시각 2026-04-16 09:49:12
생산라인 1개 (단일 라인)   현재 상태: IDLE

현재 생산 중인 항목이 없습니다.
대기 중인 주문이 없습니다.

[R] 새로고침   [0] 뒤로가기 >
```
- 진행률 바(`██░░`)는 `ConsoleUtil`에 `renderProgressBar(int percent, int width)` 같은 작은 헬퍼를 추가해 그린다 (예: `width=20`이면 `percent/5`칸을 `█`로, 나머지를 `░`로 채움).
- 타이틀 줄에 `IClock::nowMillis()` 기준 현재 시각(`formatDateTime`)을 표시한다(메인 메뉴의 "시스템 현황" 표기와 동일한 패턴).
- 대기 중인 주문 목록의 "예상완료" 컬럼은 실생산량 숫자와 너무 붙어 보이는 문제가 있어, 전용 폭(16칸)과 축약 포맷 `formatShortDateTime`(`YY/MM/DD HH:MM`, 초 단위 생략)을 사용한다. "현재 처리 중"의 완료 예정 시각은 표 형태가 아니라서 기존 `formatDateTime`(초 포함) 그대로 유지한다.

## 테스트 계획 (gmock)
`ProductionQueueProcessorTest` (신규, 이번 Phase의 핵심 테스트):
- **아직 시간이 안 지남**: `production_start_millis == 0`인 주문에 대해 `advanceQueue()` 호출 → 시작 시각/완료 예정 시각이 기록되지만 상태는 여전히 `PRODUCING`, `physical_stock` 변화 없음
- **완료 시각이 지남**: 이미 시작된 주문의 `production_end_millis`를 목 시계가 지나도록 설정 → `advanceQueue()` 호출 시 `physical_stock`에 `real_production_quantity`만큼 반영, 상태 `CONFIRMED`로 전환
- **재기동 복원(연쇄 완료)**: 큐에 2건 이상 있고 목 시계가 둘 다 완료됐어야 할 시점으로 설정 → 한 번의 `advanceQueue()` 호출로 둘 다 순서대로 완료 처리되는지, 두 번째 주문의 시작 시각이 첫 번째의 완료 시각을 정확히 이어받는지 검증
- **생산 중에는 재고 미반영**: 시작은 했지만 아직 완료 시각 전인 주문 → `physical_stock`/`available_stock` 모두 변화 없음
- `ProductionControllerTest` (신규): 현재 처리 중 항목 표시(진행률 계산 포함 — 예: 시작~완료 구간의 50% 지점에서 목 시계를 고정하면 `progress_percent == 50`), 대기 목록 누적 완료 시각 계산, 빈 큐 메시지, `"R"` 입력 시 새로고침 반복(`advanceQueue()` 재호출 확인), `"0"` 입력 시 뒤로가기(루프 종료)
- `MainControllerTest`에 `production_menu` 위임 케이스 + `production_queue_processor.advanceQueue()`가 매 루프마다 호출되는지 검증하는 케이스 추가

## 완료 기준
- 생산 중 프로그램을 종료하고, 완료 시각이 지난 뒤 재실행하면 자동으로 완료 처리(재고 반영 + 상태 전환)되는지 확인 (테스트에서는 `IClock`을 목킹해 시간 경과를 시뮬레이션 — 실제로 몇 분씩 기다리지 않는다)
- 생산 진행 중에는 재고가 반영되지 않고, 완료 후에만 반영되는지 확인
- 생산 진행 중 조회 시 진행률(%)이 그 시점의 실제 경과 시간과 일치하게 계산되는지 확인
- 여러 건이 밀려 있을 때 재기동 시 순서대로, 시간 순서가 꼬이지 않고 전부 처리되는지 확인
- gmock 테스트 전체 통과, 로컬 빌드/실행으로 실제 콘솔 흐름 검증 (단, 실제 완료까지는 평균 생산시간을 짧게(예: `0.01`min = 0.6초) 설정한 시료로 테스트하면 실제 대기 없이도 빠르게 확인 가능)

## 다음 Phase로 이월되는 항목
- 출고 처리(`CONFIRMED` → `RELEASED`, 이때 비로소 `physical_stock` 차감) → Phase 10
- 상태별 주문 수 / 재고 현황 종합 모니터링 화면 → Phase 11 (참고: 메인 메뉴 "4번"이 모니터링이며, Phase 9는 "5번" 생산라인 조회임)

## 구현 결과 (완료)
- 설계 그대로 구현됨. `Order`에 `real_production_quantity`/`production_start_millis`/`production_end_millis` 필드 추가, `JsonOrderRepository`의 `fromJson`/`toJson` 갱신.
- `production/ProductionQueueProcessor.h/.cpp` 신설 — 설계된 백데이팅 체인 알고리즘 그대로 구현(`fetchProducingOrdersByFifo()` → 미시작 주문 시작 처리 → 완료 판정 → 반복).
- `console/ConsoleUtil.h/.cpp`에 `renderProgressBar(int percent, int width)` 추가 (UTF-8 이스케이프 시퀀스로 `█`/`░` 렌더링, 소스 인코딩 이슈 방지).
- `view/IProductionView.h`(`ActiveProductionInfo`/`QueuedProductionInfo`), `view/ConsoleProductionView.h/.cpp`, `controller/ProductionController.h/.cpp` 신규 구현 — "출력 화면 예시" 목업 그대로 구현(진행률 바, FIFO 대기 목록 누적 완료 시각 계산 포함).
- `MainController`에 `production_menu`(다섯 번째 trailing default)와 `ProductionQueueProcessor*`(매 루프 `advanceQueue()` 호출) 연결. `processCommand("5")`가 생산라인 메뉴로 위임되도록 분리(기존엔 "4"/"5"/"6"이 한꺼번에 플레이스홀더 처리됐었음).
- `main.cpp`에 `ProductionQueueProcessor`/`ConsoleProductionView`/`ProductionController` 인스턴스 배선.
- gmock 테스트 61개 전체 통과 (`ProductionQueueProcessorTest` 4개: 미시작 주문 시작 처리, 완료 시 재고 반영, 재기동 백데이팅 체인, 생산 중 재고 미반영 / `ProductionControllerTest` 5개: 진행률 계산, 빈 큐 메시지, FIFO 대기 목록 누적 완료 시각, 새로고침 반복, 뒤로가기 / `MainControllerTest` 신규 3개: production_menu 위임, 플레이스홀더, 매 루프 advanceQueue 호출 검증).
  - **테스트 작성 시 주의점(버그 발견 및 수정)**: `ProductionQueueProcessorTest`/`ProductionControllerTest`에서 `order_repository.findAll()`을 정적(static) `ON_CALL(...).WillByDefault(Return(...))`으로 고정해두면, `advanceQueue()`가 주문을 완료 처리한 뒤 같은 루프 안에서 다시 `findAll()`을 호출할 때도 "완료 전" 상태의 주문이 그대로 조회되어 **`advanceQueue()`의 `while(true)` 루프가 종료되지 않는(무한 루프) 테스트 행(hang)** 이 발생했다. 완료로 이어지는 시나리오는 `save()` 호출 시 벡터를 실제로 갱신하는 `Invoke` 기반 스테이트풀 목(mock)으로, 완료가 필요 없는 시나리오는 `production_end_millis`를 목 시계보다 미래로 설정해 애초에 완료되지 않게 하는 방식으로 수정했다.
- 실제 실행(수동 스모크 테스트)으로 검증: 평균 생산시간 `0.01`min(약 0.6초)인 시료를 등록 → 재고 0 상태에서 10개 주문 → 재고 부족으로 승인 시 `PRODUCING` 전환(실생산량 10, 총 진행시간 0.1min) → 생산라인 조회 시 진행률 0%로 시작 확인 → 약 7초 대기 후 새로고침("R") → `IDLE` 상태로 전환되고 메인 메뉴 총 재고가 0 → 10 ea로 반영됨을 확인.
