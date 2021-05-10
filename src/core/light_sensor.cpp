#include "light_sensor.h"
#include <algorithm>
#include <cmath>
#include <chrono>
#include <thread>
#include <vector>

#ifdef ARDUINO
#include <Arduino.h>
#else
// Mock Arduino functions for testing
#define analogRead(pin) (rand() % 1024)
#define millis() (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count())
#define delay(ms) std::this_thread::sleep_for(std::chrono::milliseconds(ms))
#endif

namespace LightSensor {

ADCLightSensor::ADCLightSensor(const SensorConfig& config)
    : config_(config), is_sampling_(false), is_initialized_(false) {
}

bool ADCLightSensor::initialize() {
    if (is_initialized_) {
        return true;
    }
    
    // Initialize ADC pin
    #ifdef ARDUINO
    pinMode(config_.adc_pin, INPUT);
    #endif
    
    // Validate configuration
    if (config_.adc_resolution == 0 || config_.reference_voltage <= 0.0f) {
        return false;
    }
    
    is_initialized_ = true;
    return true;
}

SensorReading ADCLightSensor::read() {
    if (!is_initialized_) {
        return {0, 0.0f, 0.0f, 0.0f, false, 0};
    }
    
    SensorReading reading;
    reading.timestamp_ms = millis();
    
    // Perform oversampling for noise reduction
    float sum = 0.0f;
    for (uint8_t i = 0; i < config_.oversampling; ++i) {
        sum += readRawADC();
        if (i < config_.oversampling - 1) {
            delay(1); // Small delay between samples
        }
    }
    
    reading.raw_value = sum / config_.oversampling;
    reading.voltage = adcToVoltage(reading.raw_value);
    reading.lux_value = voltageToLux(reading.voltage);
    reading.is_valid = (reading.raw_value >= 0.0f && reading.raw_value <= 1.0f);
    reading.quality = calculateQuality(reading);
    
    // Apply noise filtering
    reading.lux_value = applyNoiseFilter(reading.lux_value);
    
    return reading;
}

void ADCLightSensor::startSampling(DataCallback callback) {
    if (!is_initialized_ || callback == nullptr) {
        return;
    }
    
    data_callback_ = callback;
    is_sampling_ = true;
    last_sample_time_ = std::chrono::steady_clock::now();
}

void ADCLightSensor::stopSampling() {
    is_sampling_ = false;
    data_callback_ = nullptr;
}

void ADCLightSensor::configure(const SensorConfig& config) {
    config_ = config;
    // Re-initialize if already initialized
    if (is_initialized_) {
        initialize();
    }
}

void ADCLightSensor::calibrate(float dark_value, float light_value) {
    if (dark_value >= light_value) {
        return; // Invalid calibration values
    }
    
    // Update calibration parameters
    config_.dark_offset = dark_value;
    config_.sensitivity = (light_value - dark_value) / 1000.0f; // Assume 1000 lux reference
    
    // Adjust noise threshold based on signal range
    config_.noise_threshold = (light_value - dark_value) * 0.01f; // 1% of signal range
}

void ADCLightSensor::enterLowPower() {
    if (is_sampling_) {
        stopSampling();
    }
    
    // In a real implementation, this would:
    // - Disable ADC
    // - Enter sleep mode
    // - Configure wake-up sources
}

void ADCLightSensor::wakeUp() {
    // In a real implementation, this would:
    // - Re-enable ADC
    // - Restore previous configuration
    // - Resume sampling if it was active
}

float ADCLightSensor::readRawADC() {
    #ifdef ARDUINO
    int raw_value = analogRead(config_.adc_pin);
    return static_cast<float>(raw_value) / ((1 << config_.adc_resolution) - 1);
    #else
    // Mock implementation for testing
    return static_cast<float>(rand()) / RAND_MAX;
    #endif
}

float ADCLightSensor::adcToVoltage(float raw_value) {
    return raw_value * config_.reference_voltage;
}

float ADCLightSensor::voltageToLux(float voltage) {
    // Apply dark current compensation
    float compensated_voltage = voltage - config_.dark_offset;
    
    if (compensated_voltage <= 0.0f) {
        return 0.0f;
    }
    
    // Convert to lux using calibrated sensitivity
    return compensated_voltage / config_.sensitivity;
}

float ADCLightSensor::applyNoiseFilter(float reading) {
    // Simple moving average filter for noise reduction
    static std::vector<float> filter_buffer(5, 0.0f);
    static size_t buffer_index = 0;
    
    filter_buffer[buffer_index] = reading;
    buffer_index = (buffer_index + 1) % filter_buffer.size();
    
    float sum = 0.0f;
    for (float value : filter_buffer) {
        sum += value;
    }
    
    return sum / filter_buffer.size();
}

uint8_t ADCLightSensor::calculateQuality(const SensorReading& reading) {
    if (!reading.is_valid) {
        return 0;
    }
    
    // Calculate quality based on signal strength and stability
    float signal_strength = reading.raw_value;
    float quality = signal_strength * 100.0f;
    
    // Reduce quality if signal is too low (noise dominated)
    if (signal_strength < 0.01f) {
        quality *= 0.5f;
    }
    
    // Reduce quality if signal is saturated
    if (signal_strength > 0.95f) {
        quality *= 0.8f;
    }
    
    return static_cast<uint8_t>(std::min(100.0f, std::max(0.0f, quality)));
}

} // namespace LightSensor
