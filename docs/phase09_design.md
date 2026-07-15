# Phase 9 설계 문서 — 생산라인 기능 (실시간 처리 + 재기동 복원)

`PLAN.md` Phase 9, `PRD.md` 6.6에 대응. `PRODUCING` 상태 주문을 단일 생산 라인(FIFO)에서 실시간으로 처리하고, 완료 시 화면 표시 재고(`physical_stock`)에 반영한다. 프로그램이 꺼져 있던 동안 완료됐어야 할 생산도 재기동 시 정확히 복원한다.

> **PDF 예시와의 차이**: PDF의 생산라인 화면에는 진행률 바(`진행 ████░░ 72%`)와 잔여율이 있지만, **이번 설계에서는 진행률/잔여율 표시를 제외한다.** 이 시스템은 백그라운드 스레드 없이 "메뉴에 접근할 때마다 현재 시각과 비교해서 상태를 갱신"하는 폴링 방식이라(아래 참고), 매끄러운 진행률 바를 보여주려면 별도의 실시간 갱신 루프가 필요한데 이는 PRD 완료 기준에 없는 범위다. 대신 **완료 예정 시각(절대 시간)**을 보여줘서 사용자가 언제 끝나는지는 알 수 있게 한다.

## 목표
- 재고 부족으로 `PRODUCING` 전환된 주문들을 하나의 생산 라인이 FIFO로 순차 처리한다.
- 생산 시간은 현실 시간 그대로 흐른다(배율 없음). 완료 시점에만 `physical_stock`에 실생산량 전량을 반영한다.
- 프로그램을 껐다 켜도, 꺼져 있던 동안 완료됐어야 할 생산은 재기동 시 자동으로 완료 처리된다(여러 건이 밀려 있어도 순서대로 전부 처리).
- 생산 현황(현재 처리 중 + 대기 목록)을 조회할 수 있다.

## 핵심 설계 결정: "폴링 기반" 실시간 처리
콘솔 앱은 사용자의 입력을 기다리는 동안만 실행되는 동기 프로그램이라, 백그라운드 타이머 스레드를 두지 않는다. 대신:

- **생산 큐 전진 로직(`ProductionQueueProcessor::advanceQueue()`)을 "메뉴가 그려질 때마다" 호출**해서, 그 시점의 실제 시각과 저장된 완료 예정 시각을 비교해 상태를 갱신한다.
- 호출 지점: ① `MainController`가 메인 메뉴를 그리기 직전(매 루프 반복마다, "생산라인 N건 대기" 카운트가 항상 최신 상태이도록), ② `ProductionController`(생산라인 조회 메뉴) 진입 시.
- 이 방식은 진행률(%)을 부드럽게 보여줄 수는 없지만(그래서 위에서 잔여율을 제외했다), **완료 여부 판정 자체는 실제 경과 시간과 정확히 일치**한다 — 사용자가 언제 메뉴를 열어보든, 그 시점 기준으로 이미 지난 완료 시각은 즉시 완료 처리되기 때문이다.

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

