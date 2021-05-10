#include "light_sensor.h"
#include "power_manager.h"
#include "data_logger.h"
#include "signal_processor.h"
#include "config_manager.h"
#include "logger.h"
#include "timer.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <memory>

using namespace LightSensor;

int main() {
    // Initialize logger
    Logger& logger = Logger::getInstance();
    logger.setLevel(LogLevel::INFO);
    logger.setOutput(LogOutput::CONSOLE);
    
    logger.info("Starting complete system example");
    
    // Initialize configuration manager
    ConfigManager config_manager("config.json");
    if (!config_manager.initialize()) {
        logger.error("Failed to initialize configuration manager");
        return 1;
    }
    
    // Load configuration
    SystemConfig config = config_manager.getConfig();
    logger.info("Configuration loaded");
    
    // Create sensor
    auto sensor = std::make_shared<ADCLightSensor>(config.sensor);
    if (!sensor->initialize()) {
        logger.error("Failed to initialize sensor");
        return 1;
    }
    
    // Create power manager
    PowerManager power_manager(config.power);
    if (!power_manager.initialize()) {
        logger.error("Failed to initialize power manager");
        return 1;
    }
    
    // Create signal processor
    SignalProcessor signal_processor(config.signal);
    
    // Create data logger
    DataLogger data_logger(config.logger);
    if (!data_logger.initialize()) {
        logger.error("Failed to initialize data logger");
        return 1;
    }
    
    logger.info("All components initialized successfully");
    
    // Set up power event callback
    power_manager.setPowerEventCallback([](PowerMode mode, WakeSource source) {
        Logger& logger = Logger::getInstance();
        std::string mode_str = (mode == PowerMode::ACTIVE) ? "ACTIVE" :
                              (mode == PowerMode::LOW_POWER) ? "LOW_POWER" :
                              (mode == PowerMode::SLEEP) ? "SLEEP" : "DEEP_SLEEP";
        logger.info("Power mode changed to: " + mode_str);
    });
    
    // Perform sensor calibration
    logger.info("Performing sensor calibration...");
    logger.info("Please cover the sensor for dark calibration");
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    SensorReading dark_reading = sensor->read();
    logger.info("Dark reading: " + std::to_string(dark_reading.lux_value) + " lux");
    
    logger.info("Please expose sensor to bright light for light calibration");
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    SensorReading light_reading = sensor->read();
    logger.info("Light reading: " + std::to_string(light_reading.lux_value) + " lux");
    
    // Calibrate sensor
    if (config_manager.calibrateSensor(dark_reading.raw_value, light_reading.raw_value, 1000.0f)) {
        logger.info("Sensor calibration completed and saved");
    } else {
        logger.warning("Sensor calibration failed");
    }
    
    // Start data logging
    logger.info("Starting data logging...");
    data_logger.startLogging(sensor);
    
    // Main loop
    logger.info("Starting main loop...");
    Timer main_timer;
    Timer sample_timer;
    Timer power_timer;
    
    const uint32_t SAMPLE_INTERVAL_MS = 1000;  // 1 second
    const uint32_t POWER_CHECK_INTERVAL_MS = 5000;  // 5 seconds
    const uint32_t TOTAL_RUNTIME_MS = 60000;  // 1 minute
    
    while (main_timer.elapsedMs() < TOTAL_RUNTIME_MS) {
        // Sample sensor
        if (sample_timer.hasElapsed(SAMPLE_INTERVAL_MS)) {
            SensorReading reading = sensor->read();
            
            // Process signal
            SignalAnalysis analysis = signal_processor.processReading(reading);
            
            // Log processed data
            SensorReading processed_reading = reading;
            processed_reading.lux_value = analysis.filtered_value;
            processed_reading.quality = analysis.quality_score;
            
            data_logger.logReading(processed_reading);
            
            // Log analysis results
            logger.info("Sample - Raw: " + std::to_string(reading.lux_value) + 
                       " lux, Filtered: " + std::to_string(analysis.filtered_value) + 
                       " lux, Quality: " + std::to_string(analysis.quality_score) + 
                       ", SNR: " + std::to_string(analysis.signal_to_noise_ratio));
            
            if (analysis.is_outlier) {
                logger.warning("Outlier detected in reading");
            }
            
            if (analysis.is_peak) {
                logger.info("Peak detected in reading");
            }
            
            if (analysis.trend_confidence > 0.7f) {
                std::string trend = analysis.trend_slope > 0 ? "increasing" : "decreasing";
                logger.info("Trend detected: " + trend + " (confidence: " + 
                           std::to_string(analysis.trend_confidence) + ")");
            }
            
            sample_timer.reset();
        }
        
        // Check power management
        if (power_timer.hasElapsed(POWER_CHECK_INTERVAL_MS)) {
            power_manager.process();
            
            // Update battery voltage (mock)
            float battery_voltage = 3.7f; // Mock battery voltage
            power_manager.updateBatteryVoltage(battery_voltage);
            
            if (power_manager.isBatteryLow()) {
                logger.warning("Battery is low");
            }
            
            if (power_manager.isBatteryCritical()) {
                logger.error("Battery is critical - entering deep sleep");
                power_manager.setPowerMode(PowerMode::DEEP_SLEEP);
            }
            
            power_timer.reset();
        }
        
        // Process data logger
        data_logger.process();
        
        // Small delay to prevent excessive CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Stop data logging
    logger.info("Stopping data logging...");
    data_logger.stopLogging();
    
    // Get final statistics
    DataStats stats = data_logger.getStats();
    PowerStats power_stats = power_manager.getPowerStats();
    
    logger.info("=== Final Statistics ===");
    logger.info("Total readings: " + std::to_string(stats.total_readings));
    logger.info("Valid readings: " + std::to_string(stats.valid_readings));
    logger.info("Filtered readings: " + std::to_string(stats.filtered_readings));
    logger.info("Min lux: " + std::to_string(stats.min_lux));
    logger.info("Max lux: " + std::to_string(stats.max_lux));
    logger.info("Average lux: " + std::to_string(stats.average_lux));
    logger.info("Buffer overflows: " + std::to_string(stats.buffer_overflow_count));
    
    logger.info("=== Power Statistics ===");
    logger.info("Total active time: " + std::to_string(power_stats.total_active_time_ms) + " ms");
    logger.info("Total sleep time: " + std::to_string(power_stats.total_sleep_time_ms) + " ms");
    logger.info("Wake count: " + std::to_string(power_stats.wake_count));
    logger.info("Average current: " + std::to_string(power_stats.average_current_ma) + " mA");
    logger.info("Peak current: " + std::to_string(power_stats.peak_current_ma) + " mA");
    logger.info("Battery voltage: " + std::to_string(power_stats.battery_voltage) + " V");
    logger.info("Battery percentage: " + std::to_string(power_stats.battery_percentage) + "%");
    
    logger.info("Complete system example completed");
    return 0;
}
