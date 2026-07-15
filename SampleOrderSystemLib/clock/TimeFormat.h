#pragma once

#include <cstdint>
#include <string>

std::string formatDate(int64_t millis);
std::string formatDateTime(int64_t millis);

// 좁은 테이블 컬럼용 축약 포맷: "YY/MM/DD HH:MM" (초 단위 생략)
std::string formatShortDateTime(int64_t millis);
