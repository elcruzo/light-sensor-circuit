#include "config_manager.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <vector>
#include <string>

#ifdef ARDUINO
#include <Arduino.h>
#else
#include <iostream>
#include <chrono>
#endif

namespace LightSensor {

ConfigManager::ConfigManager(const std::string& config_file_path)
    : config_file_path_(config_file_path) {
    
    // Initialize with default configuration
    config_ = getDefaultConfig();
    calibration_data_ = getDefaultCalibrationData();
}

bool ConfigManager::initialize() {
    // Try to load existing configuration
    if (!loadConfig()) {
        // If loading fails, save default configuration
        return saveConfig();
    }
    return true;
}

bool ConfigManager::loadConfig() {
    #ifdef ARDUINO
    // Arduino file system implementation
    if (!SD.begin()) {
        return false;
    }
    
    File config_file = SD.open(config_file_path_.c_str(), FILE_READ);
    if (!config_file) {
        return false;
    }
    
    std::string json_content;
    while (config_file.available()) {
        json_content += static_cast<char>(config_file.read());
    }
    config_file.close();
    
    return parseJsonConfig(json_content);
    #else
    // Standard C++ file implementation
    std::ifstream file(config_file_path_);
    if (!file.is_open()) {
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    
    return parseJsonConfig(buffer.str());
    #endif
}

bool ConfigManager::saveConfig() {
    std::string json_content = generateJsonConfig();
    
    #ifdef ARDUINO
    // Arduino file system implementation
    if (!SD.begin()) {
        return false;
    }
    
    File config_file = SD.open(config_file_path_.c_str(), FILE_WRITE);
    if (!config_file) {
        return false;
    }
    
    config_file.print(json_content.c_str());
    config_file.close();
    return true;
    #else
    // Standard C++ file implementation
    std::ofstream file(config_file_path_);
    if (!file.is_open()) {
        return false;
    }
    
    file << json_content;
    file.close();
    return !file.fail();
    #endif
}

const SystemConfig& ConfigManager::getConfig() const {
    return config_;
}

bool ConfigManager::updateConfig(const SystemConfig& config) {
    // Validate configuration before updating
    ConfigValidation validation = validateConfig(config);
    if (!validation.is_valid) {
        return false;
    }
    
    config_ = config;
    return saveConfig();
}

ConfigValidation ConfigManager::validateConfig(const SystemConfig& config) const {
    ConfigValidation result;
    result.is_valid = true;
    
    // Validate sensor configuration
    auto sensor_validation = validateSensorConfig(config.sensor);
    if (!sensor_validation.is_valid) {
        result.is_valid = false;
        result.errors.insert(result.errors.end(), 
                           sensor_validation.errors.begin(), 
                           sensor_validation.errors.end());
    }
    result.warnings.insert(result.warnings.end(),
                         sensor_validation.warnings.begin(),
                         sensor_validation.warnings.end());
    
    // Validate power configuration
    auto power_validation = validatePowerConfig(config.power);
    if (!power_validation.is_valid) {
        result.is_valid = false;
        result.errors.insert(result.errors.end(),
                           power_validation.errors.begin(),
                           power_validation.errors.end());
    }
    result.warnings.insert(result.warnings.end(),
                         power_validation.warnings.begin(),
                         power_validation.warnings.end());
    
    // Validate logger configuration
    auto logger_validation = validateLoggerConfig(config.logger);
    if (!logger_validation.is_valid) {
        result.is_valid = false;
        result.errors.insert(result.errors.end(),
                           logger_validation.errors.begin(),
                           logger_validation.errors.end());
    }
    result.warnings.insert(result.warnings.end(),
                         logger_validation.warnings.begin(),
                         logger_validation.warnings.end());
    
    // Validate signal configuration
    auto signal_validation = validateSignalConfig(config.signal);
    if (!signal_validation.is_valid) {
        result.is_valid = false;
        result.errors.insert(result.errors.end(),
                           signal_validation.errors.begin(),
                           signal_validation.errors.end());
    }
    result.warnings.insert(result.warnings.end(),
                         signal_validation.warnings.begin(),
                         signal_validation.warnings.end());
    
    return result;
}

const CalibrationData& ConfigManager::getCalibrationData() const {
    return calibration_data_;
}

bool ConfigManager::updateCalibrationData(const CalibrationData& calibration) {
    calibration_data_ = calibration;
    return saveConfig();
}

bool ConfigManager::calibrateSensor(float dark_value, float light_value, float light_lux) {
    if (dark_value >= light_value || light_lux <= 0.0f) {
        return false;
    }
    
    calibration_data_.dark_reference = dark_value;
    calibration_data_.light_reference = light_lux;
    calibration_data_.sensitivity = (light_value - dark_value) / light_lux;
    calibration_data_.offset = dark_value;
    #ifdef ARDUINO
    calibration_data_.calibration_timestamp = millis();
    #else
    calibration_data_.calibration_timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    #endif
    calibration_data_.is_valid = true;
    calibration_data_.calibration_method = "Two-point calibration";
    
    // Update sensor configuration with calibrated values
    config_.sensor.dark_offset = dark_value;
    config_.sensor.sensitivity = calibration_data_.sensitivity;
    
    return saveConfig();
}

bool ConfigManager::resetToDefaults() {
    config_ = getDefaultConfig();
    calibration_data_ = getDefaultCalibrationData();
    return saveConfig();
}

void ConfigManager::setConfigChangeCallback(ConfigChangeCallback callback) {
    config_change_callback_ = callback;
}

std::string ConfigManager::getConfigValue(const std::string& key) const {
    // Simple key-value mapping for basic configuration values
    if (key == "device_id") return config_.device_id;
    if (key == "firmware_version") return config_.firmware_version;
    if (key == "enable_debug_mode") return config_.enable_debug_mode ? "true" : "false";
    if (key == "system_timeout_ms") return std::to_string(config_.system_timeout_ms);
    if (key == "enable_watchdog") return config_.enable_watchdog ? "true" : "false";
    if (key == "watchdog_timeout_ms") return std::to_string(config_.watchdog_timeout_ms);
    
    return "";
}

bool ConfigManager::setConfigValue(const std::string& key, const std::string& value) {
    bool changed = false;
    
    if (key == "device_id") {
        config_.device_id = value;
        changed = true;
    } else if (key == "firmware_version") {
        config_.firmware_version = value;
        changed = true;
    } else if (key == "enable_debug_mode") {
        config_.enable_debug_mode = (value == "true");
        changed = true;
    } else if (key == "system_timeout_ms") {
        config_.system_timeout_ms = std::stoul(value);
        changed = true;
    } else if (key == "enable_watchdog") {
        config_.enable_watchdog = (value == "true");
        changed = true;
    } else if (key == "watchdog_timeout_ms") {
        config_.watchdog_timeout_ms = std::stoul(value);
        changed = true;
    }
    
    if (changed) {
        notifyConfigChange(key, value);
        return saveConfig();
    }
    
    return false;
}

std::string ConfigManager::exportToJson() const {
    return generateJsonConfig();
}

bool ConfigManager::importFromJson(const std::string& json) {
    return parseJsonConfig(json);
}

SystemConfig ConfigManager::getDefaultConfig() {
    SystemConfig config;
    
    // Default sensor configuration
    #ifdef ARDUINO
    config.sensor.adc_pin = A0;
    #else
    config.sensor.adc_pin = 0; // Mock pin for desktop
    #endif
    config.sensor.adc_resolution = 10;
    config.sensor.reference_voltage = 3.3f;
    config.sensor.dark_offset = 0.0f;
    config.sensor.sensitivity = 1.0f;
    config.sensor.noise_threshold = 0.01f;
    config.sensor.sample_rate_ms = 1000;
    config.sensor.oversampling = 4;
    config.sensor.auto_gain = false;
    config.sensor.low_power_mode = true;
    config.sensor.sleep_duration_ms = 100;
    
    // Default power configuration
    config.power.sleep_timeout_ms = 30000;      // 30 seconds
    config.power.deep_sleep_timeout_ms = 300000; // 5 minutes
    config.power.enable_wake_on_light = true;
    config.power.light_threshold = 0.1f;
    config.power.disable_unused_peripherals = true;
    config.power.reduce_clock_speed = true;
    config.power.adc_sample_delay_ms = 1;
    config.power.low_battery_threshold = 3.2f;
    config.power.critical_battery_threshold = 3.0f;
    config.power.enable_battery_monitoring = true;
    
    // Default logger configuration
    config.logger.log_file_path = "/logs";
    config.logger.buffer_size = 100;
    config.logger.flush_threshold = 50;
    config.logger.enable_compression = false;
    config.logger.enable_timestamp = true;
    config.logger.min_lux_threshold = 0.0f;
    config.logger.max_lux_threshold = 100000.0f;
    config.logger.filter_noise = true;
    config.logger.min_quality_threshold = 50;
    config.logger.max_file_size_bytes = 1024 * 1024; // 1MB
    config.logger.max_log_days = 30;
    config.logger.enable_rotation = true;
    
    // Default signal configuration
    config.signal.moving_average_window = 5;
    config.signal.low_pass_cutoff = 0.5f;
    config.signal.high_pass_cutoff = 0.01f;
    config.signal.enable_median_filter = true;
    config.signal.median_window = 3;
    config.signal.noise_threshold = 0.01f;
    config.signal.enable_outlier_removal = true;
    config.signal.outlier_threshold = 2.0f;
    config.signal.enable_trend_detection = true;
    config.signal.trend_window = 10;
    config.signal.enable_peak_detection = false;
    config.signal.peak_threshold = 0.1f;
    config.signal.enable_adaptive_filter = true;
    config.signal.adaptation_rate = 0.1f;
    config.signal.noise_floor = 0.001f;
    
    // Default system settings
    config.device_id = "light_sensor_001";
    config.firmware_version = "1.0.0";
    config.enable_debug_mode = false;
    config.system_timeout_ms = 300000; // 5 minutes
    config.enable_watchdog = true;
    config.watchdog_timeout_ms = 8000; // 8 seconds
    
    return config;
}

CalibrationData ConfigManager::getDefaultCalibrationData() {
    CalibrationData calibration;
    calibration.dark_reference = 0.0f;
    calibration.light_reference = 1000.0f;
    calibration.sensitivity = 1.0f;
    calibration.offset = 0.0f;
    calibration.calibration_timestamp = 0;
    calibration.is_valid = false;
    calibration.calibration_method = "None";
    return calibration;
}

bool ConfigManager::parseJsonConfig(const std::string& json) {
    // Simplified JSON parsing - in a real implementation, you'd use a proper JSON library
    // This is a placeholder that would need to be implemented with a JSON parser
    
    // For now, just return true to indicate "successful" parsing
    // In a real implementation, this would parse the JSON and populate the config structure
    return true;
}

std::string ConfigManager::generateJsonConfig() const {
    // Simplified JSON generation - in a real implementation, you'd use a proper JSON library
    std::stringstream json;
    
    json << "{\n";
    json << "  \"device_id\": \"" << config_.device_id << "\",\n";
    json << "  \"firmware_version\": \"" << config_.firmware_version << "\",\n";
    json << "  \"enable_debug_mode\": " << (config_.enable_debug_mode ? "true" : "false") << ",\n";
    json << "  \"system_timeout_ms\": " << config_.system_timeout_ms << ",\n";
    json << "  \"enable_watchdog\": " << (config_.enable_watchdog ? "true" : "false") << ",\n";
    json << "  \"watchdog_timeout_ms\": " << config_.watchdog_timeout_ms << ",\n";
    json << "  \"sensor\": {\n";
    json << "    \"adc_pin\": " << static_cast<int>(config_.sensor.adc_pin) << ",\n";
    json << "    \"adc_resolution\": " << config_.sensor.adc_resolution << ",\n";
    json << "    \"reference_voltage\": " << config_.sensor.reference_voltage << ",\n";
    json << "    \"sample_rate_ms\": " << config_.sensor.sample_rate_ms << "\n";
    json << "  },\n";
    json << "  \"calibration\": {\n";
    json << "    \"is_valid\": " << (calibration_data_.is_valid ? "true" : "false") << ",\n";
    json << "    \"dark_reference\": " << calibration_data_.dark_reference << ",\n";
    json << "    \"light_reference\": " << calibration_data_.light_reference << ",\n";
    json << "    \"sensitivity\": " << calibration_data_.sensitivity << "\n";
    json << "  }\n";
    json << "}\n";
    
    return json.str();
}

ConfigValidation ConfigManager::validateSensorConfig(const SensorConfig& sensor_config) const {
    ConfigValidation result;
    result.is_valid = true;
    
    if (sensor_config.adc_resolution == 0 || sensor_config.adc_resolution > 16) {
        result.errors.push_back("Invalid ADC resolution");
        result.is_valid = false;
    }
    
    if (sensor_config.reference_voltage <= 0.0f || sensor_config.reference_voltage > 5.0f) {
        result.errors.push_back("Invalid reference voltage");
        result.is_valid = false;
    }
    
    if (sensor_config.sample_rate_ms == 0) {
        result.errors.push_back("Invalid sample rate");
        result.is_valid = false;
    }
    
    if (sensor_config.oversampling == 0) {
        result.warnings.push_back("Oversampling is disabled");
    }
    
    return result;
}

ConfigValidation ConfigManager::validatePowerConfig(const PowerConfig& power_config) const {
    ConfigValidation result;
    result.is_valid = true;
    
    if (power_config.sleep_timeout_ms == 0) {
        result.warnings.push_back("Sleep timeout is disabled");
    }
    
    if (power_config.low_battery_threshold <= power_config.critical_battery_threshold) {
        result.errors.push_back("Low battery threshold must be higher than critical threshold");
        result.is_valid = false;
    }
    
    return result;
}

ConfigValidation ConfigManager::validateLoggerConfig(const LoggerConfig& logger_config) const {
    ConfigValidation result;
    result.is_valid = true;
    
    if (logger_config.buffer_size == 0) {
        result.errors.push_back("Invalid buffer size");
        result.is_valid = false;
    }
    
    if (logger_config.flush_threshold > logger_config.buffer_size) {
        result.errors.push_back("Flush threshold exceeds buffer size");
        result.is_valid = false;
    }
    
    if (logger_config.min_lux_threshold >= logger_config.max_lux_threshold) {
        result.errors.push_back("Invalid lux threshold range");
        result.is_valid = false;
    }
    
    return result;
}

ConfigValidation ConfigManager::validateSignalConfig(const SignalConfig& signal_config) const {
    ConfigValidation result;
    result.is_valid = true;
    
    if (signal_config.moving_average_window == 0) {
        result.warnings.push_back("Moving average window is disabled");
    }
    
    if (signal_config.low_pass_cutoff <= 0.0f) {
        result.warnings.push_back("Low-pass filter is disabled");
    }
    
    if (signal_config.outlier_threshold <= 0.0f) {
        result.warnings.push_back("Outlier detection threshold is too low");
    }
    
    return result;
}

void ConfigManager::notifyConfigChange(const std::string& key, const std::string& value) {
    if (config_change_callback_) {
        config_change_callback_(key, value);
    }
}

// ConfigPresets Implementation
SystemConfig ConfigPresets::getLowPowerPreset() {
    SystemConfig config = ConfigManager::getDefaultConfig();
    
    // Optimize for low power consumption
    config.sensor.sample_rate_ms = 5000;        // 5 second intervals
    config.sensor.oversampling = 1;             // No oversampling
    config.sensor.low_power_mode = true;
    config.sensor.sleep_duration_ms = 1000;     // 1 second sleep
    
    config.power.sleep_timeout_ms = 10000;      // 10 seconds
    config.power.deep_sleep_timeout_ms = 60000; // 1 minute
    config.power.disable_unused_peripherals = true;
    config.power.reduce_clock_speed = true;
    
    config.logger.buffer_size = 50;             // Smaller buffer
    config.logger.flush_threshold = 25;
    
    config.signal.moving_average_window = 3;    // Minimal filtering
    config.signal.enable_median_filter = false;
    config.signal.enable_adaptive_filter = false;
    
    return config;
}

SystemConfig ConfigPresets::getHighAccuracyPreset() {
    SystemConfig config = ConfigManager::getDefaultConfig();
    
    // Optimize for high accuracy
    config.sensor.sample_rate_ms = 100;         // 100ms intervals
    config.sensor.oversampling = 16;            // High oversampling
    config.sensor.auto_gain = true;
    
    config.logger.buffer_size = 500;            // Larger buffer
    config.logger.flush_threshold = 100;
    config.logger.filter_noise = true;
    config.logger.min_quality_threshold = 80;   // High quality threshold
    
    config.signal.moving_average_window = 10;   // More filtering
    config.signal.enable_median_filter = true;
    config.signal.median_window = 5;
    config.signal.enable_outlier_removal = true;
    config.signal.outlier_threshold = 1.5f;     // Stricter outlier detection
    config.signal.enable_adaptive_filter = true;
    
    return config;
}

SystemConfig ConfigPresets::getBalancedPreset() {
    // This is the same as the default configuration
    return ConfigManager::getDefaultConfig();
}

SystemConfig ConfigPresets::getDevelopmentPreset() {
    SystemConfig config = ConfigManager::getDefaultConfig();
    
    // Optimize for development and debugging
    config.sensor.sample_rate_ms = 500;         // 500ms intervals
    config.enable_debug_mode = true;
    
    config.logger.enable_timestamp = true;
    config.logger.min_quality_threshold = 0;    // Log all data
    
    config.signal.enable_trend_detection = true;
    config.signal.enable_peak_detection = true;
    
    return config;
}

SystemConfig ConfigPresets::getPreset(const std::string& preset_name) {
    if (preset_name == "low_power") {
        return getLowPowerPreset();
    } else if (preset_name == "high_accuracy") {
        return getHighAccuracyPreset();
    } else if (preset_name == "balanced") {
        return getBalancedPreset();
    } else if (preset_name == "development") {
        return getDevelopmentPreset();
    }
    
    return SystemConfig{}; // Empty config if preset not found
}

std::vector<std::string> ConfigPresets::getAvailablePresets() {
    return {"low_power", "high_accuracy", "balanced", "development"};
}

} // namespace LightSensor
