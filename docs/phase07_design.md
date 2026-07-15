# Phase 7 설계 문서 — 시료 주문(예약) 기능

`PLAN.md` Phase 7, `PRD.md` 6.3에 대응. 고객의 시료 요청을 주문(예약)으로 등록하는 기능을 구현한다.

> 이 문서는 1차 설계를 사용자 피드백에 따라 재설계한 버전이다. 핵심 변경: 저장 전에 **입력 내용 확인(Y/N) 단계**를 추가하고, 확인/취소 각 경우에 대해 명확한 결과 화면과 "뒤로가기"(메인 메뉴 복귀) 동작을 정의했다.

## 목표
- 시료 ID / 고객명 / 주문 수량을 입력받는다.
- 저장하기 전에 **입력 내용을 다시 보여주고 Y/N으로 확인**받는다 (PDF 예시 UI와 동일한 흐름).
- `Y` → 주문을 `RESERVED` 상태로 저장하고, **주문번호와 현재 상태**를 출력한다.
- `N`(또는 그 외 입력) → 저장하지 않고 취소하며, 그대로 메인 메뉴로 돌아갈 수 있다("뒤로가기").
- 등록되지 않은 시료 ID로는 애초에 주문 확인 단계까지 가지 못하도록 막는다.

## 전체 흐름
```
[2] 시료 주문 선택
  → 시료 ID 입력
      → 존재하지 않는 ID면: 오류 메시지 → 즉시 메인 메뉴로 복귀 (뒤로가기)
  → 고객명 입력
  → 주문 수량 입력
      → 파싱 실패 또는 1 미만이면: 오류 메시지 → 즉시 메인 메뉴로 복귀 (뒤로가기)
  → [입력 내용 확인 화면]
      시료   {시료명} ({시료ID})
      고객   {고객명}
      수량   {수량} ea
      [Y] 예약 접수   [N] 취소
  → 응답이 Y/y
      → 주문 저장 (RESERVED, 주문번호 자동 채번)
      → "예약 접수 완료." + 주문번호 + 현재 상태(RESERVED) 출력
      → 메인 메뉴로 복귀 (뒤로가기)
  → 응답이 N/n 또는 그 외
      → "주문이 취소되었습니다." 출력, 저장하지 않음
      → 메인 메뉴로 복귀 (뒤로가기)
```
어느 경로든 `OrderController::run()`이 반환되면 `MainController`가 다시 메인 메뉴를 표시하므로, 별도의 "뒤로가기" 명령을 따로 구현할 필요는 없다 — **처리(완료/취소/오류) 후 자동으로 메인 메뉴로 돌아가는 것 자체가 뒤로가기**다. (Phase 6 피드백에 따라 화면 클리어 없이 결과 메시지가 그대로 남은 채로 메인 메뉴가 이어서 표시된다.)

## `OrderController` 설계
```cpp
class OrderController : public ISubMenuController
{
    public:
        OrderController(IOrderView& view, IInputReader& input_reader,
            ISampleRepository& sample_repository, IOrderRepository& order_repository);

        void run() override;               // processReservation()을 한 번 호출하고 반환
        bool processReservation();          // 테스트 용이성을 위해 공개. Y로 확정 시 true, 취소/오류 시 false

    private:
        IOrderView& view;
        IInputReader& input_reader;
        ISampleRepository& sample_repository;
        IOrderRepository& order_repository;
};
```

**`processReservation()` 단계별 동작**
1. "시료 ID > " 입력 → `sample_repository.findById(id)`
   - 없으면 `view.showMessage("등록되지 않은 시료 ID입니다.")` 후 `return false` (즉시 종료 → 뒤로가기)
2. "고객명 > " 입력 (빈 문자열도 허용, 별도 검증 없음)
3. "주문 수량 > " 입력 → 정수 파싱
   - 파싱 실패 또는 1 미만 → `view.showMessage("수량은 1 이상의 숫자로 입력해주세요.")` 후 `return false`
