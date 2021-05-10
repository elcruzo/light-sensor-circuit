#include "timer.h"
#include <chrono>

namespace LightSensor {

Timer::Timer() : start_time_(std::chrono::steady_clock::now()) {
}

void Timer::reset() {
    start_time_ = std::chrono::steady_clock::now();
}

uint32_t Timer::elapsedMs() const {
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time_);
    return static_cast<uint32_t>(duration.count());
}

uint32_t Timer::elapsedUs() const {
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now - start_time_);
    return static_cast<uint32_t>(duration.count());
}

float Timer::elapsedSeconds() const {
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now - start_time_);
    return static_cast<float>(duration.count()) / 1000000.0f;
}

bool Timer::hasElapsed(uint32_t timeout_ms) const {
    return elapsedMs() >= timeout_ms;
}

} // namespace LightSensor
