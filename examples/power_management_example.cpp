#include "light_sensor.h"
#include "power_manager.h"
#include "logger.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace LightSensor;

int main() {
    // Initialize logger
    Logger& logger = Logger::getInstance();
    logger.setLevel(LogLevel::INFO);
    logger.setOutput(LogOutput::CONSOLE);
    
    logger.info("Starting power management example");
    
    // Configure power management
    PowerConfig power_config;
    power_config.sleep_timeout_ms = 5000;        // 5 seconds
    power_config.deep_sleep_timeout_ms = 15000;  // 15 seconds
    power_config.enable_wake_on_light = true;
    power_config.light_threshold = 0.1f;
    power_config.disable_unused_peripherals = true;
    power_config.reduce_clock_speed = true;
    power_config.adc_sample_delay_ms = 1;
    power_config.low_battery_threshold = 3.2f;
    power_config.critical_battery_threshold = 3.0f;
    power_config.enable_battery_monitoring = true;
    
    // Create power manager
    PowerManager power_manager(power_config);
    if (!power_manager.initialize()) {
        logger.error("Failed to initialize power manager");
        return 1;
    }
    
    // Set up power event callback
    power_manager.setPowerEventCallback([](PowerMode mode, WakeSource source) {
        Logger& logger = Logger::getInstance();
        std::string mode_str = (mode == PowerMode::ACTIVE) ? "ACTIVE" :
                              (mode == PowerMode::LOW_POWER) ? "LOW_POWER" :
                              (mode == PowerMode::SLEEP) ? "SLEEP" : "DEEP_SLEEP";
        std::string source_str = (source == WakeSource::TIMER) ? "TIMER" :
                                (source == WakeSource::LIGHT_CHANGE) ? "LIGHT_CHANGE" :
                                (source == WakeSource::BUTTON) ? "BUTTON" :
                                (source == WakeSource::EXTERNAL) ? "EXTERNAL" : "LOW_BATTERY";
        logger.info("Power event: " + mode_str + " (source: " + source_str + ")");
    });
    
    logger.info("Power manager initialized successfully");
    
    // Configure sensor
    SensorConfig sensor_config;
    #ifdef ARDUINO
    sensor_config.adc_pin = A0;
    #else
    sensor_config.adc_pin = 0; // Mock pin for desktop
    #endif
    sensor_config.adc_resolution = 10;
    sensor_config.reference_voltage = 3.3f;
    sensor_config.dark_offset = 0.0f;
    sensor_config.sensitivity = 1.0f;
    sensor_config.noise_threshold = 0.01f;
    sensor_config.sample_rate_ms = 1000;
    sensor_config.oversampling = 4;
    sensor_config.auto_gain = false;
    sensor_config.low_power_mode = true;
    sensor_config.sleep_duration_ms = 100;
    
    // Create sensor
    ADCLightSensor sensor(sensor_config);
    if (!sensor.initialize()) {
        logger.error("Failed to initialize sensor");
        return 1;
    }
    
    logger.info("Sensor initialized successfully");
    
    // Test different power modes
    logger.info("Testing different power modes...");
    
    // Test active mode
    logger.info("Setting to ACTIVE mode");
    power_manager.setPowerMode(PowerMode::ACTIVE);
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Test low power mode
    logger.info("Setting to LOW_POWER mode");
    power_manager.setPowerMode(PowerMode::LOW_POWER);
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Test sleep mode
    logger.info("Setting to SLEEP mode");
    power_manager.setPowerMode(PowerMode::SLEEP);
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Test deep sleep mode
    logger.info("Setting to DEEP_SLEEP mode");
    power_manager.setPowerMode(PowerMode::DEEP_SLEEP);
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Return to active mode
    logger.info("Returning to ACTIVE mode");
    power_manager.setPowerMode(PowerMode::ACTIVE);
    
    // Test battery monitoring
    logger.info("Testing battery monitoring...");
    
    // Simulate different battery levels
    std::vector<float> battery_levels = {4.0f, 3.7f, 3.3f, 3.1f, 2.9f, 2.7f};
    
    for (float voltage : battery_levels) {
        power_manager.updateBatteryVoltage(voltage);
        
        logger.info("Battery voltage: " + std::to_string(voltage) + "V");
        logger.info("  Low battery: " + std::string(power_manager.isBatteryLow() ? "YES" : "NO"));
        logger.info("  Critical battery: " + std::string(power_manager.isBatteryCritical() ? "YES" : "NO"));
        
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    
    // Test wake on light
    logger.info("Testing wake on light...");
    power_manager.setWakeOnLight(true, 0.2f);
    
    // Simulate light changes
    for (int i = 0; i < 5; ++i) {
        SensorReading reading = sensor.read();
        logger.info("Light reading: " + std::to_string(reading.lux_value) + " lux");
        
        // Check if wake on light would trigger
        if (reading.lux_value > 0.2f) {
            logger.info("  Light threshold exceeded - would wake up");
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    
    // Test power management processing
    logger.info("Testing power management processing...");
    
    // Simulate activity
    for (int i = 0; i < 10; ++i) {
        SensorReading reading = sensor.read();
        logger.info("Activity " + std::to_string(i + 1) + 
                   " - Lux: " + std::to_string(reading.lux_value) + 
                   ", Mode: " + (power_manager.getCurrentMode() == PowerMode::ACTIVE ? "ACTIVE" : "OTHER"));
        
        // Process power management
        power_manager.process();
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    
    // Test sleep functionality
    logger.info("Testing sleep functionality...");
    logger.info("Entering sleep for 3 seconds...");
    power_manager.sleep(3000, WakeSource::TIMER);
    logger.info("Woke up from sleep");
    
    // Get final power statistics
    PowerStats stats = power_manager.getPowerStats();
    logger.info("=== Power Management Statistics ===");
    logger.info("Total active time: " + std::to_string(stats.total_active_time_ms) + " ms");
    logger.info("Total sleep time: " + std::to_string(stats.total_sleep_time_ms) + " ms");
    logger.info("Wake count: " + std::to_string(stats.wake_count));
    logger.info("Average current: " + std::to_string(stats.average_current_ma) + " mA");
    logger.info("Peak current: " + std::to_string(stats.peak_current_ma) + " mA");
    logger.info("Battery voltage: " + std::to_string(stats.battery_voltage) + " V");
    logger.info("Battery percentage: " + std::to_string(stats.battery_percentage) + "%");
    
    logger.info("Power management example completed");
    return 0;
}
