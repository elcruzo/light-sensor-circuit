#pragma once

#include <cstdint>

namespace LightSensor {

/**
 * @brief Simple timer utility class for ESP32
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
    uint32_t start_time_ms_;
    uint32_t start_time_us_;
};

}  // namespace LightSensor
