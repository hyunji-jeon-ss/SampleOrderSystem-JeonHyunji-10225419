# Phase 8 설계 문서 — 주문 승인/거절 기능 (재고 선점 로직 포함)

`PLAN.md` Phase 8, `PRD.md` 6.4에 대응. 접수된 주문(RESERVED)을 조회하고 승인/거절 처리하며, 재고 이중 관리(availableStock/physicalStock)의 핵심 로직을 구현한다.

## 목표
- `RESERVED` 상태 주문 목록을 조회한다.
- 주문을 선택해 **승인** 또는 **거절**한다.
- 승인 시, **승인 시점의 내부 가용 재고**로 충분/부족을 판단해 자동 분기한다.
  - 충분 → `CONFIRMED` 전환 + 내부 가용 재고 즉시 차감(화면 표시 재고는 그대로 유지)
  - 부족 → `PRODUCING` 전환 + 내부 가용 재고를 남은 만큼만 차감(0 하한) + 부족분을 주문에 기록(Phase 9가 사용)
- 거절 시 `REJECTED` 전환, 재고는 내부/화면 모두 변화 없음.
- 여러 건을 연속으로 처리할 수 있도록 **목록을 반복해서 보여주는 루프** 형태로 만든다 (Phase 6 `SampleController`와 동일한 패턴).

## 메뉴 흐름 (루프형 — `SampleController`와 동일 패턴)
```
[3] 주문 승인/거절 선택
  → RESERVED 주문 목록 표시 (번호, 주문번호, 고객, 시료명, 수량)
  → 번호 입력
      → "0": 뒤로가기 (메인 메뉴로 복귀)
      → 유효하지 않은 번호: 오류 메시지 → 목록 다시 표시
      → 유효한 번호: 해당 주문의 재고 확인 화면 표시
          시료   {시료명} ({시료ID})
          현재 재고(가용)   {available_stock} ea
          주문 수량         {quantity} ea
          [재고 충분 시] "재고가 충분합니다. 승인하시겠습니까?"
          [재고 부족 시] "재고 부족. 부족분 {shortage} ea → 실생산량 {ceil(shortage/yield)} ea (예상 {실생산량 * 평균생산시간} min) 이대로 승인(생산 등록)하시겠습니까?"
          [Y] 승인   [N] 거절
      → 승인 확정("Y"/"y")
          - 재고 충분: available_stock -= quantity, 주문 상태 CONFIRMED
          - 재고 부족: shortage = quantity - available_stock; available_stock = 0; 주문 상태 PRODUCING, shortage_quantity/enqueued_at_millis 기록
          - 결과 표시 후 목록으로 복귀(루프 계속)
      → 거절("N"/"n"/그 외)
          - 주문 상태 REJECTED (재고 변화 없음)
          - 결과 표시 후 목록으로 복귀(루프 계속)
```
루프 안에서 매번 목록이 갱신되어 표시되므로, Phase 6/7에서 겪은 "결과 메시지가 사라지는" 문제와 무관하다 (SampleController와 동일하게 이 메뉴에서도 화면 클리어를 사용하지 않는다).

## 도메인 모델 변경: `Order`에 필드 추가
```cpp
struct Order
{
    std::string order_number;
    std::string sample_id;
    std::string customer_name;
    int quantity = 0;
    OrderStatus status = OrderStatus::RESERVED;
    int shortage_quantity = 0;        // PRODUCING 전환 시 부족분(개). Phase 9가 실생산량 계산에 사용.
    long long enqueued_at_millis = 0; // PRODUCING 전환(승인) 시각. Phase 9의 생산 큐 FIFO 정렬 기준.
};
```
- `shortage_quantity`/`enqueued_at_millis`은 `PRODUCING`이 아닌 주문에서는 `0`으로 유지된다.
- 두 필드 모두 `JsonOrderRepository`의 직렬화 대상에 추가한다.
- **왜 지금 추가하나**: 승인 시점에 계산한 부족분은 이 시점이 아니면 복원할 수 없다 (내부 가용 재고는 이미 0으로 깎여서, 나중에 다시 계산할 수 없음). Phase 9가 이 값을 그대로 읽어 실생산량을 계산하도록 Phase 8에서 미리 기록해둔다.

## 공용 계산 유틸: `ProductionCalculator`
승인 화면의 "예상 실생산량/소요시간" 미리보기와, Phase 9의 실제 생산 처리가 **동일한 공식**을 써야 하므로 지금 공용 함수로 뺀다.
```cpp
// production/ProductionCalculator.h
int computeRealProductionQuantity(int shortage_quantity, double yield);       // ceil(shortage / yield)
double computeTotalProductionTimeMin(int real_production_quantity, double avg_production_time_min);
```

