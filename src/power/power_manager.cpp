#include "power_manager.h"
#include <algorithm>
#include <chrono>
#include <thread>

#ifdef ARDUINO
#include <Arduino.h>
#include <avr/sleep.h>
#include <avr/power.h>
#else
// Mock functions for testing
#define millis() (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count())
#define delay(ms) std::this_thread::sleep_for(std::chrono::milliseconds(ms))
#endif

namespace LightSensor {

PowerManager::PowerManager(const PowerConfig& config)
    : config_(config), current_mode_(PowerMode::ACTIVE), 
      wake_on_light_enabled_(false), last_light_level_(0.0f) {
    
    // Initialize power statistics
    stats_ = {0, 0, 0, 0.0f, 0.0f, 0.0f, 100};
    last_activity_time_ = std::chrono::steady_clock::now();
}

bool PowerManager::initialize() {
    // Initialize power management hardware
    #ifdef ARDUINO
    // Configure power reduction register
    PRR = 0;
    
    // Configure sleep mode
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    #endif
    
    last_activity_time_ = std::chrono::steady_clock::now();
    return true;
}

void PowerManager::setPowerMode(PowerMode mode) {
    if (mode == current_mode_) {
        return;
    }
    
    PowerMode previous_mode = current_mode_;
    current_mode_ = mode;
    
    configureHardwareForMode(mode);
    
    if (event_callback_) {
        event_callback_(mode, WakeSource::TIMER);
    }
    
    // Update statistics
    updatePowerStats();
}

PowerMode PowerManager::getCurrentMode() const {
    return current_mode_;
}

void PowerManager::sleep(uint32_t duration_ms, WakeSource wake_source) {
    if (current_mode_ == PowerMode::ACTIVE) {
        setPowerMode(PowerMode::SLEEP);
    }
    
    sleep_start_time_ = std::chrono::steady_clock::now();
    
    #ifdef ARDUINO
    // Configure timer for wake-up
    if (wake_source == WakeSource::TIMER) {
        // Use watchdog timer for wake-up
        wdt_enable(WDTO_8S); // 8 second timeout
    }
    
    // Enter sleep mode
    sleep_enable();
    sleep_cpu();
    sleep_disable();
    #else
    // Mock sleep for testing
    delay(duration_ms);
    #endif
    
    wakeUp(wake_source);
}

void PowerManager::wakeUp(WakeSource source) {
    if (current_mode_ == PowerMode::SLEEP || current_mode_ == PowerMode::DEEP_SLEEP) {
        setPowerMode(PowerMode::ACTIVE);
        stats_.wake_count++;
    }
    
    last_activity_time_ = std::chrono::steady_clock::now();
    
    if (event_callback_) {
        event_callback_(current_mode_, source);
    }
}

bool PowerManager::shouldEnterLowPower() const {
    auto now = std::chrono::steady_clock::now();
    auto time_since_activity = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - last_activity_time_).count();
    
    return time_since_activity > config_.sleep_timeout_ms;
}

void PowerManager::optimizePowerConsumption() {
    if (shouldEnterLowPower()) {
        if (current_mode_ == PowerMode::ACTIVE) {
            setPowerMode(PowerMode::LOW_POWER);
        } else if (current_mode_ == PowerMode::LOW_POWER) {
            auto now = std::chrono::steady_clock::now();
            auto time_since_activity = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - last_activity_time_).count();
            
            if (time_since_activity > config_.deep_sleep_timeout_ms) {
                setPowerMode(PowerMode::DEEP_SLEEP);
            }
        }
    }
}

PowerStats PowerManager::getPowerStats() const {
    return stats_;
}

void PowerManager::setPowerEventCallback(PowerEventCallback callback) {
    event_callback_ = callback;
}

