#include "data_logger.h"
#include "light_sensor.h"
#include <cassert>
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>

using namespace LightSensor;

void testDataLoggerInitialization() {
    std::cout << "Testing data logger initialization..." << std::endl;
    
    LoggerConfig config;
    config.log_file_path = "/tmp/test_logs";
    config.buffer_size = 10;
    config.flush_threshold = 5;
    config.enable_compression = false;
    config.enable_timestamp = true;
    config.min_lux_threshold = 0.0f;
    config.max_lux_threshold = 100000.0f;
    config.filter_noise = false;
    config.min_quality_threshold = 0;
    config.max_file_size_bytes = 1024 * 1024;
    config.max_log_days = 30;
    config.enable_rotation = false;
    
    DataLogger logger(config);
    
    // Test initialization
    assert(logger.initialize() == true);
    assert(logger.isLogging() == false);
    
    std::cout << "✓ Data logger initialization passed" << std::endl;
}

void testDataLogging() {
    std::cout << "Testing data logging..." << std::endl;
    
    LoggerConfig config;
    config.buffer_size = 10;
    config.flush_threshold = 5;
    config.min_quality_threshold = 0;
    
    DataLogger logger(config);
    logger.initialize();
    
    // Create test readings
    SensorReading reading1;
    reading1.timestamp_ms = 1000;
    reading1.raw_value = 0.5f;
    reading1.lux_value = 100.0f;
    reading1.voltage = 1.65f;
    reading1.is_valid = true;
    reading1.quality = 80;
    
    SensorReading reading2;
    reading2.timestamp_ms = 2000;
    reading2.raw_value = 0.6f;
    reading2.lux_value = 120.0f;
    reading2.voltage = 1.98f;
    reading2.is_valid = true;
    reading2.quality = 85;
    
    // Test logging
    assert(logger.logReading(reading1) == true);
    assert(logger.logReading(reading2) == true);
    
    // Test statistics
    DataStats stats = logger.getStats();
    assert(stats.total_readings >= 2);
    assert(stats.valid_readings >= 2);
    
    std::cout << "✓ Data logging passed" << std::endl;
}

void testDataLoggerWithSensor() {
    std::cout << "Testing data logger with sensor..." << std::endl;
    
    // Create sensor config
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
    sensor_config.sample_rate_ms = 100;
    sensor_config.oversampling = 1;
    sensor_config.auto_gain = false;
    sensor_config.low_power_mode = false;
    sensor_config.sleep_duration_ms = 0;
    
    // Create logger config
    LoggerConfig logger_config;
    logger_config.buffer_size = 5;
    logger_config.flush_threshold = 3;
    logger_config.min_quality_threshold = 0;
    
    // Create components
    auto sensor = std::make_shared<ADCLightSensor>(sensor_config);
    DataLogger logger(logger_config);
    
    // Initialize
    sensor->initialize();
    logger.initialize();
    
    // Start logging
    logger.startLogging(sensor);
    assert(logger.isLogging() == true);
    
    // Let it run briefly
    #ifdef ARDUINO
    delay(500);
    #else
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    #endif
    
    // Stop logging
    logger.stopLogging();
    assert(logger.isLogging() == false);
    
    // Check statistics
    DataStats stats = logger.getStats();
    assert(stats.total_readings > 0);
    
    std::cout << "✓ Data logger with sensor passed" << std::endl;
}

void testDataStats() {
    std::cout << "Testing data statistics..." << std::endl;
    
    LoggerConfig config;
    config.buffer_size = 10;
    config.flush_threshold = 5;
    config.min_quality_threshold = 0;
    
    DataLogger logger(config);
    logger.initialize();
    
    // Log some test data
    for (int i = 0; i < 5; ++i) {
        SensorReading reading;
        reading.timestamp_ms = i * 1000;
        reading.raw_value = 0.5f + i * 0.1f;
        reading.lux_value = 100.0f + i * 20.0f;
        reading.voltage = 1.65f + i * 0.33f;
        reading.is_valid = true;
        reading.quality = 70 + i * 5;
        
        logger.logReading(reading);
    }
    
    // Get statistics
    DataStats stats = logger.getStats();
    assert(stats.total_readings >= 5);
    assert(stats.valid_readings >= 5);
    assert(stats.min_lux > 0.0f);
    assert(stats.max_lux > stats.min_lux);
    assert(stats.average_lux > 0.0f);
    
    std::cout << "✓ Data statistics passed" << std::endl;
    std::cout << "  Total readings: " << stats.total_readings << std::endl;
    std::cout << "  Valid readings: " << stats.valid_readings << std::endl;
    std::cout << "  Min lux: " << stats.min_lux << std::endl;
    std::cout << "  Max lux: " << stats.max_lux << std::endl;
    std::cout << "  Average lux: " << stats.average_lux << std::endl;
}

int runDataLoggerTests() {
    try {
        testDataLoggerInitialization();
        testDataLogging();
        testDataLoggerWithSensor();
        testDataStats();
        
        return 0;
    } catch (const std::exception& e) {
        std::cout << "\n✗ Test failed: " << e.what() << std::endl;
        return 1;
    }
}
