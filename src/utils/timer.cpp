#include "timer.h"
#include <Arduino.h>

namespace LightSensor {

Timer::Timer() {
    reset();
}

void Timer::reset() {
    start_time_ms_ = millis();
    start_time_us_ = micros();
}

uint32_t Timer::elapsedMs() const {
    return millis() - start_time_ms_;
}

uint32_t Timer::elapsedUs() const {
    return micros() - start_time_us_;
}

float Timer::elapsedSeconds() const {
    return static_cast<float>(elapsedMs()) / 1000.0f;
}

bool Timer::hasElapsed(uint32_t timeout_ms) const {
    return elapsedMs() >= timeout_ms;
}

}  // namespace LightSensor
