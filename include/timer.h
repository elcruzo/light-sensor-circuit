#pragma once

#include <cstdint>
#include <chrono>

namespace LightSensor {

/**
 * @brief Simple timer utility class
 */
class Timer {
public:
    Timer();
    
    void reset();
    uint32_t elapsedMs() const;
    uint32_t elapsedUs() const;
    float elapsedSeconds() const;
    bool hasElapsed(uint32_t timeout_ms) const;
    
private:
    std::chrono::steady_clock::time_point start_time_;
};

} // namespace LightSensor
