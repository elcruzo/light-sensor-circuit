#include "power_manager.h"
#include <Arduino.h>
#include <esp_sleep.h>
#include <esp_wifi.h>
#include <driver/adc.h>
#include <algorithm>

namespace LightSensor {

PowerManager::PowerManager(const PowerConfig& config)
    : config_(config), current_mode_(PowerMode::ACTIVE), 
      wake_on_light_enabled_(false), last_light_level_(0.0f),
      last_activity_time_ms_(0), sleep_start_time_ms_(0) {
    
    // Initialize power statistics
    stats_ = {0, 0, 0, 0.0f, 0.0f, 0.0f, 100};
}

bool PowerManager::initialize() {
    last_activity_time_ms_ = millis();
    
    // Check wake reason
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    if (wakeup_reason != ESP_SLEEP_WAKEUP_UNDEFINED) {
        stats_.wake_count++;
    }
    
    return true;
}

void PowerManager::setPowerMode(PowerMode mode) {
    if (mode == current_mode_) {
        return;
    }
    
    current_mode_ = mode;
    configureHardwareForMode(mode);
    
    if (event_callback_) {
        event_callback_(mode, WakeSource::TIMER);
    }
    
    updatePowerStats();
}

PowerMode PowerManager::getCurrentMode() const {
    return current_mode_;
}

void PowerManager::sleep(uint32_t duration_ms, WakeSource wake_source) {
    if (current_mode_ == PowerMode::ACTIVE) {
        setPowerMode(PowerMode::SLEEP);
    }
    
    sleep_start_time_ms_ = millis();
    
    // Configure wake-up source
    if (wake_source == WakeSource::TIMER) {
        esp_sleep_enable_timer_wakeup(duration_ms * 1000ULL);  // Convert to microseconds
    }
    
    if (wake_on_light_enabled_ && config_.enable_wake_on_light) {
        // Configure GPIO wake-up for light sensor interrupt (if using interrupt-capable sensor)
        // This requires external comparator circuit
    }
    
    // Enter light sleep (maintains RAM, faster wake)
    esp_light_sleep_start();
    
    // Execution resumes here after wake-up
    wakeUp(wake_source);
}

void PowerManager::deepSleep(uint32_t duration_ms) {
    setPowerMode(PowerMode::DEEP_SLEEP);
    sleep_start_time_ms_ = millis();
    
    // Configure timer wake-up
    esp_sleep_enable_timer_wakeup(duration_ms * 1000ULL);
    
    // Enter deep sleep (RAM lost, slower wake but lowest power)
    esp_deep_sleep_start();
    
    // Execution never reaches here - CPU resets on wake
}

void PowerManager::wakeUp(WakeSource source) {
    if (current_mode_ == PowerMode::SLEEP || current_mode_ == PowerMode::DEEP_SLEEP) {
        setPowerMode(PowerMode::ACTIVE);
        stats_.wake_count++;
    }
    
    last_activity_time_ms_ = millis();
    
    if (event_callback_) {
        event_callback_(current_mode_, source);
    }
}

bool PowerManager::shouldEnterLowPower() const {
    uint32_t time_since_activity = millis() - last_activity_time_ms_;
    return time_since_activity > config_.sleep_timeout_ms;
}

void PowerManager::optimizePowerConsumption() {
    if (shouldEnterLowPower()) {
        if (current_mode_ == PowerMode::ACTIVE) {
            setPowerMode(PowerMode::LOW_POWER);
        } else if (current_mode_ == PowerMode::LOW_POWER) {
            uint32_t time_since_activity = millis() - last_activity_time_ms_;
            
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
    
    // Calculate battery percentage (assuming 3.0V - 4.2V LiPo range)
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

void PowerManager::recordActivity() {
    last_activity_time_ms_ = millis();
}

void PowerManager::process() {
    updatePowerStats();
    
    // Check for low battery conditions
    if (isBatteryCritical()) {
        deepSleep(60000);  // Deep sleep for 1 minute to conserve power
    } else if (isBatteryLow()) {
        setPowerMode(PowerMode::LOW_POWER);
    } else {
        optimizePowerConsumption();
    }
}

void PowerManager::configureHardwareForMode(PowerMode mode) {
    switch (mode) {
        case PowerMode::ACTIVE:
            // Full power - enable all peripherals
            setCpuFrequency(240);  // Full speed
            adc_power_on();
            break;
            
        case PowerMode::LOW_POWER:
            // Reduced power - lower CPU frequency
            setCpuFrequency(80);  // Reduced speed
            if (config_.disable_unused_peripherals) {
                disableUnusedPeripherals();
            }
            break;
            
        case PowerMode::SLEEP:
            // Light sleep preparation
            setCpuFrequency(80);
            disableUnusedPeripherals();
            break;
            
        case PowerMode::DEEP_SLEEP:
            // Deep sleep preparation - disable everything possible
            disableUnusedPeripherals();
            adc_power_off();
            break;
    }
}

void PowerManager::setCpuFrequency(uint32_t freq_mhz) {
    setCpuFrequencyMhz(freq_mhz);
}

void PowerManager::disableUnusedPeripherals() {
    if (config_.disable_unused_peripherals) {
        // Disable WiFi if not needed
        esp_wifi_stop();
        
        // Disable Bluetooth if not needed
        // btStop();  // Uncomment if using Bluetooth
    }
}

void PowerManager::enableEssentialPeripherals() {
    adc_power_on();
}

void PowerManager::updatePowerStats() {
    uint32_t current_time_ms = millis();
    
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
    // ESP32 typical current consumption by mode
    switch (current_mode_) {
        case PowerMode::ACTIVE:
            return 80.0f;   // ~80mA at 240MHz with WiFi off
        case PowerMode::LOW_POWER:
            return 20.0f;   // ~20mA at 80MHz
        case PowerMode::SLEEP:
            return 0.8f;    // ~0.8mA in light sleep
        case PowerMode::DEEP_SLEEP:
            return 0.01f;   // ~10uA in deep sleep
        default:
            return 0.0f;
    }
}

}  // namespace LightSensor
