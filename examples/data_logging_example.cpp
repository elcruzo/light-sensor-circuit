#include "light_sensor.h"
#include "data_logger.h"
#include "logger.h"
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
    
    logger.info("Starting data logging example");
    
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
    sensor_config.sample_rate_ms = 500;
    sensor_config.oversampling = 4;
    sensor_config.auto_gain = false;
    sensor_config.low_power_mode = false;
    sensor_config.sleep_duration_ms = 0;
    
    // Configure data logger
    LoggerConfig logger_config;
    logger_config.log_file_path = "/tmp/light_sensor_logs";
    logger_config.buffer_size = 50;
    logger_config.flush_threshold = 25;
    logger_config.enable_compression = false;
    logger_config.enable_timestamp = true;
    logger_config.min_lux_threshold = 0.0f;
    logger_config.max_lux_threshold = 100000.0f;
    logger_config.filter_noise = true;
    logger_config.min_quality_threshold = 30;
    logger_config.max_file_size_bytes = 1024 * 1024; // 1MB
    logger_config.max_log_days = 7;
    logger_config.enable_rotation = true;
    
    // Create sensor
    ADCLightSensor sensor(sensor_config);
    if (!sensor.initialize()) {
        logger.error("Failed to initialize sensor");
        return 1;
    }
    
    // Create data logger
    DataLogger data_logger(logger_config);
    if (!data_logger.initialize()) {
        logger.error("Failed to initialize data logger");
        return 1;
    }
    
    logger.info("Components initialized successfully");
    
    // Perform calibration
    logger.info("Performing sensor calibration...");
    logger.info("Please cover the sensor for dark calibration");
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    SensorReading dark_reading = sensor.read();
    logger.info("Dark reading: " + std::to_string(dark_reading.lux_value) + " lux");
    
    logger.info("Please expose sensor to bright light for light calibration");
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    SensorReading light_reading = sensor.read();
    logger.info("Light reading: " + std::to_string(light_reading.lux_value) + " lux");
    
    // Calibrate sensor
    sensor.calibrate(dark_reading.raw_value, light_reading.raw_value);
    logger.info("Sensor calibration completed");
    
    // Start data logging
    logger.info("Starting data logging...");
    data_logger.startLogging(std::make_shared<ADCLightSensor>(sensor));
    
    // Log some manual readings
    logger.info("Logging manual readings...");
    for (int i = 0; i < 10; ++i) {
        SensorReading reading = sensor.read();
        data_logger.logReading(reading);
        
        logger.info("Manual reading " + std::to_string(i + 1) + 
                   " - Lux: " + std::to_string(reading.lux_value) + 
                   ", Quality: " + std::to_string(reading.quality));
        
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    
    // Let automatic logging run for a while
    logger.info("Running automatic logging for 30 seconds...");
    std::this_thread::sleep_for(std::chrono::seconds(30));
    
    // Stop data logging
    logger.info("Stopping data logging...");
    data_logger.stopLogging();
    
    // Get final statistics
    DataStats stats = data_logger.getStats();
    logger.info("=== Data Logging Statistics ===");
    logger.info("Total readings: " + std::to_string(stats.total_readings));
    logger.info("Valid readings: " + std::to_string(stats.valid_readings));
    logger.info("Filtered readings: " + std::to_string(stats.filtered_readings));
    logger.info("Min lux: " + std::to_string(stats.min_lux));
    logger.info("Max lux: " + std::to_string(stats.max_lux));
    logger.info("Average lux: " + std::to_string(stats.average_lux));
    logger.info("Buffer overflows: " + std::to_string(stats.buffer_overflow_count));
    logger.info("Current buffer size: " + std::to_string(stats.current_buffer_size));
    
    // Test different storage types
    logger.info("Testing different storage types...");
    
    // Test memory storage
    LoggerConfig memory_config = logger_config;
    memory_config.buffer_size = 20;
    
    auto memory_storage = std::make_unique<MemoryDataStorage>(memory_config);
    memory_storage->initialize();
    
    DataLogger memory_logger(memory_config);
    memory_logger.setStorage(std::move(memory_storage));
    memory_logger.initialize();
    
    // Log some data to memory
    for (int i = 0; i < 5; ++i) {
        SensorReading reading = sensor.read();
        memory_logger.logReading(reading);
    }
    
    // Get data from memory storage
    // Note: getStorage() method not implemented in current version
    // auto memory_data = static_cast<MemoryDataStorage*>(memory_logger.getStorage())->getData();
    logger.info("Memory storage test completed");
    // logger.info("Memory storage contains " + std::to_string(memory_data.size()) + " readings");
    
    logger.info("Data logging example completed");
    return 0;
}
