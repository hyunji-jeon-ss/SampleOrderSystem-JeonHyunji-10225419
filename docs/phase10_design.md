# Phase 10 설계 문서 — 출고 처리 기능

`PLAN.md` Phase 10, `PRD.md` 6.7에 대응. `CONFIRMED` 상태 주문(생산까지 끝나 화면 표시 재고가 확보된 주문)을 출고 처리하여 `RELEASED`로 전환하고, 이 시점에 비로소 **화면 표시 재고(`physical_stock`)**를 차감한다.

## 목표
- `CONFIRMED` 상태 주문 목록을 조회한다.
- 주문을 선택해 출고를 확정(Y/N)한다.
- 출고 확정 시: 주문 상태 `CONFIRMED → RELEASED` 전환 + `physical_stock`을 주문 수량(`order.quantity`)만큼 차감.
- **`available_stock`은 건드리지 않는다** — 이미 승인(Phase 8) 시점에 차감이 끝난 상태이기 때문. Phase 10은 오직 "화면에 보이던 재고가 실제로 나갔다"는 사실을 반영하는 단계다.
- 여러 건을 연속으로 처리할 수 있도록 Phase 8(`ApprovalController`)과 동일하게 **목록을 반복해서 보여주는 루프** 형태로 만든다.

## 왜 `order.quantity`를 차감하는가 (핵심 결정)
`physical_stock`에는 생산 완료 시(Phase 9) `real_production_quantity`(수율을 반영해 올림 계산한 실제 생산량, `order.quantity`보다 크거나 같음)가 전량 반영되어 있다. 그런데 고객에게 실제로 나가는 물량은 **주문한 수량(`order.quantity`)** 뿐이다 — 수율 보정으로 여분 생산된 만큼은 출고되지 않고 재고로 남는다 (불량 손실을 시뮬레이션하지 않는 대신, 여분은 그대로 다음 주문에 쓸 수 있는 재고로 남긴다는 뜻). 따라서 출고 시 차감량은 `real_production_quantity`가 아니라 **`order.quantity`**로 한다.
- 재고가 부족했던 적 없이 바로 `CONFIRMED`가 된 주문(승인 시점에 재고 충분)도 동일하게 `order.quantity`만큼 차감한다 — 이 경우 애초에 `real_production_quantity` 개념 자체가 없다(생산 큐를 거치지 않음).

## 메뉴 흐름 (루프형 — Phase 8 `ApprovalController`와 동일 패턴)
```
[6] 출고 처리 선택
  → CONFIRMED 주문 목록 표시 (번호, 주문번호, 고객, 시료명, 수량)
  → 번호 입력
      → "0": 뒤로가기 (메인 메뉴로 복귀)
      → 유효하지 않은 번호: 오류 메시지 → 목록 다시 표시
      → 유효한 번호: 해당 주문의 출고 확인 화면 표시
          시료          {시료명} ({시료ID})
          현재 재고(화면)  {physical_stock} ea
          출고 수량        {quantity} ea
          "출고 처리하시겠습니까?"
          [Y] 출고   [N] 취소
      → 출고 확정("Y"/"y")
          - physical_stock -= quantity (0 미만으로 내려가지 않아야 하지만, 정상 흐름상 quantity ≤ physical_stock이 항상 보장됨 — 아래 불변식 참고)
          - 주문 상태 RELEASED
          - 결과 표시 후 목록으로 복귀(루프 계속)
      → 취소("N"/"n"/그 외)
          - 아무 것도 변경하지 않고(주문 상태 유지, 재고 유지) 목록으로 복귀(루프 계속)
```
루프 안에서 매번 목록이 갱신되어 표시되므로 Phase 6/8과 동일하게 이 메뉴에서도 **화면 클리어를 사용하지 않는다.**

## 왜 "재고 부족" 분기가 없는가
Phase 8(승인)에서 `CONFIRMED`가 되는 시점은 항상 승인 시점 가용 재고가 충분했을 때뿐이고, Phase 9(생산)에서 `PRODUCING → CONFIRMED`로 전환되는 시점은 생산이 실제로 완료되어 `physical_stock`에 `real_production_quantity`(≥ `order.quantity`가 되도록 수율 보정 후 올림 계산됨)가 이미 반영된 뒤다. 즉 **`CONFIRMED` 상태에 도달했다는 것 자체가 "이 주문 몫의 물량은 이미 화면 재고에 존재한다"는 불변식을 보장**한다. 따라서 출고 화면에는 승인 화면(Phase 8)처럼 "충분/부족" 분기가 필요 없다 — 항상 출고 가능하다.

