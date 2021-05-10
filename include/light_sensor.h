#pragma once

#include <cstdint>
#include <functional>
#include <vector>
#include <chrono>

namespace LightSensor {

/**
 * @brief Light sensor reading data structure
 */
struct SensorReading {
    uint32_t timestamp_ms;    // Timestamp in milliseconds
    float raw_value;          // Raw ADC value (0.0 - 1.0)
    float lux_value;          // Converted lux value
    float voltage;            // Measured voltage
    bool is_valid;            // Data validity flag
    uint8_t quality;          // Signal quality (0-100)
};

/**
 * @brief Sensor configuration parameters
 */
struct SensorConfig {
    // ADC Configuration
    uint8_t adc_pin;          // ADC pin number
    uint16_t adc_resolution;  // ADC resolution (bits)
    float reference_voltage;  // Reference voltage (V)
    
    // Calibration parameters
    float dark_offset;        // Dark current offset
    float sensitivity;        // Lux per volt sensitivity
    float noise_threshold;    // Noise threshold for filtering
    
    // Sampling configuration
    uint32_t sample_rate_ms;  // Sampling interval in milliseconds
    uint8_t oversampling;     // Number of samples to average
    bool auto_gain;           // Enable automatic gain adjustment
    
    // Power management
    bool low_power_mode;      // Enable low power mode
    uint32_t sleep_duration_ms; // Sleep duration between readings
};

/**
 * @brief Callback function type for sensor data
 */
using DataCallback = std::function<void(const SensorReading&)>;

/**
 * @brief Abstract base class for light sensor implementations
 */
class ILightSensor {
public:
    virtual ~ILightSensor() = default;
    
    /**
     * @brief Initialize the sensor
     * @return true if initialization successful
     */
    virtual bool initialize() = 0;
    
    /**
     * @brief Read current sensor value
     * @return SensorReading structure with current data
     */
    virtual SensorReading read() = 0;
    
    /**
     * @brief Start continuous sampling
     * @param callback Function to call with each reading
     */
    virtual void startSampling(DataCallback callback) = 0;
    
    /**
     * @brief Stop continuous sampling
     */
    virtual void stopSampling() = 0;
    
    /**
     * @brief Configure sensor parameters
     * @param config Configuration structure
     */
    virtual void configure(const SensorConfig& config) = 0;
    
    /**
     * @brief Calibrate sensor (dark and light reference)
     * @param dark_value Dark reference value
     * @param light_value Light reference value
     */
    virtual void calibrate(float dark_value, float light_value) = 0;
    
    /**
     * @brief Enter low power mode
     */
    virtual void enterLowPower() = 0;
    
    /**
     * @brief Wake from low power mode
     */
    virtual void wakeUp() = 0;
};

/**
 * @brief Concrete implementation for ADC-based light sensor
 */
class ADCLightSensor : public ILightSensor {
public:
    explicit ADCLightSensor(const SensorConfig& config);
    ~ADCLightSensor() override = default;
    
    bool initialize() override;
    SensorReading read() override;
    void startSampling(DataCallback callback) override;
    void stopSampling() override;
    void configure(const SensorConfig& config) override;
    void calibrate(float dark_value, float light_value) override;
    void enterLowPower() override;
    void wakeUp() override;
    
private:
    SensorConfig config_;
    bool is_sampling_;
    bool is_initialized_;
    DataCallback data_callback_;
    std::chrono::steady_clock::time_point last_sample_time_;
    
    /**
     * @brief Read raw ADC value
     * @return Raw ADC reading (0.0 - 1.0)
     */
    float readRawADC();
    
    /**
     * @brief Convert raw ADC value to voltage
     * @param raw_value Raw ADC value
     * @return Voltage in volts
     */
    float adcToVoltage(float raw_value);
    
    /**
     * @brief Convert voltage to lux value
     * @param voltage Voltage reading
     * @return Lux value
     */
    float voltageToLux(float voltage);
    
    /**
     * @brief Apply noise filtering to reading
     * @param reading Raw reading to filter
     * @return Filtered reading
     */
    float applyNoiseFilter(float reading);
    
    /**
     * @brief Calculate signal quality
     * @param reading Current reading
     * @return Quality score (0-100)
     */
    uint8_t calculateQuality(const SensorReading& reading);
};

} // namespace LightSensor
