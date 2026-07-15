# Phase 6 설계 문서 — 시료 관리 기능

`PLAN.md` Phase 6, `PRD.md` 6.2에 대응. 시료 등록/조회/검색과 재고 표시 기반을 구현한다.

## 목표
- 시료 등록(이름, 평균 생산시간, 수율 입력 → ID 자동 채번)
- 시료 조회(현재 재고 수량 포함)
- 시료 검색(이름 기준)
- 재고는 재시작 후에도 유지 (PRD 6.2 추가 제약)

## 도메인 모델 변경: `Sample`에 재고 필드 추가
```cpp
struct Sample
{
    std::string id;
    std::string name;
    long long avg_production_time_ms = 0;
    double yield = 1.0;
    int physical_stock = 0;    // 화면 표시용 재고 (PRD 6.4/6.5의 "화면 표시용 재고")
    int available_stock = 0;   // 내부 판단용 가용 재고 (PRD 6.4, Phase 8 주문 승인 로직이 사용)
};
```
- 두 필드 모두 `samples.json`에 함께 영속화한다. 등록 시 둘 다 `0`으로 시작한다 (등록 시점엔 초기 재고 입력이 없음 — PRD 6.2는 등록 속성으로 ID/이름/평균생산시간/수율만 명시).
- `available_stock`은 이번 Phase에서는 읽기만 하고 값을 변경하지 않는다 (변경 로직은 Phase 8에서 구현). 지금 필드를 미리 추가하는 이유는, Phase 8에서 재고 이중 관리를 도입할 때 `Sample` 구조체를 다시 건드리지 않기 위함이다.
- `physical_stock`도 이번 Phase에서는 변경되지 않는다 (생산 완료 시 증가는 Phase 9, 출고 시 감소는 Phase 10에서 구현). 현재는 등록 시 0으로 고정, 조회 시 그대로 표시.

## 서브메뉴 컨트롤러 패턴 (Phase 7~11에서 재사용)
`MainController`가 모든 기능을 직접 구현하지 않고, 메뉴별로 별도 컨트롤러에 위임하는 구조를 이번 Phase에서 확립한다.

```cpp
class ISubMenuController
{
    public:
        virtual ~ISubMenuController() = default;
        virtual void run() = 0;   // 서브메뉴 표시 ~ "0"(뒤로가기) 입력까지 루프
};
```

- `SampleController : ISubMenuController` — 이번 Phase에서 구현 ("1. 시료 관리")
- `MainController`는 `ISubMenuController* sample_menu = nullptr` 형태로 **nullable 포인터**를 추가로 주입받는다 (trailing default parameter라 기존 생성자 호출/테스트는 그대로 컴파일된다).
  - `processCommand("1")`: `sample_menu`가 있으면 `sample_menu->run()` 호출, 없으면 기존처럼 "아직 구현되지 않은 기능" 메시지.
  - Phase 7~11도 동일한 패턴으로 `order_menu`, `approval_menu`, `monitoring_menu`, `production_menu`, `release_menu`를 순서대로 추가한다.

## `SampleController` 설계
```cpp
class SampleController : public ISubMenuController
{
    public:
        SampleController(ISampleView& view, IInputReader& input_reader, ISampleRepository& sample_repository);

        void run() override;
        bool processCommand(const std::string& command);   // 테스트 용이성을 위해 공개

    private:
        void handleRegister();
        void handleList();
        void handleSearch();
};
```

- **`ISampleView`**: `showSampleMenu()`, `showSamples(const std::vector<Sample>&)`, `showMessage(const std::string&)` — `ConsoleMVC`/PoC들과 동일한 View 분리 패턴.
- **등록(`handleRegister`)**: 이름 → 평균 생산시간(ms, 정수) → 수율(0~1 실수) 순으로 입력받아 `Sample{"", name, time_ms, yield, 0, 0}`을 저장. 파싱 실패 시 에러 메시지 출력 후 등록 중단(크래시 없이).
- **조회(`handleList`)**: `findAll()` 결과를 ID/이름/평균생산시간/수율/재고(physical_stock) 형태로 표시.
- **검색(`handleSearch`)**: 검색어 입력 → 이름에 대소문자 무시 부분일치(`DataMonitor` PoC와 동일한 방식)로 필터링해 표시. Repository에 검색 메서드를 추가하지 않고, Controller에서 `findAll()` 결과를 필터링한다 (Repository 인터페이스는 CRUD 최소 계약만 유지).

## 메인 메뉴 요약 정보 갱신
`MainController::buildSummary()`의 "총 재고" 플레이스홀더(0 고정)를 실제 계산으로 교체한다: 등록된 모든 `Sample.physical_stock`의 합.

