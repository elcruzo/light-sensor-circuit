#pragma once

#include <cstdint>
#include <functional>

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
    uint8_t adc_pin;          // ADC pin number (GPIO 32-39 for ESP32)
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
    
    /**
     * @brief Process sensor (call in main loop for continuous sampling)
     */
    virtual void process() = 0;
};

/**
 * @brief ESP32 ADC-based light sensor implementation
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
    void process() override;
    
private:
    static const size_t FILTER_BUFFER_SIZE = 5;
    
    SensorConfig config_;
    bool is_sampling_;
    bool is_initialized_;
    bool was_sampling_before_sleep_;
    DataCallback data_callback_;
    uint32_t last_sample_time_ms_;
    
    // Noise filter buffer (static array instead of std::vector)
    float filter_buffer_[FILTER_BUFFER_SIZE];
    size_t filter_buffer_index_;
    
    float readRawADC();
    float adcToVoltage(float raw_value);
    float voltageToLux(float voltage);
    float applyNoiseFilter(float reading);
    uint8_t calculateQuality(const SensorReading& reading);
};

}  // namespace LightSensor
