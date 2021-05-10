#pragma once

#include <cstdint>
#include <chrono>
#include <functional>

namespace LightSensor {

/**
 * @brief Power management modes
 */
enum class PowerMode {
    ACTIVE,         // Full power, all systems running
    LOW_POWER,      // Reduced power, essential systems only
    SLEEP,          // Sleep mode, wake on interrupt
    DEEP_SLEEP      // Deep sleep, minimal power consumption
};

/**
 * @brief Power management configuration
 */
struct PowerConfig {
    // Sleep configuration
    uint32_t sleep_timeout_ms;        // Timeout before entering sleep
    uint32_t deep_sleep_timeout_ms;   // Timeout before deep sleep
    bool enable_wake_on_light;        // Wake on light level change
    float light_threshold;            // Light level threshold for wake-up
    
    // Power optimization
    bool disable_unused_peripherals;  // Disable unused peripherals
    bool reduce_clock_speed;          // Reduce CPU clock speed
    uint32_t adc_sample_delay_ms;     // Delay between ADC samples
    
    // Battery management
    float low_battery_threshold;      // Low battery voltage threshold
    float critical_battery_threshold; // Critical battery voltage threshold
    bool enable_battery_monitoring;   // Enable battery voltage monitoring
};

/**
 * @brief Power statistics
 */
struct PowerStats {
    uint32_t total_active_time_ms;    // Total time in active mode
    uint32_t total_sleep_time_ms;     // Total time in sleep mode
    uint32_t wake_count;              // Number of wake-ups
    float average_current_ma;         // Average current consumption
    float peak_current_ma;            // Peak current consumption
    float battery_voltage;            // Current battery voltage
    uint8_t battery_percentage;       // Battery percentage (0-100)
};

/**
 * @brief Wake-up source types
 */
enum class WakeSource {
    TIMER,          // Timer-based wake-up
    LIGHT_CHANGE,   // Light level change
    BUTTON,         // Button press
    EXTERNAL,       // External interrupt
    LOW_BATTERY     // Low battery warning
};

/**
 * @brief Callback function for power events
 */
using PowerEventCallback = std::function<void(PowerMode, WakeSource)>;

/**
 * @brief Power management class for low-power operation
 */
class PowerManager {
public:
    explicit PowerManager(const PowerConfig& config);
    ~PowerManager() = default;
    
    /**
     * @brief Initialize power management
     * @return true if initialization successful
     */
    bool initialize();
    
    /**
     * @brief Set power mode
     * @param mode Power mode to set
     */
    void setPowerMode(PowerMode mode);
    
    /**
     * @brief Get current power mode
     * @return Current power mode
     */
    PowerMode getCurrentMode() const;
    
    /**
     * @brief Enter sleep mode for specified duration
     * @param duration_ms Sleep duration in milliseconds
     * @param wake_source Wake-up source
     */
    void sleep(uint32_t duration_ms, WakeSource wake_source = WakeSource::TIMER);
    
    /**
     * @brief Wake up from sleep
     * @param source Source that caused wake-up
     */
    void wakeUp(WakeSource source);
    
    /**
     * @brief Check if system should enter low power mode
     * @return true if should enter low power mode
     */
    bool shouldEnterLowPower() const;
    
    /**
     * @brief Optimize power consumption based on current activity
     */
    void optimizePowerConsumption();
    
    /**
     * @brief Get power statistics
     * @return Power statistics structure
     */
    PowerStats getPowerStats() const;
    
    /**
     * @brief Set power event callback
     * @param callback Function to call on power events
     */
    void setPowerEventCallback(PowerEventCallback callback);
    
    /**
     * @brief Update battery voltage reading
     * @param voltage Current battery voltage
     */
    void updateBatteryVoltage(float voltage);
    
    /**
     * @brief Check if battery is low
     * @return true if battery is low
     */
    bool isBatteryLow() const;
    
    /**
     * @brief Check if battery is critical
     * @return true if battery is critical
     */
    bool isBatteryCritical() const;
    
    /**
     * @brief Enable/disable wake on light change
     * @param enable Enable flag
     * @param threshold Light threshold for wake-up
     */
    void setWakeOnLight(bool enable, float threshold = 0.1f);
    
    /**
     * @brief Process power management (call in main loop)
     */
    void process();
    
private:
    PowerConfig config_;
    PowerMode current_mode_;
    PowerStats stats_;
    PowerEventCallback event_callback_;
    
    std::chrono::steady_clock::time_point last_activity_time_;
    std::chrono::steady_clock::time_point sleep_start_time_;
    bool wake_on_light_enabled_;
    float last_light_level_;
    
    /**
     * @brief Configure hardware for power mode
     * @param mode Power mode to configure for
     */
    void configureHardwareForMode(PowerMode mode);
    
    /**
     * @brief Disable unused peripherals
     */
    void disableUnusedPeripherals();
    
    /**
     * @brief Enable essential peripherals
     */
    void enableEssentialPeripherals();
    
    /**
     * @brief Update power statistics
     */
    void updatePowerStats();
    
    /**
     * @brief Calculate current consumption estimate
     * @return Estimated current in milliamps
     */
    float calculateCurrentConsumption() const;
};

} // namespace LightSensor
