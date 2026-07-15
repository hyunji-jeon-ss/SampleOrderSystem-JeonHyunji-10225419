#pragma once

#include <string>

void clearConsoleScreen();

// 한글(다국어) 표시 너비를 고려해 오른쪽에 공백을 채워 왼쪽 정렬한다 (콘솔 테이블 컬럼 정렬용).
std::string padEnd(const std::string& text, std::size_t target_width);

// 한글(다국어) 표시 너비를 고려해 왼쪽에 공백을 채워 오른쪽 정렬한다 (콘솔 테이블 컬럼 정렬용).
std::string padStart(const std::string& text, std::size_t target_width);