## `ApprovalController` 설계
```cpp
struct ApprovalOrderRow
{
    Order order;
    std::string sample_name;
};

class ApprovalController : public ISubMenuController
{
    public:
        ApprovalController(IApprovalView& view, IInputReader& input_reader,
            ISampleRepository& sample_repository, IOrderRepository& order_repository, IClock& clock);

        void run() override;
        bool processCommand(const std::string& command);

    private:
        std::vector<ApprovalOrderRow> buildReservedRows();
        void handleApprovalFlow(const ApprovalOrderRow& row);

        std::vector<ApprovalOrderRow> current_rows;   // 마지막으로 표시한 목록 (번호 선택 시 참조)
        ...
};
```
- `current_rows`는 매 루프 반복(`run()`)마다 새로 채워지고, `processCommand`가 번호를 이 목록의 인덱스로 해석한다. (`SampleController`에는 없던 상태지만, 목록 기반 선택 UI라 필요함 — `MainController`/`SampleController`/`OrderController`와 달리 상태를 하나 더 가진다는 점이 이 컨트롤러의 특징이다.)

## `IApprovalView` / `ConsoleApprovalView`
```cpp
class IApprovalView
{
    public:
        virtual ~IApprovalView() = default;

        virtual void showReservedList(const std::vector<ApprovalOrderRow>& rows) = 0;
        virtual void showSufficientStockCheck(const ApprovalOrderRow& row, int available_stock) = 0;
        virtual void showInsufficientStockCheck(const ApprovalOrderRow& row, int available_stock,
            int shortage_quantity, int real_production_quantity, double total_production_time_min) = 0;
        virtual void showApprovalResult(const Order& order) = 0;   // 상태(CONFIRMED/PRODUCING)에 따라 다르게 표기
        virtual void showRejection(const Order& order) = 0;
        virtual void showMessage(const std::string& message) = 0;
};
```
- 화면 클리어 없음 (Phase 6/7과 동일 원칙).
- "현재 상태" 표기에는 Phase 7에서 만든 `orderStatusToString()`을 재사용한다.

## 재고 이중 관리 핵심 로직 (`handleApprovalFlow` 의사코드)
```cpp
void ApprovalController::handleApprovalFlow(const ApprovalOrderRow& row)
{
    Sample sample = *sample_repository.findById(row.order.sample_id);   // Phase 7에서 존재 보장됨

    if (sample.available_stock >= row.order.quantity)
    {
        view.showSufficientStockCheck(row, sample.available_stock);
    }
    else
    {
        const int shortage = row.order.quantity - sample.available_stock;
        const int real_qty = computeRealProductionQuantity(shortage, sample.yield);
        const double total_min = computeTotalProductionTimeMin(real_qty, sample.avg_production_time_min);
        view.showInsufficientStockCheck(row, sample.available_stock, shortage, real_qty, total_min);
    }

    const std::string confirm = input_reader.readLine();
    if (confirm != "Y" && confirm != "y")
    {
        Order rejected = row.order;
        rejected.status = OrderStatus::REJECTED;
        order_repository.save(rejected);
        view.showRejection(rejected);
        return;
    }

    Order updated = row.order;
    if (sample.available_stock >= row.order.quantity)
    {
        sample.available_stock -= row.order.quantity;
        updated.status = OrderStatus::CONFIRMED;
    }
    else
    {
        const int shortage = row.order.quantity - sample.available_stock;
        sample.available_stock = 0;
        updated.status = OrderStatus::PRODUCING;
        updated.shortage_quantity = shortage;
        updated.enqueued_at_millis = clock.nowMillis();
    }

    sample_repository.save(sample);
    order_repository.save(updated);
    view.showApprovalResult(updated);
}
```
**핵심 불변식(테스트로 검증할 것)**: 이 함수가 끝나기 전까지 `sample.physical_stock`은 절대 건드리지 않는다 — 화면 표시 재고는 오직 Phase 9(생산 완료) / Phase 10(출고)에서만 변한다.

## `MainController` 연동
- 생성자에 `ISubMenuController* approval_menu = nullptr`를 **네 번째 trailing default 파라미터**로 추가 (`sample_menu`, `order_menu` 다음).
- `processCommand("3")`: `approval_menu`가 있으면 `approval_menu->run()`, 없으면 기존 placeholder.