## `IReleaseView` / `ConsoleReleaseView`
```cpp
struct ReleaseOrderRow
{
    Order order;
    std::string sample_name;
};

class IReleaseView
{
    public:
        virtual ~IReleaseView() = default;

        virtual void showConfirmedList(const std::vector<ReleaseOrderRow>& rows) = 0;
        virtual void showReleaseCheck(const ReleaseOrderRow& row, int physical_stock) = 0;
        virtual void showReleaseResult(const Order& order) = 0;
        virtual void showCancellation(const Order& order) = 0;
        virtual void showMessage(const std::string& message) = 0;
};
```
- `ApprovalOrderRow`/`IApprovalView`와 거의 같은 모양이지만, "충분/부족" 두 갈래였던 확인 화면이 `showReleaseCheck` 하나로 단순화된다(위 불변식 덕분에 분기가 필요 없음).
- 화면 클리어 없음. "현재 상태" 표기에는 `orderStatusToString()` 재사용.

## `ReleaseController` 설계
```cpp
class ReleaseController : public ISubMenuController
{
    public:
        ReleaseController(IReleaseView& view, IInputReader& input_reader,
            ISampleRepository& sample_repository, IOrderRepository& order_repository);

        void run() override;
        bool processCommand(const std::string& command);

    private:
        std::vector<ReleaseOrderRow> buildConfirmedRows();
        void handleReleaseFlow(const ReleaseOrderRow& row);
};
```
- Phase 8 구현 결과에서 확정된 패턴을 그대로 따른다: 목록을 멤버 변수에 캐싱하지 않고, `run()`/`processCommand()` 양쪽에서 매번 `buildConfirmedRows()`를 새로 호출한다 (항상 최신 상태 반영, 테스트 단순화).
- `IClock`은 필요 없다 — 출고 처리는 시각을 기록하지 않는다(FIFO 큐 진입 시각처럼 나중에 쓰이는 타임스탬프가 없음).

## 출고 처리 핵심 로직 (`handleReleaseFlow` 의사코드)
```cpp
void ReleaseController::handleReleaseFlow(const ReleaseOrderRow& row)
{
    Sample sample = *sample_repository.findById(row.order.sample_id);   // Phase 7에서 존재 보장됨

    view.showReleaseCheck(row, sample.physical_stock);

    const std::string confirm = input_reader.readLine();
    if (confirm != "Y" && confirm != "y")
    {
        view.showCancellation(row.order);   // 상태/재고 변화 없음
        return;
    }

    sample.physical_stock -= row.order.quantity;
    sample_repository.save(sample);

    Order updated = row.order;
    updated.status = OrderStatus::RELEASED;
    order_repository.save(updated);

    view.showReleaseResult(updated);
}
```
**핵심 불변식(테스트로 검증할 것)**: 이 함수는 `sample.available_stock`을 절대 건드리지 않는다 — 이미 승인 시점(Phase 8)에 반영이 끝났기 때문이다.

## `MainController` 연동
- 생성자에 `ISubMenuController* release_menu = nullptr`를 **여섯 번째 trailing default 파라미터**로 추가 (`sample_menu`, `order_menu`, `approval_menu`, `production_menu`, `ProductionQueueProcessor*` 다음).
- `processCommand("6")`: `release_menu`가 있으면 `release_menu->run()`, 없으면 기존 placeholder.

## 테스트 계획 (gmock)
`ReleaseControllerTest` 신규:
- **정상 출고** → `sample.physical_stock`이 `order.quantity`만큼 차감된 상태로 저장, `order.status == RELEASED`, `available_stock`은 변화 없음
- **수율 보정으로 여분이 남은 주문의 출고** → 생산 완료로 `physical_stock`에 `real_production_quantity`(> `order.quantity`)가 반영되어 있던 주문을 출고할 때, 차감량이 `real_production_quantity`가 아니라 정확히 `order.quantity`인지 검증 (핵심 검증)
- **취소("N")** → 주문 상태 변화 없음(여전히 `CONFIRMED`), `sample_repository.save()` 호출 안 됨(재고 변화 없음)
- **잘못된 번호 입력** → 오류 메시지, 상태 변화 없음
- **목록이 `CONFIRMED` 상태만 걸러서 보여주는지 확인** (`RESERVED`/`PRODUCING`/`REJECTED`/`RELEASED` 주문은 제외)
- `MainControllerTest`에 `release_menu` 위임 케이스 추가

## 완료 기준
- 출고 처리 후 화면 표시 재고(`physical_stock`)가 `order.quantity`만큼 정확히 감소하는지 확인
- 생산 큐를 거쳐(Phase 9) 수율 보정된 실생산량이 반영된 주문도, 출고 시 차감량은 항상 `order.quantity` 기준임을 확인
- 출고 후 `available_stock`은 전혀 변하지 않는지 확인 (이미 승인 시점에 반영 완료)
- gmock 테스트 전체 통과, 로컬 빌드/실행으로 실제 콘솔 흐름(정상 출고/취소) 검증

## 다음 Phase로 이월되는 항목
- 상태별 주문 건수 집계, 시료별 재고 현황(화면 표시 재고 기준) 통합 모니터링 화면 → Phase 11
