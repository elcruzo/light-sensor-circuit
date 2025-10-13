#include "light_sensor.h"
#include <Arduino.h>
#include <algorithm>
#include <cmath>

namespace LightSensor {

// ESP32 ADC configuration
static const uint8_t ADC_WIDTH = 12;  // 12-bit ADC
static const uint16_t ADC_MAX_VALUE = 4095;

ADCLightSensor::ADCLightSensor(const SensorConfig& config)
    : config_(config), is_sampling_(false), is_initialized_(false),
      last_sample_time_ms_(0), was_sampling_before_sleep_(false) {
    // Initialize filter buffer
    for (size_t i = 0; i < FILTER_BUFFER_SIZE; ++i) {
        filter_buffer_[i] = 0.0f;
    }
    filter_buffer_index_ = 0;
}

bool ADCLightSensor::initialize() {
    if (is_initialized_) {
        return true;
    }
    
    // Validate configuration
    if (config_.adc_resolution == 0 || config_.reference_voltage <= 0.0f) {
        return false;
    }
    
    // Validate ADC pin is a valid ESP32 ADC pin (GPIO 32-39 for ADC1)
    if (config_.adc_pin < 32 || config_.adc_pin > 39) {
        return false;
    }
    
    // Configure ADC
    analogReadResolution(ADC_WIDTH);
    analogSetAttenuation(ADC_11db);  // Full range 0-3.3V
    
    // Set pin mode
    pinMode(config_.adc_pin, INPUT);
    
    // Test read to verify ADC is working
    int test_value = analogRead(config_.adc_pin);
    if (test_value < 0) {
        return false;
    }
    
    is_initialized_ = true;
    return true;
}

SensorReading ADCLightSensor::read() {
    SensorReading reading = {0, 0.0f, 0.0f, 0.0f, false, 0};
    
    if (!is_initialized_) {
        return reading;
    }
    
    reading.timestamp_ms = millis();
    
    // Perform oversampling for noise reduction
    float sum = 0.0f;
    for (uint8_t i = 0; i < config_.oversampling; ++i) {
        sum += readRawADC();
        if (i < config_.oversampling - 1) {
            delayMicroseconds(100);  // Small delay between samples
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
    last_sample_time_ms_ = millis();
}

void ADCLightSensor::stopSampling() {
    is_sampling_ = false;
    data_callback_ = nullptr;
}

void ADCLightSensor::configure(const SensorConfig& config) {
    config_ = config;
    // Re-initialize if already initialized
    if (is_initialized_) {
        is_initialized_ = false;
        initialize();
    }
}

void ADCLightSensor::calibrate(float dark_value, float light_value) {
    if (dark_value >= light_value) {
        return;  // Invalid calibration values
    }
    
    // Update calibration parameters
    config_.dark_offset = dark_value;
    config_.sensitivity = (light_value - dark_value) / 1000.0f;  // Assume 1000 lux reference
    
    // Adjust noise threshold based on signal range
    config_.noise_threshold = (light_value - dark_value) * 0.01f;  // 1% of signal range
}

void ADCLightSensor::enterLowPower() {
    was_sampling_before_sleep_ = is_sampling_;
    
    if (is_sampling_) {
        stopSampling();
    }
    
    // Disable ADC to save power
    adc_power_off();
}

void ADCLightSensor::wakeUp() {
    // Re-enable ADC
    adc_power_on();
    
    // Small delay for ADC to stabilize
    delay(10);
    
    // Resume sampling if it was active before sleep
    if (was_sampling_before_sleep_ && data_callback_) {
        is_sampling_ = true;
        last_sample_time_ms_ = millis();
    }
}

void ADCLightSensor::process() {
    if (!is_sampling_ || !data_callback_) {
        return;
    }
    
    uint32_t now = millis();
    if (now - last_sample_time_ms_ >= config_.sample_rate_ms) {
        SensorReading reading = read();
        data_callback_(reading);
        last_sample_time_ms_ = now;
    }
}

float ADCLightSensor::readRawADC() {
    int raw_value = analogRead(config_.adc_pin);
    return static_cast<float>(raw_value) / ADC_MAX_VALUE;
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
    filter_buffer_[filter_buffer_index_] = reading;
    filter_buffer_index_ = (filter_buffer_index_ + 1) % FILTER_BUFFER_SIZE;
    
    float sum = 0.0f;
    for (size_t i = 0; i < FILTER_BUFFER_SIZE; ++i) {
        sum += filter_buffer_[i];
    }
    
    return sum / FILTER_BUFFER_SIZE;
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

}  // namespace LightSensor