## `ProductionController` / `IProductionView` (읽기 전용, 1회성 조회 — `OrderController`와 동일한 "표시 후 뒤로가기" 패턴)
```cpp
struct ActiveProductionInfo
{
    Order order;
    std::string sample_name;
};

struct QueuedProductionInfo
{
    Order order;
    std::string sample_name;
    long long expected_completion_millis;   // 화면 표시 시점에 누적 계산 (아래 참고)
};

class IProductionView
{
    public:
        virtual ~IProductionView() = default;

        virtual void showActiveProduction(const std::optional<ActiveProductionInfo>& active) = 0;
        virtual void showQueue(const std::vector<QueuedProductionInfo>& queue) = 0;
        virtual void showMessage(const std::string& message) = 0;
};

class ProductionController : public ISubMenuController
{
    public:
        ProductionController(IProductionView& view, IInputReader& input_reader,
            ISampleRepository& sample_repository, IOrderRepository& order_repository,
            ProductionQueueProcessor& queue_processor);

        void run() override;   // advanceQueue() → 현재 처리 중 + 대기 목록 표시 → "[Enter] 뒤로가기" 대기
};
```
- `run()` 시작 시 `queue_processor.advanceQueue()`를 먼저 호출해 최신 상태를 반영한 뒤 조회한다.
- **현재 처리 중**: FIFO 맨 앞(가장 먼저 `enqueued_at_millis`를 가진 `PRODUCING` 주문)을 표시. 주문번호/시료명/주문수량/부족분/실생산량/완료 예정 시각(`formatDateTime(production_end_millis)`)을 보여준다. 진행률(%)은 표시하지 않는다(위 결정 참고). 큐가 비어 있으면 "현재 생산 중인 항목이 없습니다."
- **대기 중인 주문**: 나머지 `PRODUCING` 주문들을 FIFO 순서로 나열. 각 항목의 "예상 완료 시각"은 화면 표시 시점에 다음과 같이 누적 계산한다 — 활성 주문의 `production_end_millis`부터 시작해서, 대기 목록을 순서대로 훑으며 각 주문의 `computeTotalProductionTimeMin(computeRealProductionQuantity(shortage, sample.yield), sample.avg_production_time_min)`을 누적한다. (이 값은 저장하지 않고 조회 시점에만 계산 — 아직 시작 전이라 실제 생산량/시작시각이 확정되지 않았기 때문)
- 화면 클리어 없음 (Phase 6~8과 동일 원칙).

## 테스트 계획 (gmock)
`ProductionQueueProcessorTest` (신규, 이번 Phase의 핵심 테스트):
- **아직 시간이 안 지남**: `production_start_millis == 0`인 주문에 대해 `advanceQueue()` 호출 → 시작 시각/완료 예정 시각이 기록되지만 상태는 여전히 `PRODUCING`, `physical_stock` 변화 없음
- **완료 시각이 지남**: 이미 시작된 주문의 `production_end_millis`를 목 시계가 지나도록 설정 → `advanceQueue()` 호출 시 `physical_stock`에 `real_production_quantity`만큼 반영, 상태 `CONFIRMED`로 전환
- **재기동 복원(연쇄 완료)**: 큐에 2건 이상 있고 목 시계가 둘 다 완료됐어야 할 시점으로 설정 → 한 번의 `advanceQueue()` 호출로 둘 다 순서대로 완료 처리되는지, 두 번째 주문의 시작 시각이 첫 번째의 완료 시각을 정확히 이어받는지 검증
- **생산 중에는 재고 미반영**: 시작은 했지만 아직 완료 시각 전인 주문 → `physical_stock`/`available_stock` 모두 변화 없음
- `ProductionControllerTest` (신규): 현재 처리 중 항목 표시, 대기 목록 누적 완료 시각 계산, 빈 큐 메시지, "[Enter] 뒤로가기" 대기
- `MainControllerTest`에 `production_menu` 위임 케이스 + `production_queue_processor.advanceQueue()`가 매 루프마다 호출되는지 검증하는 케이스 추가

## 완료 기준
- 생산 중 프로그램을 종료하고, 완료 시각이 지난 뒤 재실행하면 자동으로 완료 처리(재고 반영 + 상태 전환)되는지 확인 (테스트에서는 `IClock`을 목킹해 시간 경과를 시뮬레이션 — 실제로 몇 분씩 기다리지 않는다)
- 생산 진행 중에는 재고가 반영되지 않고, 완료 후에만 반영되는지 확인
- 여러 건이 밀려 있을 때 재기동 시 순서대로, 시간 순서가 꼬이지 않고 전부 처리되는지 확인
- gmock 테스트 전체 통과, 로컬 빌드/실행으로 실제 콘솔 흐름 검증 (단, 실제 완료까지는 평균 생산시간을 짧게(예: `0.01`min = 0.6초) 설정한 시료로 테스트하면 실제 대기 없이도 빠르게 확인 가능)

## 다음 Phase로 이월되는 항목
- 출고 처리(`CONFIRMED` → `RELEASED`, 이때 비로소 `physical_stock` 차감) → Phase 10
- 상태별 주문 수 / 재고 현황 종합 모니터링 화면 → Phase 11 (참고: 메인 메뉴 "4번"이 모니터링이며, Phase 9는 "5번" 생산라인 조회임)