void PowerManager::updateBatteryVoltage(float voltage) {
    stats_.battery_voltage = voltage;
    
    // Calculate battery percentage (assuming 3.0V - 4.2V range)
    const float min_voltage = 3.0f;
    const float max_voltage = 4.2f;
    
    if (voltage <= min_voltage) {
        stats_.battery_percentage = 0;
    } else if (voltage >= max_voltage) {
        stats_.battery_percentage = 100;
    } else {
        stats_.battery_percentage = static_cast<uint8_t>(
            ((voltage - min_voltage) / (max_voltage - min_voltage)) * 100.0f);
    }
}

bool PowerManager::isBatteryLow() const {
    return config_.enable_battery_monitoring && 
           stats_.battery_voltage < config_.low_battery_threshold;
}

bool PowerManager::isBatteryCritical() const {
    return config_.enable_battery_monitoring && 
           stats_.battery_voltage < config_.critical_battery_threshold;
}

void PowerManager::setWakeOnLight(bool enable, float threshold) {
    wake_on_light_enabled_ = enable;
    config_.light_threshold = threshold;
}

void PowerManager::process() {
    // Update power statistics
    updatePowerStats();
    
    // Check for low battery conditions
    if (isBatteryCritical()) {
        setPowerMode(PowerMode::DEEP_SLEEP);
    } else if (isBatteryLow()) {
        setPowerMode(PowerMode::LOW_POWER);
    } else {
        optimizePowerConsumption();
    }
}

void PowerManager::configureHardwareForMode(PowerMode mode) {
    switch (mode) {
        case PowerMode::ACTIVE:
            enableEssentialPeripherals();
            break;
            
        case PowerMode::LOW_POWER:
            disableUnusedPeripherals();
            #ifdef ARDUINO
            if (config_.reduce_clock_speed) {
                // Reduce clock speed (implementation depends on MCU)
                // This is a placeholder - actual implementation would be MCU-specific
            }
            #endif
            break;
            
        case PowerMode::SLEEP:
            disableUnusedPeripherals();
            #ifdef ARDUINO
            set_sleep_mode(SLEEP_MODE_PWR_DOWN);
            #endif
            break;
            
        case PowerMode::DEEP_SLEEP:
            disableUnusedPeripherals();
            #ifdef ARDUINO
            set_sleep_mode(SLEEP_MODE_PWR_DOWN);
            // Disable more peripherals for deep sleep
            power_all_disable();
            #endif
            break;
    }
}

void PowerManager::disableUnusedPeripherals() {
    #ifdef ARDUINO
    if (config_.disable_unused_peripherals) {
        // Disable unused peripherals to save power
        power_spi_disable();
        power_twi_disable();
        power_usart0_disable();
        power_timer1_disable();
        power_timer2_disable();
    }
    #endif
}

void PowerManager::enableEssentialPeripherals() {
    #ifdef ARDUINO
    // Enable essential peripherals
    power_adc_enable();
    power_timer0_enable(); // For millis() function
    #endif
}

void PowerManager::updatePowerStats() {
    auto now = std::chrono::steady_clock::now();
    auto current_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    
    // Update active/sleep time based on current mode
    if (current_mode_ == PowerMode::ACTIVE || current_mode_ == PowerMode::LOW_POWER) {
        stats_.total_active_time_ms = current_time_ms;
    } else {
        stats_.total_sleep_time_ms = current_time_ms;
    }
    
    // Update current consumption estimate
    stats_.average_current_ma = calculateCurrentConsumption();
    
    if (stats_.average_current_ma > stats_.peak_current_ma) {
        stats_.peak_current_ma = stats_.average_current_ma;
    }
}

float PowerManager::calculateCurrentConsumption() const {
    // Estimate current consumption based on power mode
    switch (current_mode_) {
        case PowerMode::ACTIVE:
            return 15.0f; // ~15mA in active mode
        case PowerMode::LOW_POWER:
            return 5.0f;  // ~5mA in low power mode
        case PowerMode::SLEEP:
            return 0.5f;  // ~0.5mA in sleep mode
        case PowerMode::DEEP_SLEEP:
            return 0.1f;  // ~0.1mA in deep sleep mode
        default:
            return 0.0f;
    }
}

} // namespace LightSensor
