#pragma once

#include "light_sensor.h"
#include "power_manager.h"
#include "data_logger.h"
#include "signal_processor.h"
#include <string>
#include <map>
#include <memory>
#include <functional>

namespace LightSensor {

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
    std::string device_id;            // Unique device identifier
    std::string firmware_version;     // Firmware version
    bool enable_debug_mode;           // Enable debug output
    uint32_t system_timeout_ms;       // System timeout in milliseconds
    bool enable_watchdog;             // Enable watchdog timer
    uint32_t watchdog_timeout_ms;     // Watchdog timeout in milliseconds
};

/**
 * @brief Calibration data
 */
struct CalibrationData {
    float dark_reference;             // Dark current reference value
    float light_reference;            // Light reference value (lux)
    float sensitivity;                // Calculated sensitivity
    float offset;                     // Calculated offset
    uint32_t calibration_timestamp;   // When calibration was performed
    bool is_valid;                    // Calibration validity flag
    std::string calibration_method;   // Method used for calibration
};

/**
 * @brief Configuration validation result
 */
struct ConfigValidation {
    bool is_valid;                    // Overall validation result
    std::vector<std::string> errors; // List of validation errors
    std::vector<std::string> warnings; // List of validation warnings
};

/**
 * @brief Configuration change callback
 */
using ConfigChangeCallback = std::function<void(const std::string&, const std::string&)>;

/**
 * @brief Configuration manager class
 */
class ConfigManager {
public:
    explicit ConfigManager(const std::string& config_file_path = "config.json");
    ~ConfigManager() = default;
    
    /**
     * @brief Initialize configuration manager
     * @return true if initialization successful
     */
    bool initialize();
    
    /**
     * @brief Load configuration from file
     * @return true if load successful
     */
    bool loadConfig();
    
    /**
     * @brief Save configuration to file
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
     * @brief Get configuration value by key
     * @param key Configuration key
     * @return Configuration value (empty if not found)
     */
    std::string getConfigValue(const std::string& key) const;
    
    /**
     * @brief Set configuration value by key
     * @param key Configuration key
     * @param value Configuration value
     * @return true if set successful
     */
    bool setConfigValue(const std::string& key, const std::string& value);
    
    /**
     * @brief Export configuration to JSON
     * @return JSON string representation
     */
    std::string exportToJson() const;
    
    /**
     * @brief Import configuration from JSON
     * @param json JSON string representation
     * @return true if import successful
     */
    bool importFromJson(const std::string& json);
    
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
    
private:
    std::string config_file_path_;
    SystemConfig config_;
    CalibrationData calibration_data_;
    ConfigChangeCallback config_change_callback_;
    
    /**
     * @brief Parse JSON configuration
     * @param json JSON string
     * @return true if parse successful
     */
    bool parseJsonConfig(const std::string& json);
    
    /**
     * @brief Generate JSON configuration
     * @return JSON string
     */
    std::string generateJsonConfig() const;
    
    /**
     * @brief Validate sensor configuration
     * @param sensor_config Sensor configuration to validate
     * @return Validation result
     */
    ConfigValidation validateSensorConfig(const SensorConfig& sensor_config) const;
    
    /**
     * @brief Validate power configuration
     * @param power_config Power configuration to validate
     * @return Validation result
     */
    ConfigValidation validatePowerConfig(const PowerConfig& power_config) const;
    
    /**
     * @brief Validate logger configuration
     * @param logger_config Logger configuration to validate
     * @return Validation result
     */
    ConfigValidation validateLoggerConfig(const LoggerConfig& logger_config) const;
    
    /**
     * @brief Validate signal configuration
     * @param signal_config Signal configuration to validate
     * @return Validation result
     */
    ConfigValidation validateSignalConfig(const SignalConfig& signal_config) const;
    
    /**
     * @brief Notify configuration change
     * @param key Changed configuration key
     * @param value New configuration value
     */
    void notifyConfigChange(const std::string& key, const std::string& value);
};

/**
 * @brief Configuration presets
 */
class ConfigPresets {
public:
    /**
     * @brief Get low power preset
     * @return Low power configuration
     */
    static SystemConfig getLowPowerPreset();
    
    /**
     * @brief Get high accuracy preset
     * @return High accuracy configuration
     */
    static SystemConfig getHighAccuracyPreset();
    
    /**
     * @brief Get balanced preset
     * @return Balanced configuration
     */
    static SystemConfig getBalancedPreset();
    
    /**
     * @brief Get development preset
     * @return Development configuration
     */
    static SystemConfig getDevelopmentPreset();
    
    /**
     * @brief Get preset by name
     * @param preset_name Name of preset
     * @return Configuration preset (empty if not found)
     */
    static SystemConfig getPreset(const std::string& preset_name);
    
    /**
     * @brief Get available preset names
     * @return Vector of preset names
     */
    static std::vector<std::string> getAvailablePresets();
};

} // namespace LightSensor
