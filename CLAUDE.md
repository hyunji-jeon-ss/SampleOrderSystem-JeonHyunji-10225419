# CLAUDE.md — SampleOrderSystem-JeonHyunji-10225419

## 프로젝트 개요
반도체 시료 생산주문관리 시스템 메인 프로젝트. 상세 요구사항은 `PRD.md`, 구현 순서는 `PLAN.md`를 따른다.

## 기술 스택 / 컨벤션
- C++20, Visual Studio(MSBuild, .vcxproj), gmock(NuGet)
- nlohmann/json (NuGet 패키지 `nlohmann.json`) — `DataPersistence` PoC와 동일한 라이브러리/사용법 유지
- 코드 컨벤션은 `CODE_CONVENTION.md`를 따른다 (PascalCase 클래스, camelCase 메서드, snake_case 변수 등).

## 아키텍처
- MVC 구조: `SampleOrderSystemLib/` 아래 `model/`, `view/`, `controller/`, `repository/`, `clock/` (정적 라이브러리) + `SampleOrderSystemApp/`(실행 파일) + `SampleOrderSystemTest/`(gmock 테스트), 3-프로젝트 구조 (`ConsoleMVC`/`DataPersistence` PoC와 동일한 패턴)
- 영속성 계층: **JSON 파일 기반**(nlohmann/json), `Sample`/`Order` 데이터를 재실행 후에도 유지 (`JsonSampleRepository`, `JsonOrderRepository`)
- ID/주문번호 자동 채번: 시료 `S-XXX`, 주문 `ORD-YYYYMMDD-NNNN` (`PRD.md` 6.2, 6.3 참고). 주문번호 채번은 `IClock`에 의존하므로 시간 관련 로직은 항상 이 인터페이스를 통해 목킹 가능하게 유지한다.
- 재고는 **내부 판단용 가용 재고(availableStock)**와 **화면 표시용 재고(physicalStock)**로 이원화하여 관리 (`PRD.md` 6.4, 6.5 참고) — Phase 6/8에서 구현 예정, 아직 미구현
- 생산라인은 현실 시간(ms 단위) 기반 타이머로 동작하며, 프로그램 재기동 시 저장된 시작/완료 시각을 기준으로 상태를 복원한다 (`PRD.md` 6.6 참고) — Phase 9에서 구현 예정, 아직 미구현

## 설계 문서
Phase별 상세 설계는 `docs/phase{NN}_design.md`에 기록한다 (예: `docs/phase05_design.md`). 새 Phase를 시작하기 전에 해당 문서를 먼저 작성/갱신한다.

## 진행 상황
- **Phase 5 완료**: 도메인 모델(`Sample`/`Order`/`OrderStatus`), JSON Repository, `IClock`/`SystemClock`, 메인 메뉴 골격(표시·종료만 동작, 1~6번은 플레이스홀더) 구현 및 검증 완료. 설계 문서: `docs/phase05_design.md`
- **Phase 6 완료**: 시료 등록/조회/검색(`SampleController`), `Sample`에 재고 필드(`physical_stock`/`available_stock`) 추가, `ISubMenuController` 서브메뉴 위임 패턴 확립. 설계 문서: `docs/phase06_design.md`
- **Phase 7 완료**: 시료 주문(예약) 기능(`OrderController`) — 입력 내용 확인(Y/N) → 확정 시 저장+주문번호/상태 출력, 취소 시 미저장. `orderStatusToString`/`orderStatusFromString`을 `model/OrderStatusText.h/.cpp`로 공용 추출. 설계 문서: `docs/phase07_design.md`
- 다음: Phase 8(주문 승인/거절) — 재고 이중 관리(`availableStock`/`physicalStock`) 로직 구현.

## 개발 순서
`PLAN.md`의 Phase 5부터 Phase 12까지 순서대로 진행한다. 각 Phase 완료 시 해당 Phase의 "완료 기준" 항목을 충족했는지 확인한다.

## 테스트
- 모든 기능은 구현 시마다 gmock 기반 단위 테스트를 함께 작성한다.
- 시간(Clock), 저장소(Repository) 등 외부 의존성은 인터페이스로 분리하여 Mock 처리한다.

## 한글 인코딩 (중요)
콘솔에 한글을 출력하므로 반드시 아래 두 가지를 유지한다. 자세한 이유는 상위 `Semiconductor` 폴더의 `CLAUDE.md` 참고.
1. 모든 `.vcxproj`의 각 ClCompile 설정에 `<AdditionalOptions>/utf-8 %(AdditionalOptions)</AdditionalOptions>` 적용
2. 콘솔 진입점(main)에서 `SetConsoleOutputCP(CP_UTF8)` / `SetConsoleCP(CP_UTF8)` 호출

## 커밋 컨벤션
`COMMIT_CONVENTION.md`를 따른다. 커밋 메시지는 `<헤더> 변경 내용` 형식이며, 헤더는 `<FEATURE>`/`<FIX>`/`<DOCS>`/`<STYLE>`/`<REFACTOR>`/`<TEST>`/`<CHORE>` 중 하나만 사용한다.

## 빌드/실행
Visual Studio에서 솔루션을 열어 빌드/실행한다. **빌드가 실패한 상태에서는 절대 커밋하지 않는다** — 반드시 로컬 빌드 성공(및 가능하면 테스트 통과)을 확인한 뒤 커밋한다. 자세한 내용은 상위 `Semiconductor` 폴더의 `CLAUDE.md` 참고.

## Harness — 커맨드라인 빌드/테스트 명령어
Visual Studio GUI 없이(예: Claude Code 세션에서) 커밋 전 빌드/테스트를 검증할 때 사용하는 명령어. Git Bash 기준.

**1. MSBuild 경로 찾기** (VS 버전에 따라 경로가 달라질 수 있어, 하드코딩 대신 `vswhere`로 찾는 것을 권장)
```bash
VSWHERE="/c/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe"
MSBUILD=$("$VSWHERE" -latest -requireAny \
  -requires Microsoft.Component.MSBuild \
  -find "MSBuild\**\Bin\MSBuild.exe" | head -1)
```
(참고: 이 저장소 검증 시점에는 `C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\amd64\MSBuild.exe` 였음)

**2. NuGet 패키지 복원** (packages.config 방식이라 `msbuild -t:restore`가 아닌 `nuget.exe restore`가 필요)
```bash
# 최초 1회: nuget.exe 다운로드 (별도 스크래치 폴더에 받아두고 재사용)
curl -sL -o nuget.exe https://dist.nuget.org/win-x86-commandline/latest/nuget.exe

# 저장소 루트(SampleOrderSystem.sln 위치)에서 실행
./nuget.exe restore SampleOrderSystem.sln -NonInteractive
```

**3. 빌드**
```bash
export MSYS_NO_PATHCONV=1 MSYS2_ARG_CONV_EXCL="*"   # git bash의 "/p:" 경로 치환 방지
"$MSBUILD" SampleOrderSystem.sln "/p:Configuration=Debug" "/p:Platform=x64" "/m"
```

**4. 테스트 실행**
```bash
./x64/Debug/SampleOrderSystemTest.exe
```

**5. (선택) 콘솔 앱 스모크 테스트** — 표준입력을 파이프로 흘려 대화형 메뉴를 무인 실행
```bash
printf '1\n9\n0\n' | ./x64/Debug/SampleOrderSystemApp.exe
```
