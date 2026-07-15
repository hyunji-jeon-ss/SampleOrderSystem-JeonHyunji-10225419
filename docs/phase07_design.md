# Phase 7 설계 문서 — 시료 주문(예약) 기능

`PLAN.md` Phase 7, `PRD.md` 6.3에 대응. 고객의 시료 요청을 주문(예약)으로 등록하는 기능을 구현한다.

## 목표
- 시료 ID / 고객명 / 주문 수량을 입력받아 주문을 생성한다.
- 생성된 주문은 `RESERVED` 상태로 저장된다.
- 주문번호는 `ORD-YYYYMMDD-NNNN` 형식으로 자동 채번된다 (Phase 5의 `JsonOrderRepository`가 이미 구현).
- 등록되지 않은 시료 ID로는 주문할 수 없다 (PRD 6.2 "등록된 시료만 주문 가능").

## 메뉴 구조: 서브메뉴가 아니라 단일 동작
`SampleController`(Phase 6)는 등록/조회/검색 3가지 선택지가 있는 서브메뉴였지만, PRD 6.3은 "시료 주문" 메뉴에 예약 생성 하나만 정의한다. 따라서 `OrderController`는 반복 루프(서브메뉴)를 갖지 않고, **메인 메뉴에서 "2"를 선택하면 예약 입력 → 저장 → 결과 표시까지 한 번에 수행하고 곧바로 메인 메뉴로 돌아간다.**

- `ISubMenuController`(Phase 6에서 도입) 인터페이스는 동일하게 구현하지만, `run()`의 동작이 "루프"가 아니라 "1회 실행"이라는 점만 다르다. 인터페이스 재사용에는 문제 없음 (메인 메뉴 입장에서는 `run()`이 반환되면 다시 메인 메뉴로 돌아가는 것은 동일).

## `OrderController` 설계
```cpp
class OrderController : public ISubMenuController
{
    public:
        OrderController(IOrderView& view, IInputReader& input_reader,
            ISampleRepository& sample_repository, IOrderRepository& order_repository);

        void run() override;              // processReservation()을 한 번 호출하고 반환
        bool processReservation();         // 테스트 용이성을 위해 공개. 성공 시 true

    private:
        IOrderView& view;
        IInputReader& input_reader;
        ISampleRepository& sample_repository;
        IOrderRepository& order_repository;
};
```

**`processReservation()` 흐름**
1. "시료 ID > " 입력 → `sample_repository.findById(id)`로 존재 확인
   - 없으면 "등록되지 않은 시료 ID입니다." 메시지 후 즉시 중단 (주문 생성 안 함)
2. "고객명 > " 입력 (빈 문자열도 일단 허용 — 별도 검증 없음, 필요시 이후 Phase에서 보강)
3. "주문 수량 > " 입력 → 정수 파싱, **1 이상의 양의 정수**만 허용
   - 파싱 실패 또는 0 이하 → "수량은 1 이상의 숫자로 입력해주세요." 메시지 후 중단
4. `Order{"", sample_id, customer_name, quantity, OrderStatus::RESERVED}` 저장 (`order_number`는 Repository가 자동 채번)
5. 저장된 주문 정보(주문번호, 시료, 고객, 수량, 상태)를 `view.showOrderConfirmation(saved_order)`로 표시

## `IOrderView` / `ConsoleOrderView`
```cpp
class IOrderView
{
    public:
        virtual ~IOrderView() = default;

        virtual void showOrderConfirmation(const Order& order) = 0;
        virtual void showMessage(const std::string& message) = 0;
};
```
- `ConsoleMVC`/`SampleController` 패턴과 동일하게 View는 출력만 담당, 입력은 Controller가 `IInputReader`로 직접 처리한다.
- Phase 6 피드백 반영: **화면 클리어는 이 메뉴에서도 사용하지 않는다** (주문 결과 메시지가 사용자가 읽기 전에 지워지는 문제를 이미 Phase 6에서 겪었으므로 동일 원칙 적용).

## `MainController` 연동
Phase 6에서 확립한 `ISubMenuController*` 위임 패턴을 그대로 재사용한다.
- `MainController` 생성자에 `ISubMenuController* order_menu = nullptr`를 **두 번째 trailing default 파라미터**로 추가 (기존 5~6개 인자 호출과 하위 호환).
- `processCommand("2")`: `order_menu`가 있으면 `order_menu->run()` 호출, 없으면 기존 placeholder 메시지.

## 테스트 계획 (gmock)
`OrderControllerTest` 신규 작성:
- 정상 입력(존재하는 시료 ID, 고객명, 양의 정수 수량) → `order_repository.save()`가 `sample_id`/`customer_name`/`quantity`/`status=RESERVED`가 올바른 `Order`로 1회 호출되는지 검증
- 존재하지 않는 시료 ID 입력 → `save()` 호출 안 됨, 에러 메시지 표시
- 수량이 숫자가 아니거나 0 이하 → `save()` 호출 안 됨, 에러 메시지 표시
- `MainControllerTest`에 `order_menu` 위임 케이스 추가 (Phase 6의 `sample_menu` 위임 테스트와 동일한 패턴)

## 완료 기준
- 정상적으로 예약 생성 후, `orders.json`에서 상태가 `RESERVED`로 저장되는지 확인 (재시작 후에도 유지 — Repository는 이미 Phase 5에서 검증됨)
- 미등록 시료 ID로 주문 시도 시 거부되는지 확인
- gmock 테스트 전체 통과, 로컬 빌드/실행으로 실제 콘솔 흐름 검증 (등록된 시료로 주문 → 확인 메시지 → 메인 메뉴 복귀)

## 다음 Phase로 이월되는 항목
- 접수된 주문(RESERVED) 목록 조회, 승인/거절, 재고 이중 관리 로직 → Phase 8