4. `view.showConfirmation(sample, customer_name, quantity)` — 입력 내용 확인 화면 출력
5. 확인 입력 읽기 (`input_reader.readLine()`)
   - `"Y"` 또는 `"y"`:
     - `Order{"", sample.id, customer_name, quantity, OrderStatus::RESERVED}` 저장
     - `view.showReservationComplete(saved_order)` — "예약 접수 완료." + 주문번호 + 현재 상태 출력
     - `return true`
   - 그 외:
     - `view.showMessage("주문이 취소되었습니다.")`
     - `return false` (저장 안 함)

## `IOrderView` / `ConsoleOrderView`
```cpp
class IOrderView
{
    public:
        virtual ~IOrderView() = default;

        virtual void showConfirmation(const Sample& sample, const std::string& customer_name, int quantity) = 0;
        virtual void showReservationComplete(const Order& order) = 0;
        virtual void showMessage(const std::string& message) = 0;
};
```
콘솔 출력 예시 (PDF와 동일한 형태):
```
입력 내용 확인
시료   SiC 파워기판-6인치 (S-003)
고객   삼성전자 파운드리
수량   200 ea

[Y] 예약 접수   [N] 취소
선택 >
```
```
예약 접수 완료.

주문번호   ORD-20260416-0043
현재 상태  RESERVED
```
- Phase 6 피드백 원칙 재적용: **이 메뉴에서도 화면 클리어를 사용하지 않는다.**

## 공용 유틸: `OrderStatus` ↔ 표시 문자열
현재 `JsonOrderRepository.cpp`의 익명 네임스페이스 안에 `orderStatusToString`/`orderStatusFromString`이 갇혀 있어 다른 곳(View, 이후 Phase 8/11의 승인·모니터링 화면)에서 재사용할 수 없다. 이번 Phase에서 공용 헤더로 추출한다.

```cpp
// model/OrderStatusText.h
std::string orderStatusToString(OrderStatus status);
OrderStatus orderStatusFromString(const std::string& text);
```
`JsonOrderRepository`는 이 공용 함수를 사용하도록 리팩터링하고, `ConsoleOrderView`의 "현재 상태" 출력에도 동일 함수를 사용한다. (Phase 8 승인/거절 화면, Phase 11 모니터링 화면에서도 이 함수를 재사용할 예정.)

## `MainController` 연동
Phase 6에서 확립한 `ISubMenuController*` 위임 패턴을 그대로 재사용한다.
- 생성자에 `ISubMenuController* order_menu = nullptr`를 세 번째 trailing default 파라미터로 추가 (`sample_menu` 다음).
- `processCommand("2")`: `order_menu`가 있으면 `order_menu->run()` 호출, 없으면 기존 placeholder 메시지.

## 테스트 계획 (gmock)
`OrderControllerTest` 신규 작성:
- **정상 흐름 + Y 확인** → `save()`가 올바른 필드(`sample_id`/`customer_name`/`quantity`/`status=RESERVED`)로 1회 호출되는지 검증, `showReservationComplete` 호출 확인, `processReservation()`이 `true` 반환
- **정상 흐름 + N 취소** → `save()` 호출 안 됨, `showMessage`로 취소 메시지 표시, `false` 반환
- **존재하지 않는 시료 ID** → 확인 화면(`showConfirmation`)까지 가지 않고 즉시 오류, `save()` 호출 안 됨
- **잘못된 수량**(비숫자, 0, 음수) → 확인 화면까지 가지 않고 즉시 오류, `save()` 호출 안 됨
- `MainControllerTest`에 `order_menu` 위임 케이스 추가 (Phase 6의 `sample_menu` 위임 테스트와 동일한 패턴)

## 완료 기준
- Y로 확정 시 `orders.json`에 `RESERVED` 상태로 저장되고, 화면에 주문번호/상태가 표시되는지 확인
- N으로 취소 시 아무것도 저장되지 않고 메인 메뉴로 돌아가는지 확인
- 미등록 시료 ID, 잘못된 수량 각각에 대해 확인 화면 없이 즉시 오류 처리되는지 확인
- gmock 테스트 전체 통과, 로컬 빌드/실행으로 실제 콘솔 흐름(정상 예약, 취소, 오류 케이스) 검증

## 다음 Phase로 이월되는 항목
- 접수된 주문(RESERVED) 목록 조회, 승인/거절, 재고 이중 관리 로직 → Phase 8
