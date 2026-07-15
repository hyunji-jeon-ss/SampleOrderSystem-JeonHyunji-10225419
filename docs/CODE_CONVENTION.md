# Code Convention
우리 팀이 지켜야 할 코드 컨벤션을 이 문서를 통해 확인하세요.


## Include
가장 가까운 헤더부터 상단에 작성하고 줄바꿈으로 서로 구분합니다.
그리고 각 헤더와 소스코드 파일을 구분해서 그 파일에서 필요한 헤더만 include 합니다.
> 헤더를 따라가지 않아도 되도록 cpp에 필요한 헤더는 다시 include 합니다.

```cpp
#include "MyClass.h"

#include <android/log.h>

#include <iostream>
#include <string>
```


Using declaration을 사용해서 네임스페이스를 통한 접근을 하지 않도록 합니다.
std처럼 광범위하게 풀지 않고 실제 사용하는 부분에 대해서만 사용합니다. 단, testing은 예외로 합니다.

```cpp
#include <vector>

#include "gmock/gmock.h"

using namespace std;     // (X)
using std::vector;       // (O)
using namespace testing; // (O)
```

## Class
멤버변수와 메소드의 순서를 아래와 같이 메소드 > 멤버변수 순으로 하고, public > protected > private 순으로 합니다.
클래스의 이름은 대문자부터 시작하는 파스칼, 메소드 이름은 소문자부터 시작하는 카멜, 변수 이름은 소문자와 _로 이뤄진 스네이크 방식을 사용합니다.


```cpp
class Something
{
    public:
        void publicMethod();

    protected:
        void protectedMethod();

    private:
        void privateMethod();

        int member_variable;
}
```


## File
파일명의 경우 클래스 구현과 관련된 파일의 경우 파스칼 방식을 사용하고 만약 인터페이스라면 앞에 I를 붙여줍니다.

> IPlayer.h Player.h  Player.cpp

만약 유틸리티 함수만을 포함하고 있는 헤더 및 소스코드 파일이라면 스네이크 방식을 사용합니다.

> util_logger.h


## Function
한 줄짜리 동작을 하는 조건문의 경우 한 줄로 표현합니다.

```cpp
// 이 방법 사용
if (check()) return;

// 중괄호 없이 코드를 추가하는 실수를 유발할 수 있기 때문에 사용하지 않음
if (check())
    return;

// 불필요한 공간을 많이 차지하므로 사용하지 않음
if (check())
{
    return;
}
```