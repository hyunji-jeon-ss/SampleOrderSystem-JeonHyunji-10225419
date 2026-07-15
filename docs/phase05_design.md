# Phase 5 설계 문서 — 메인 프로젝트 기반 구축

`PLAN.md`의 Phase 5에 대응하는 설계 문서. PoC(ConsoleMVC, DataPersistence)에서 검증한 패턴을 메인 프로젝트에 이식하고, 이후 Phase에서 재사용할 기반 구조를 확립한다.

## 목표
- 도메인 모델(`Sample`, `Order`) 및 상태(`OrderStatus`) 정의
- JSON 파일 기반 영속성 계층 확정 (PoC와 동일한 Repository 인터페이스 패턴)
- 메인 메뉴가 표시·종료되는 최소 골격 완성

## 프로젝트 구조
`ConsoleMVC`/`DataPersistence` PoC와 동일하게 3-프로젝트 구조를 사용한다.

```
SampleOrderSystemLib/   (정적 라이브러리)
  model/       Sample, Order, OrderStatus
  clock/       IClock, SystemClock
  repository/  ISampleRepository/JsonSampleRepository, IOrderRepository/JsonOrderRepository
  view/        IMainView, ConsoleMainView
  controller/  IInputReader/ConsoleInputReader, MainController
SampleOrderSystemApp/   (콘솔 실행 파일, Lib 참조)
SampleOrderSystemTest/  (gmock 테스트, Lib 참조)
```

## 도메인 모델

```cpp
struct Sample
{
    std::string id;                      // "S-001" 형식, 자동 채번
    std::string name;
    long long avg_production_time_ms;    // 단위: 밀리초 (PRD 6.6 결정 참고)
    double yield;
};

struct Order
{
    std::string order_number;            // "ORD-YYYYMMDD-NNNN" 형식, 자동 채번
    std::string sample_id;
    std::string customer_name;
    int quantity;
    OrderStatus status;                  // RESERVED/REJECTED/PRODUCING/CONFIRMED/RELEASED
};
```

## ID / 주문번호 채번 설계
- **시료 ID (`S-XXX`)**: `JsonSampleRepository`가 생성 시점에 파일을 읽어 기존 ID의 최대 순번을 파악하고, 이후 저장부터 `+1`씩 채번한다. 재시작 후에도 이어짐(재실행 시 파일을 다시 읽어 최대값을 재계산).
- **주문번호 (`ORD-YYYYMMDD-NNNN`)**: `JsonOrderRepository`가 `IClock`으로부터 현재 시각을 받아 오늘 날짜를 계산하고, 같은 날짜 prefix(`ORD-{today}-`)를 가진 기존 주문 중 최대 순번 `+1`을 4자리로 채번한다.

## 시간 의존성 분리 (`IClock`)
주문번호 채번이 "오늘 날짜"에 의존하기 때문에, 실제 시스템 시계에 직접 의존하지 않도록 `IClock` 인터페이스로 분리했다.
- `SystemClock`: `std::chrono::system_clock` 기반 실제 구현 (`SampleOrderSystemApp`에서 사용)
- 테스트에서는 `gmock`으로 `IClock`을 모킹해 날짜를 고정하고 채번 시퀀스를 검증한다.
- 이 인터페이스는 Phase 9(생산라인 실시간 타이머)에서도 그대로 재사용할 계획이다 (생산 시작/완료 시각 계산, 재기동 시 경과 시간 판단).

## 영속성 계층
`DataPersistence` PoC와 동일하게 nlohmann/json 기반 파일 저장소를 사용한다. `Sample`/`Order`를 각각 `samples.json`, `orders.json`에 저장하며, Repository 인터페이스(`save`/`find*`)와 사용법은 PoC와 일관되게 유지했다 (코드는 재작성, 패턴은 동일).

## 메인 메뉴 골격
- 요약 정보(등록 시료 수, 총 재고, 전체 주문 수, 생산라인 대기 건수)를 표시한다.
- **총 재고는 현재 `0`으로 고정된 의도적 placeholder**다 — 재고(내부 가용 재고/화면 표시 재고 이원화, PRD 6.4)는 Phase 6·8에서 실제로 구현된다.
- 메뉴 `1`~`6`은 아직 "다음 Phase에서 구현 예정" 메시지만 표시하는 placeholder이며, `0`은 정상 종료된다.

## 테스트 및 검증
- gmock 기반 단위 테스트 16개: `JsonSampleRepositoryTest`(7), `JsonOrderRepositoryTest`(6, `MockClock`으로 날짜 고정), `MainControllerTest`(3, View/InputReader/Repository 전부 Mock).
- 로컬 MSBuild로 실제 빌드 성공(오류 0개) 및 테스트 전체 통과, 콘솔 실행으로 메뉴 표시/종료 흐름까지 확인 완료.

## 추가 반영 (Phase 5 완료 후)
- **메인 메뉴에 현재 시스템 시각 표시** (PDF 예시 UI의 "시스템 현황 2026-04-16 09:32:15" 참고): `MainMenuSummary`에 `current_time_text` 필드를 추가하고, `MainController`가 `IClock`을 주입받아 `buildSummary()`에서 채운다.
- 날짜/시간 포맷팅 로직(`formatDate`, `formatDateTime`)을 `clock/TimeFormat.h/.cpp`로 공용화하여 `JsonOrderRepository`(주문번호 채번)와 `MainController`(메뉴 시각 표시) 양쪽에서 재사용한다.

## 다음 Phase로 이월되는 항목
- 재고 이중 관리(`availableStock`/`physicalStock`) 구현 — Phase 6(시료 관리), Phase 8(주문 승인/거절)
- 생산라인 실시간 타이머 및 재기동 복원 — Phase 9
- 메인 메뉴 `1`~`6`을 실제 기능으로 교체 — Phase 6~11 진행에 따라 순차 반영
