#pragma once

#include "light_sensor.h"
#include "power_manager.h"
#include "data_logger.h"
#include "signal_processor.h"
#include <cstdint>
#include <functional>

namespace LightSensor {

// Maximum string lengths for config fields
static const size_t MAX_DEVICE_ID_LEN = 32;
static const size_t MAX_VERSION_LEN = 16;
static const size_t MAX_PATH_LEN = 64;
static const size_t MAX_METHOD_LEN = 32;

/**
 * @brief System configuration structure
 */
struct SystemConfig {
    // Sensor configuration
    SensorConfig sensor;
    
    // Power management configuration
    PowerConfig power;
    
    // Data logging configuration
    LoggerConfig logger;
    
    // Signal processing configuration
    SignalConfig signal;
    
    // System settings
    char device_id[MAX_DEVICE_ID_LEN];
    char firmware_version[MAX_VERSION_LEN];
    bool enable_debug_mode;
    uint32_t system_timeout_ms;
    bool enable_watchdog;
    uint32_t watchdog_timeout_ms;
};

/**
 * @brief Calibration data
 */
struct CalibrationData {
    float dark_reference;
    float light_reference;
    float sensitivity;
    float offset;
    uint32_t calibration_timestamp;
    bool is_valid;
    char calibration_method[MAX_METHOD_LEN];
};

/**
 * @brief Configuration validation result
 */
struct ConfigValidation {
    bool is_valid;
    uint8_t error_count;
    uint8_t warning_count;
    char last_error[64];
    char last_warning[64];
};

/**
 * @brief Configuration change callback
 */
using ConfigChangeCallback = std::function<void(const char*, const char*)>;

/**
 * @brief ESP32 Configuration manager with SPIFFS and ArduinoJson
 */
class ConfigManager {
public:
    explicit ConfigManager(const char* config_file_path = "/config.json");
    ~ConfigManager() = default;
    
    /**
     * @brief Initialize configuration manager and SPIFFS
     * @return true if initialization successful
     */
    bool initialize();
    
    /**
     * @brief Load configuration from SPIFFS
     * @return true if load successful
     */
    bool loadConfig();
    
    /**
     * @brief Save configuration to SPIFFS
     * @return true if save successful
     */
    bool saveConfig();
    
    /**
     * @brief Get current system configuration
     * @return System configuration
     */
    const SystemConfig& getConfig() const;
    
    /**
     * @brief Update system configuration
     * @param config New configuration
     * @return true if update successful
     */
    bool updateConfig(const SystemConfig& config);
    
    /**
     * @brief Validate configuration
     * @param config Configuration to validate
     * @return Validation result
     */
    ConfigValidation validateConfig(const SystemConfig& config) const;
    
    /**
     * @brief Get calibration data
     * @return Calibration data
     */
    const CalibrationData& getCalibrationData() const;
    
    /**
     * @brief Update calibration data
     * @param calibration New calibration data
     * @return true if update successful
     */
    bool updateCalibrationData(const CalibrationData& calibration);
    
    /**
     * @brief Perform sensor calibration
     * @param dark_value Dark reference value
     * @param light_value Light reference value
     * @param light_lux Light reference in lux
     * @return true if calibration successful
     */
    bool calibrateSensor(float dark_value, float light_value, float light_lux);
    
    /**
     * @brief Reset configuration to defaults
     * @return true if reset successful
     */
    bool resetToDefaults();
    
    /**
     * @brief Set configuration change callback
     * @param callback Function to call on configuration changes
     */
    void setConfigChangeCallback(ConfigChangeCallback callback);
    
    /**
     * @brief Get default configuration
     * @return Default system configuration
     */
    static SystemConfig getDefaultConfig();
    
    /**
     * @brief Get default calibration data
     * @return Default calibration data
     */
    static CalibrationData getDefaultCalibrationData();
    
    /**
     * @brief Check if SPIFFS is available
     * @return true if SPIFFS is mounted
     */
    bool isStorageAvailable() const;
    
private:
    char config_file_path_[MAX_PATH_LEN];
    char calibration_file_path_[MAX_PATH_LEN];
    SystemConfig config_;
    CalibrationData calibration_data_;
    ConfigChangeCallback config_change_callback_;
    bool spiffs_initialized_;
    
    bool initializeSPIFFS();
    bool parseJsonConfig(const char* json);
    bool generateJsonConfig(char* buffer, size_t buffer_size) const;
    bool loadCalibration();
    bool saveCalibration();
    
    ConfigValidation validateSensorConfig(const SensorConfig& sensor_config) const;
    ConfigValidation validatePowerConfig(const PowerConfig& power_config) const;
    ConfigValidation validateLoggerConfig(const LoggerConfig& logger_config) const;
    ConfigValidation validateSignalConfig(const SignalConfig& signal_config) const;
    
    void notifyConfigChange(const char* key, const char* value);
};

/**
 * @brief Configuration presets
 */
class ConfigPresets {
public:
    static SystemConfig getLowPowerPreset();
    static SystemConfig getHighAccuracyPreset();
    static SystemConfig getBalancedPreset();
    static SystemConfig getDevelopmentPreset();
    static SystemConfig getPreset(const char* preset_name);
};

}  // namespace LightSensor
