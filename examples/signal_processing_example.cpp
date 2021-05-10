#include "light_sensor.h"
#include "signal_processor.h"
#include "logger.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>

using namespace LightSensor;

int main() {
    // Initialize logger
    Logger& logger = Logger::getInstance();
    logger.setLevel(LogLevel::INFO);
    logger.setOutput(LogOutput::CONSOLE);
    
    logger.info("Starting signal processing example");
    
    // Configure signal processing
    SignalConfig signal_config;
    signal_config.moving_average_window = 5;
    signal_config.low_pass_cutoff = 0.5f;
    signal_config.high_pass_cutoff = 0.01f;
    signal_config.enable_median_filter = true;
    signal_config.median_window = 3;
    signal_config.noise_threshold = 0.01f;
    signal_config.enable_outlier_removal = true;
    signal_config.outlier_threshold = 2.0f;
    signal_config.enable_trend_detection = true;
    signal_config.trend_window = 10;
    signal_config.enable_peak_detection = true;
    signal_config.peak_threshold = 0.1f;
    signal_config.enable_adaptive_filter = true;
    signal_config.adaptation_rate = 0.1f;
    signal_config.noise_floor = 0.001f;
    
    // Create signal processor
    SignalProcessor processor(signal_config);
    
    logger.info("Signal processor initialized successfully");
    
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
    sensor_config.sample_rate_ms = 100;
    sensor_config.oversampling = 1;
    sensor_config.auto_gain = false;
    sensor_config.low_power_mode = false;
    sensor_config.sleep_duration_ms = 0;
    
    // Create sensor
    ADCLightSensor sensor(sensor_config);
    if (!sensor.initialize()) {
        logger.error("Failed to initialize sensor");
        return 1;
    }
    
    logger.info("Sensor initialized successfully");
    
    // Test with synthetic data
    logger.info("Testing with synthetic data...");
    
    // Generate test data with known patterns
    std::vector<float> test_data = {
        100.0f, 102.0f, 98.0f, 105.0f, 103.0f,  // Normal variation
        200.0f,  // Outlier
        101.0f, 103.0f, 99.0f, 104.0f, 102.0f,  // Back to normal
        110.0f, 115.0f, 120.0f, 125.0f, 130.0f,  // Increasing trend
        135.0f, 140.0f, 145.0f, 150.0f, 155.0f,  // Continued increase
        160.0f,  // Peak
        155.0f, 150.0f, 145.0f, 140.0f, 135.0f,  // Decreasing trend
        130.0f, 125.0f, 120.0f, 115.0f, 110.0f   // Continued decrease
    };
    
    logger.info("Processing " + std::to_string(test_data.size()) + " synthetic readings...");
    
    for (size_t i = 0; i < test_data.size(); ++i) {
        SensorReading reading;
        reading.timestamp_ms = i * 100;
        reading.raw_value = test_data[i] / 1000.0f; // Convert to 0-1 range
        reading.lux_value = test_data[i];
        reading.voltage = test_data[i] * 0.0033f; // Convert to voltage
        reading.is_valid = true;
        reading.quality = 80;
        
        // Process signal
        SignalAnalysis analysis = processor.processReading(reading);
        
        logger.info("Reading " + std::to_string(i + 1) + 
                   " - Raw: " + std::to_string(reading.lux_value) + 
                   " lux, Filtered: " + std::to_string(analysis.filtered_value) + 
                   " lux, Quality: " + std::to_string(analysis.quality_score));
        
        if (analysis.is_outlier) {
            logger.warning("  -> OUTLIER DETECTED");
        }
        
        if (analysis.is_peak) {
            logger.info("  -> PEAK DETECTED");
        }
        
        if (analysis.trend_confidence > 0.7f) {
            std::string trend = analysis.trend_slope > 0 ? "increasing" : "decreasing";
            logger.info("  -> TREND: " + trend + " (confidence: " + 
                       std::to_string(analysis.trend_confidence) + ")");
        }
        
        logger.info("  -> SNR: " + std::to_string(analysis.signal_to_noise_ratio) + 
                   ", Noise: " + std::to_string(analysis.noise_level));
        
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    // Test with real sensor data
    logger.info("Testing with real sensor data...");
    logger.info("Please vary the light conditions on the sensor...");
    
    for (int i = 0; i < 20; ++i) {
        SensorReading reading = sensor.read();
        SignalAnalysis analysis = processor.processReading(reading);
        
        logger.info("Real reading " + std::to_string(i + 1) + 
                   " - Raw: " + std::to_string(reading.lux_value) + 
                   " lux, Filtered: " + std::to_string(analysis.filtered_value) + 
                   " lux, Quality: " + std::to_string(analysis.quality_score));
        
        if (analysis.is_outlier) {
            logger.warning("  -> OUTLIER DETECTED");
        }
        
        if (analysis.is_peak) {
            logger.info("  -> PEAK DETECTED");
        }
        
        if (analysis.trend_confidence > 0.7f) {
            std::string trend = analysis.trend_slope > 0 ? "increasing" : "decreasing";
            logger.info("  -> TREND: " + trend + " (confidence: " + 
                       std::to_string(analysis.trend_confidence) + ")");
        }
        
        logger.info("  -> SNR: " + std::to_string(analysis.signal_to_noise_ratio) + 
                   ", Noise: " + std::to_string(analysis.noise_level));
        
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    
    // Test different filter configurations
    logger.info("Testing different filter configurations...");
    
    // Test with minimal filtering
    SignalConfig minimal_config = signal_config;
    minimal_config.moving_average_window = 1;
    minimal_config.enable_median_filter = false;
    minimal_config.enable_outlier_removal = false;
    minimal_config.enable_trend_detection = false;
    minimal_config.enable_peak_detection = false;
    minimal_config.enable_adaptive_filter = false;
    
    SignalProcessor minimal_processor(minimal_config);
    
    logger.info("Testing minimal filtering...");
    for (int i = 0; i < 5; ++i) {
        SensorReading reading = sensor.read();
        SignalAnalysis analysis = minimal_processor.processReading(reading);
        
        logger.info("Minimal filter - Raw: " + std::to_string(reading.lux_value) + 
                   " lux, Filtered: " + std::to_string(analysis.filtered_value) + 
                   " lux, Quality: " + std::to_string(analysis.quality_score));
        
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    
    // Test with heavy filtering
    SignalConfig heavy_config = signal_config;
    heavy_config.moving_average_window = 10;
    heavy_config.median_window = 5;
    heavy_config.outlier_threshold = 1.5f; // Stricter outlier detection
    
    SignalProcessor heavy_processor(heavy_config);
    
    logger.info("Testing heavy filtering...");
    for (int i = 0; i < 5; ++i) {
        SensorReading reading = sensor.read();
        SignalAnalysis analysis = heavy_processor.processReading(reading);
        
        logger.info("Heavy filter - Raw: " + std::to_string(reading.lux_value) + 
                   " lux, Filtered: " + std::to_string(analysis.filtered_value) + 
                   " lux, Quality: " + std::to_string(analysis.quality_score));
        
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    
    // Test signal quality monitoring
    logger.info("Testing signal quality monitoring...");
    
    uint8_t quality = processor.getSignalQuality();
    float noise_level = processor.getNoiseLevel();
    
    logger.info("Current signal quality: " + std::to_string(quality));
    logger.info("Current noise level: " + std::to_string(noise_level));
    
    // Test filter enabling/disabling
    logger.info("Testing filter enabling/disabling...");
    
    processor.setFilterEnabled(FilterType::MOVING_AVERAGE, false);
    logger.info("Disabled moving average filter");
    
    processor.setFilterEnabled(FilterType::MEDIAN, false);
    logger.info("Disabled median filter");
    
    // Test with disabled filters
    for (int i = 0; i < 3; ++i) {
        SensorReading reading = sensor.read();
        SignalAnalysis analysis = processor.processReading(reading);
        
        logger.info("Disabled filters - Raw: " + std::to_string(reading.lux_value) + 
                   " lux, Filtered: " + std::to_string(analysis.filtered_value) + 
                   " lux, Quality: " + std::to_string(analysis.quality_score));
        
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    
    // Re-enable filters
    processor.setFilterEnabled(FilterType::MOVING_AVERAGE, true);
    processor.setFilterEnabled(FilterType::MEDIAN, true);
    logger.info("Re-enabled filters");
    
    logger.info("Signal processing example completed");
    return 0;
}
