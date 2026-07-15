#include "console/ConsoleUtil.h"

#define NOMINMAX
#include <windows.h>

#include <algorithm>

namespace
{
    // UTF-8 문자열의 콘솔 표시 너비를 계산한다. ASCII는 1칸, 3바이트 UTF-8(한글 등)과
    // 4바이트 시퀀스는 2칸으로 취급한다 (일반적인 고정폭 콘솔 폰트 기준의 근사치).
    std::size_t displayWidth(const std::string& text)
    {
        std::size_t width = 0;
        for (std::size_t i = 0; i < text.size(); )
        {
            const unsigned char lead = static_cast<unsigned char>(text[i]);
            if (lead < 0x80) { width += 1; i += 1; }
            else if ((lead & 0xE0) == 0xC0) { width += 1; i += 2; }
            else if ((lead & 0xF0) == 0xE0) { width += 2; i += 3; }
            else if ((lead & 0xF8) == 0xF0) { width += 2; i += 4; }
            else { width += 1; i += 1; }
        }
        return width;
    }
}

std::string padEnd(const std::string& text, std::size_t target_width)
{
    const std::size_t current_width = displayWidth(text);
    if (current_width >= target_width) return text;
    return text + std::string(target_width - current_width, ' ');
}

std::string padStart(const std::string& text, std::size_t target_width)
{
    const std::size_t current_width = displayWidth(text);
    if (current_width >= target_width) return text;
    return std::string(target_width - current_width, ' ') + text;
}

std::string renderProgressBar(int percent, int width)
{
    const int clamped_percent = std::clamp(percent, 0, 100);
    const int filled = width * clamped_percent / 100;

    std::string bar;
    for (int i = 0; i < width; i++)
    {
        bar += (i < filled) ? "\xE2\x96\x88" : "\xE2\x96\x91";
    }
    return bar;
}

void clearConsoleScreen()
{
    const HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    if (console == INVALID_HANDLE_VALUE) return;

    CONSOLE_SCREEN_BUFFER_INFO screen_buffer_info;
    if (!GetConsoleScreenBufferInfo(console, &screen_buffer_info)) return;

    const DWORD console_size =
        static_cast<DWORD>(screen_buffer_info.dwSize.X) * static_cast<DWORD>(screen_buffer_info.dwSize.Y);
    const COORD origin = { 0, 0 };
    DWORD written = 0;

    FillConsoleOutputCharacter(console, ' ', console_size, origin, &written);
    FillConsoleOutputAttribute(console, screen_buffer_info.wAttributes, console_size, origin, &written);
    SetConsoleCursorPosition(console, origin);
}