## 테스트 계획 (gmock)
`ApprovalControllerTest` 신규:
- **재고 충분 승인** → `sample.available_stock`이 주문 수량만큼 차감된 상태로 저장, `order.status == CONFIRMED`, `physical_stock`은 변화 없음
- **재고 부족 승인** → `sample.available_stock == 0`으로 저장(음수 아님), `order.status == PRODUCING`, `order.shortage_quantity == quantity - 원래 available_stock`, `physical_stock`은 변화 없음
- **거절** → `order.status == REJECTED`, `sample_repository.save()` 호출 안 됨(재고 변화 없음)
- **연속 승인 시 재고 중복 사용 방지**: 동일 시료에 대해 순차적으로 주문 2건을 승인할 때, 두 번째 주문이 첫 번째가 이미 차감한 `available_stock`을 다시 사용하지 않는지 검증 (완료 기준 핵심 항목)
- **잘못된 번호 입력** → 오류 메시지, 상태 변화 없음
- `computeRealProductionQuantity`/`computeTotalProductionTimeMin` 단위 테스트 (올림 계산 경계값 포함, 예: `shortage=170, yield=0.92` → `ceil(170/0.92)=185`)
- `MainControllerTest`에 `approval_menu` 위임 케이스 추가

## 완료 기준
- 승인 후 시료 조회(화면, `physical_stock`)에서 재고가 아직 차감되지 않은 것으로 보이는지 확인 (내부 가용 재고만 변함)
- 재고 부족으로 두 건 이상 순차 승인 시, 각 주문의 부족분이 서로 겹치지 않고 정확히 계산되는지 확인
- gmock 테스트 전체 통과, 로컬 빌드/실행으로 실제 콘솔 흐름(충분/부족/거절 각각) 검증

## 다음 Phase로 이월되는 항목
- `PRODUCING` 주문의 실제 생산 큐 처리, 실시간 타이머, 재기동 복원, `physical_stock` 반영 → Phase 9

## 구현 결과 (완료)
- 설계와 한 가지 차이: `ApprovalController`가 목록을 멤버 변수(`current_rows`)에 캐싱하지 않고, **`run()`과 `processCommand()` 양쪽에서 매번 `buildReservedRows()`를 새로 호출**하도록 단순화했다. 캐싱된 상태가 없어 테스트가 훨씬 단순해지고(각 `processCommand()` 호출이 독립적으로 완결됨), 목록이 항상 최신 `order_repository`/`sample_repository` 상태를 반영한다는 이점도 있다.
- `Order`에 `shortage_quantity`, `enqueued_at_millis` 필드 추가, `JsonOrderRepository` 직렬화에 반영.
- `production/ProductionCalculator.h/.cpp` 신설 (`computeRealProductionQuantity`, `computeTotalProductionTimeMin`), 승인 화면 미리보기에서 사용 — Phase 9에서 동일 함수 재사용 예정.
- `IApprovalView`/`ConsoleApprovalView`, `ApprovalController` 구현. `MainController`에 `approval_menu`(네 번째 trailing default) 연결.
- gmock 테스트 49개 전체 통과 (`ApprovalControllerTest` 6개, `ProductionCalculatorTest` 5개 신규 — 재고 충분/부족/거절, **연속 승인 시 재고 중복 사용 방지(핵심 검증)**, 잘못된 번호, 뒤로가기).
- 실제 실행으로 재고 30인 시료에 수량 20 주문 2건을 연속 승인 → 첫 주문은 재고 충분(CONFIRMED), 두 번째 주문은 화면에 "현재 재고(가용) 10 ea"로 정확히 표시되어 부족분 10으로 PRODUCING 전환됨을 확인. 승인 후 `samples.json`에서 `physical_stock`은 그대로, `available_stock`만 정확히 차감된 것도 확인.

## 커밋 이후 리팩터링 (코드 리뷰 반영)
Phase 8 완료 직후, `JsonSampleRepository`/`JsonOrderRepository`/`ConsoleSampleView`/`MainController`를 대상으로 clean code 관점 코드 리뷰를 진행해 아래를 정리했다 (기능 변경 없음, 49개 테스트 그대로 통과 확인):
- **Repository 파일 I/O 중복 제거**: 두 Repository에 거의 동일하게 있던 `ifstream`/`ofstream` 처리(빈 파일 체크, JSON 파싱/직렬화)를 `repository/JsonFileStore.h/.cpp`(`readJsonArray`/`writeJsonArray`)로 공용화.
- **버그 수정**: `ConsoleSampleView::showSampleMenu()`에서 `[1]`이 헤더와 메뉴 옵션에 중복 출력되던 것을 수정.
- **`MainController` 위임 분기 정리**: `if (menu) menu->run(); else placeholder;` 패턴이 3번 반복되던 것을 `runSubMenuOrShowPlaceholder()` 헬퍼로 통합.
