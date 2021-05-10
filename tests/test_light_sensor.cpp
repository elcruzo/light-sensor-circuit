#include "light_sensor.h"
#include <cassert>
#include <iostream>
#include <thread>
#include <chrono>

using namespace LightSensor;

void testSensorInitialization() {
    std::cout << "Testing sensor initialization..." << std::endl;
    
    SensorConfig config;
    #ifdef ARDUINO
    config.adc_pin = A0;
    #else
    config.adc_pin = 0; // Mock pin for desktop
    #endif
    config.adc_resolution = 10;
    config.reference_voltage = 3.3f;
    config.dark_offset = 0.0f;
    config.sensitivity = 1.0f;
    config.noise_threshold = 0.01f;
    config.sample_rate_ms = 1000;
    config.oversampling = 4;
    config.auto_gain = false;
    config.low_power_mode = false;
    config.sleep_duration_ms = 0;
    
    ADCLightSensor sensor(config);
    
    // Test initialization
    assert(sensor.initialize() == true);
    std::cout << "✓ Sensor initialization passed" << std::endl;
}

void testSensorReading() {
    std::cout << "Testing sensor reading..." << std::endl;
    
    SensorConfig config;
    #ifdef ARDUINO
    config.adc_pin = A0;
    #else
    config.adc_pin = 0; // Mock pin for desktop
    #endif
    config.adc_resolution = 10;
    config.reference_voltage = 3.3f;
    config.dark_offset = 0.0f;
    config.sensitivity = 1.0f;
    config.noise_threshold = 0.01f;
    config.sample_rate_ms = 1000;
    config.oversampling = 1; // Use 1 for testing
    config.auto_gain = false;
    config.low_power_mode = false;
    config.sleep_duration_ms = 0;
    
    ADCLightSensor sensor(config);
    sensor.initialize();
    
    // Test reading
    SensorReading reading = sensor.read();
    
    // Basic validation
    assert(reading.raw_value >= 0.0f && reading.raw_value <= 1.0f);
    assert(reading.voltage >= 0.0f && reading.voltage <= config.reference_voltage);
    assert(reading.lux_value >= 0.0f);
    assert(reading.quality >= 0 && reading.quality <= 100);
    
    std::cout << "✓ Sensor reading passed" << std::endl;
    std::cout << "  Raw value: " << reading.raw_value << std::endl;
    std::cout << "  Voltage: " << reading.voltage << "V" << std::endl;
    std::cout << "  Lux: " << reading.lux_value << " lux" << std::endl;
    std::cout << "  Quality: " << static_cast<int>(reading.quality) << std::endl;
}

void testSensorCalibration() {
    std::cout << "Testing sensor calibration..." << std::endl;
    
    SensorConfig config;
    #ifdef ARDUINO
    config.adc_pin = A0;
    #else
    config.adc_pin = 0; // Mock pin for desktop
    #endif
    config.adc_resolution = 10;
    config.reference_voltage = 3.3f;
    config.dark_offset = 0.0f;
    config.sensitivity = 1.0f;
    config.noise_threshold = 0.01f;
    config.sample_rate_ms = 1000;
    config.oversampling = 1;
    config.auto_gain = false;
    config.low_power_mode = false;
    config.sleep_duration_ms = 0;
    
    ADCLightSensor sensor(config);
    sensor.initialize();
    
    // Test calibration
    float dark_value = 0.1f;
    float light_value = 0.8f;
    
    sensor.calibrate(dark_value, light_value);
    
    // Test reading after calibration
    SensorReading reading = sensor.read();
    
    // Basic validation
    assert(reading.raw_value >= 0.0f && reading.raw_value <= 1.0f);
    assert(reading.voltage >= 0.0f && reading.voltage <= config.reference_voltage);
    assert(reading.lux_value >= 0.0f);
    
    std::cout << "✓ Sensor calibration passed" << std::endl;
}

void testSensorSampling() {
    std::cout << "Testing sensor sampling..." << std::endl;
    
    SensorConfig config;
    #ifdef ARDUINO
    config.adc_pin = A0;
    #else
    config.adc_pin = 0; // Mock pin for desktop
    #endif
    config.adc_resolution = 10;
    config.reference_voltage = 3.3f;
    config.dark_offset = 0.0f;
    config.sensitivity = 1.0f;
    config.noise_threshold = 0.01f;
    config.sample_rate_ms = 100;
    config.oversampling = 1;
    config.auto_gain = false;
    config.low_power_mode = false;
    config.sleep_duration_ms = 0;
    
    ADCLightSensor sensor(config);
    sensor.initialize();
    
    // Test sampling callback
    int callback_count = 0;
    sensor.startSampling([&callback_count](const SensorReading& reading) {
        callback_count++;
        assert(reading.raw_value >= 0.0f && reading.raw_value <= 1.0f);
    });
    
    // Let it run for a short time
    #ifdef ARDUINO
    delay(500);
    #else
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    #endif
    
    sensor.stopSampling();
    
    // Should have received some callbacks
    assert(callback_count > 0);
    
    std::cout << "✓ Sensor sampling passed (callbacks: " << callback_count << ")" << std::endl;
}

int runLightSensorTests() {
    try {
        testSensorInitialization();
        testSensorReading();
        testSensorCalibration();
        testSensorSampling();
        
        return 0;
    } catch (const std::exception& e) {
        std::cout << "\n✗ Test failed: " << e.what() << std::endl;
        return 1;
    }
}
