#pragma once

#include <chrono>
#include <string>

namespace Format
{
    std::string date(const std::chrono::time_point<std::chrono::system_clock>& date);
    std::string timeDuration(size_t msec_duration);
    std::string progress(double progress);
    std::string largeNumber(size_t n);
}