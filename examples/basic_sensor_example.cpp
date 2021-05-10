#include "light_sensor.h"
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
    
    logger.info("Starting basic sensor example");
    
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
    sensor_config.low_power_mode = false;
    sensor_config.sleep_duration_ms = 0;
    
    // Create sensor
    ADCLightSensor sensor(sensor_config);
    
    // Initialize sensor
    if (!sensor.initialize()) {
        logger.error("Failed to initialize sensor");
        return 1;
    }
    
    logger.info("Sensor initialized successfully");
    
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
    
    // Start continuous sampling
    logger.info("Starting continuous sampling...");
    sensor.startSampling([](const SensorReading& reading) {
        Logger& logger = Logger::getInstance();
        logger.info("Reading - Raw: " + std::to_string(reading.raw_value) + 
                   ", Lux: " + std::to_string(reading.lux_value) + 
                   ", Voltage: " + std::to_string(reading.voltage) + 
                   ", Quality: " + std::to_string(reading.quality));
    });
    
    // Run for 30 seconds
    logger.info("Running for 30 seconds...");
    std::this_thread::sleep_for(std::chrono::seconds(30));
    
    // Stop sampling
    sensor.stopSampling();
    logger.info("Sampling stopped");
    
    // Test low power mode
    logger.info("Testing low power mode...");
    sensor.enterLowPower();
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    sensor.wakeUp();
    logger.info("Woke up from low power mode");
    
    // Final reading
    SensorReading final_reading = sensor.read();
    logger.info("Final reading: " + std::to_string(final_reading.lux_value) + " lux");
    
    logger.info("Basic sensor example completed");
    return 0;
}