## 테스트 계획 (gmock)
- `JsonSampleRepositoryTest`: 재고 필드 저장/조회 및 재시작 후 유지 검증 케이스 추가.
- `SampleControllerTest` (신규): `MockSampleView`, `MockInputReader`, `MockSampleRepository`로 등록/조회/검색/뒤로가기 각 분기를 검증.
- `MainControllerTest`: `sample_menu` 주입 시 "1" 명령이 `ISubMenuController::run()`을 호출하는지 확인하는 케이스 추가 (Mock으로 대체).

## 완료 기준
- 시료 등록 → 프로그램 재시작 → 조회 시 데이터(이름/생산시간/수율/재고) 유지 확인
- 검색어로 필터링된 결과만 표시되는지 확인
- gmock 테스트 전체 통과, 로컬 빌드/실행으로 실제 콘솔 흐름 검증

## 변경 이력
- **평균 생산시간 단위를 ms → 분(min)으로 재변경** (사용자 요청, `PRD.md` 6.6 기존 결정 대체). `avg_production_time_ms`(long long) → `avg_production_time_min`(double)로 변경, 소수점 입력 허용(예: `0.1`min). Phase 9 실시간 타이머 계산 시 `분 * 60000`으로 ms 환산해서 사용할 예정.
- **시료 목록/검색 결과에 페이지네이션 추가**: 5개씩 표시, 다 보여준 뒤 남은 항목이 있으면 "[N] 다음 페이지" 프롬프트 → `N`/`n` 입력 시에만 다음 페이지, 그 외 입력 시 종료. `SampleController::displayPaged()`로 공용화해 조회/검색 양쪽에서 재사용.

## 변경 이력 (2)
- **화면 클리어**: 사용자가 메뉴에서 선택할 때마다 화면을 지우고 결과를 보여주도록 변경. `console/ConsoleUtil.h/.cpp`에 Win32 콘솔 API(`FillConsoleOutputCharacter` 등) 기반 `clearConsoleScreen()`을 구현하고, `MainController`/`SampleController`의 `run()` 루프(메뉴 표시 전, 입력 직후, 페이지네이션 전환 시)에 적용. 콘솔이 아닌 환경(파이프/리다이렉션, 테스트)에서는 `GetConsoleScreenBufferInfo` 실패 시 안전하게 no-op.
- **시료 목록/검색 테이블 정렬 고정**: 값 길이가 들쭉날쭉해 정렬이 어긋나던 문제를 고정폭 컬럼으로 해결. `ConsoleUtil`에 UTF-8 표시 너비를 고려한 `padEnd`(왼쪽 정렬)/`padStart`(오른쪽 정렬) 공용 함수를 추가해 한글(3바이트, 표시폭 2칸)과 ASCII를 함께 정렬한다. ID/이름은 왼쪽 정렬, 생산시간/수율/재고는 오른쪽 정렬.

## 변경 이력 (3) — 화면 클리어 범위 조정
사용자 피드백: 시료 관리 서브메뉴(등록/조회/검색)에서 클리어가 너무 자주 발생해, 등록 완료 메시지나 페이지네이션 2페이지 내용이 사용자가 읽기도 전에 곧바로 다음 화면으로 덮어써지는 문제가 있었다 (예: "N"을 눌러도 다음 페이지로 안 넘어가는 것처럼 보임 — 실제로는 넘어갔지만 화면이 바로 지워짐).

- `SampleController::run()`과 `displayPaged()`에서 `clearConsoleScreen()` 호출을 전부 제거했다. 시료 관리 서브메뉴 안에서는 이제 클리어 없이 이전 내용 위에 계속 이어서 출력된다 — 등록 완료 메시지, 조회/검색 결과, 페이지 전환 내용이 전부 화면에 남아 사용자가 확인할 수 있다.
- `MainController`의 메인 메뉴 클리어 동작은 그대로 유지한다 (이번 피드백 대상이 아님).

## 구현 결과 (완료)
- `Sample`에 `physical_stock`/`available_stock` 필드 추가, `JsonSampleRepository`가 두 필드 모두 영속화
- `ISubMenuController` 도입, `MainController`가 `ISubMenuController* sample_menu = nullptr`(trailing default)로 서브메뉴에 위임하는 구조 확립 — 기존 생성자 호출/테스트는 변경 없이 컴파일됨
- `ISampleView`/`ConsoleSampleView`, `SampleController`(등록/조회/검색/뒤로가기) 구현
- `MainController::buildSummary()`의 총 재고 계산을 `physical_stock` 합산으로 교체
- gmock 테스트 26개 전체 통과(`JsonSampleRepositoryTest` 9, `JsonOrderRepositoryTest` 6, `MainControllerTest` 5, `SampleControllerTest` 6 신규)
- 실제 실행으로 등록 → 재시작 → 조회/검색 데이터 유지 확인, 메인 메뉴 "등록 시료 1종" 요약 반영 확인
