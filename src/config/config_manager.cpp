#include "config_manager.h"
#include <Arduino.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <cstring>
#include <cmath>

namespace LightSensor {

ConfigManager::ConfigManager(const char* config_file_path)
    : spiffs_initialized_(false) {
    
    strncpy(config_file_path_, config_file_path, MAX_PATH_LEN - 1);
    config_file_path_[MAX_PATH_LEN - 1] = '\0';
    
    // Set calibration file path
    strncpy(calibration_file_path_, "/calibration.json", MAX_PATH_LEN - 1);
    calibration_file_path_[MAX_PATH_LEN - 1] = '\0';
    
    // Initialize with default configuration
    config_ = getDefaultConfig();
    calibration_data_ = getDefaultCalibrationData();
}

bool ConfigManager::initialize() {
    if (!initializeSPIFFS()) {
        return false;
    }
    
    // Try to load existing configuration
    if (!loadConfig()) {
        // If loading fails, save default configuration
        if (!saveConfig()) {
            return false;
        }
    }
    
    // Try to load calibration data
    loadCalibration();
    
    return true;
}

bool ConfigManager::initializeSPIFFS() {
    if (spiffs_initialized_) {
        return true;
    }
    
    if (!SPIFFS.begin(true)) {  // true = format if mount fails
        return false;
    }
    
    spiffs_initialized_ = true;
    return true;
}

bool ConfigManager::isStorageAvailable() const {
    return spiffs_initialized_;
}

bool ConfigManager::loadConfig() {
    if (!spiffs_initialized_) {
        return false;
    }
    
    File config_file = SPIFFS.open(config_file_path_, FILE_READ);
    if (!config_file) {
        return false;
    }
    
    // Read file content
    size_t file_size = config_file.size();
    if (file_size > 4096) {  // Sanity check
        config_file.close();
        return false;
    }
    
    char* json_buffer = new char[file_size + 1];
    if (!json_buffer) {
        config_file.close();
        return false;
    }
    
    size_t bytes_read = config_file.readBytes(json_buffer, file_size);
    json_buffer[bytes_read] = '\0';
    config_file.close();
    
    bool result = parseJsonConfig(json_buffer);
    delete[] json_buffer;
    
    return result;
}

bool ConfigManager::parseJsonConfig(const char* json) {
    JsonDocument doc;
    
    DeserializationError error = deserializeJson(doc, json);
    if (error) {
        return false;
    }
    
    // Parse device info
    if (doc.containsKey("device_id")) {
        strncpy(config_.device_id, doc["device_id"] | "light_sensor_001", MAX_DEVICE_ID_LEN - 1);
    }
    if (doc.containsKey("firmware_version")) {
        strncpy(config_.firmware_version, doc["firmware_version"] | "1.0.0", MAX_VERSION_LEN - 1);
    }
    config_.enable_debug_mode = doc["enable_debug_mode"] | false;
    config_.system_timeout_ms = doc["system_timeout_ms"] | 300000;
    config_.enable_watchdog = doc["enable_watchdog"] | true;
    config_.watchdog_timeout_ms = doc["watchdog_timeout_ms"] | 8000;
    
    // Parse sensor configuration
    JsonObject sensor = doc["sensor"];
    if (!sensor.isNull()) {
        config_.sensor.adc_pin = sensor["adc_pin"] | 34;
        config_.sensor.adc_resolution = sensor["adc_resolution"] | 12;
        config_.sensor.reference_voltage = sensor["reference_voltage"] | 3.3f;
        config_.sensor.dark_offset = sensor["dark_offset"] | 0.0f;
        config_.sensor.sensitivity = sensor["sensitivity"] | 1.0f;
        config_.sensor.noise_threshold = sensor["noise_threshold"] | 0.01f;
        config_.sensor.sample_rate_ms = sensor["sample_rate_ms"] | 1000;
        config_.sensor.oversampling = sensor["oversampling"] | 4;
        config_.sensor.auto_gain = sensor["auto_gain"] | false;
        config_.sensor.low_power_mode = sensor["low_power_mode"] | true;
        config_.sensor.sleep_duration_ms = sensor["sleep_duration_ms"] | 100;
    }
    
    // Parse power configuration
    JsonObject power = doc["power"];
    if (!power.isNull()) {
        config_.power.sleep_timeout_ms = power["sleep_timeout_ms"] | 30000;
        config_.power.deep_sleep_timeout_ms = power["deep_sleep_timeout_ms"] | 300000;
        config_.power.enable_wake_on_light = power["enable_wake_on_light"] | true;
        config_.power.light_threshold = power["light_threshold"] | 0.1f;
        config_.power.disable_unused_peripherals = power["disable_unused_peripherals"] | true;
        config_.power.reduce_clock_speed = power["reduce_clock_speed"] | true;
        config_.power.adc_sample_delay_ms = power["adc_sample_delay_ms"] | 1;
        config_.power.low_battery_threshold = power["low_battery_threshold"] | 3.2f;
        config_.power.critical_battery_threshold = power["critical_battery_threshold"] | 3.0f;
        config_.power.enable_battery_monitoring = power["enable_battery_monitoring"] | true;
    }
    
    // Parse logger configuration
    JsonObject logger = doc["logger"];
    if (!logger.isNull()) {
        strncpy(config_.logger.log_file_path, logger["log_file_path"] | "/logs", MAX_PATH_LEN - 1);
        config_.logger.buffer_size = logger["buffer_size"] | 100;
        config_.logger.flush_threshold = logger["flush_threshold"] | 50;
        config_.logger.enable_compression = logger["enable_compression"] | false;
        config_.logger.enable_timestamp = logger["enable_timestamp"] | true;
        config_.logger.min_lux_threshold = logger["min_lux_threshold"] | 0.0f;
        config_.logger.max_lux_threshold = logger["max_lux_threshold"] | 100000.0f;
        config_.logger.filter_noise = logger["filter_noise"] | true;
        config_.logger.min_quality_threshold = logger["min_quality_threshold"] | 50;
        config_.logger.max_file_size_bytes = logger["max_file_size_bytes"] | 1048576;
        config_.logger.max_log_days = logger["max_log_days"] | 30;
        config_.logger.enable_rotation = logger["enable_rotation"] | true;
    }
    
    // Parse signal configuration
    JsonObject signal = doc["signal"];
    if (!signal.isNull()) {
        config_.signal.moving_average_window = signal["moving_average_window"] | 5;
        config_.signal.low_pass_cutoff = signal["low_pass_cutoff"] | 0.5f;
        config_.signal.high_pass_cutoff = signal["high_pass_cutoff"] | 0.01f;
        config_.signal.enable_median_filter = signal["enable_median_filter"] | true;
        config_.signal.median_window = signal["median_window"] | 3;
        config_.signal.noise_threshold = signal["noise_threshold"] | 0.01f;
        config_.signal.enable_outlier_removal = signal["enable_outlier_removal"] | true;
        config_.signal.outlier_threshold = signal["outlier_threshold"] | 2.0f;
        config_.signal.enable_trend_detection = signal["enable_trend_detection"] | true;
        config_.signal.trend_window = signal["trend_window"] | 10;
        config_.signal.enable_peak_detection = signal["enable_peak_detection"] | false;
        config_.signal.peak_threshold = signal["peak_threshold"] | 0.1f;
        config_.signal.enable_adaptive_filter = signal["enable_adaptive_filter"] | true;
        config_.signal.adaptation_rate = signal["adaptation_rate"] | 0.1f;
        config_.signal.noise_floor = signal["noise_floor"] | 0.001f;
    }
    
    return true;
}

bool ConfigManager::saveConfig() {
    if (!spiffs_initialized_) {
        return false;
    }
    
    JsonDocument doc;
    
    // Device info
    doc["device_id"] = config_.device_id;
    doc["firmware_version"] = config_.firmware_version;
    doc["enable_debug_mode"] = config_.enable_debug_mode;
    doc["system_timeout_ms"] = config_.system_timeout_ms;
    doc["enable_watchdog"] = config_.enable_watchdog;
    doc["watchdog_timeout_ms"] = config_.watchdog_timeout_ms;
    
    // Sensor configuration
    JsonObject sensor = doc["sensor"].to<JsonObject>();
    sensor["adc_pin"] = config_.sensor.adc_pin;
    sensor["adc_resolution"] = config_.sensor.adc_resolution;
    sensor["reference_voltage"] = config_.sensor.reference_voltage;
    sensor["dark_offset"] = config_.sensor.dark_offset;
    sensor["sensitivity"] = config_.sensor.sensitivity;
    sensor["noise_threshold"] = config_.sensor.noise_threshold;
    sensor["sample_rate_ms"] = config_.sensor.sample_rate_ms;
    sensor["oversampling"] = config_.sensor.oversampling;
    sensor["auto_gain"] = config_.sensor.auto_gain;
    sensor["low_power_mode"] = config_.sensor.low_power_mode;
    sensor["sleep_duration_ms"] = config_.sensor.sleep_duration_ms;
    
    // Power configuration
    JsonObject power = doc["power"].to<JsonObject>();
    power["sleep_timeout_ms"] = config_.power.sleep_timeout_ms;
    power["deep_sleep_timeout_ms"] = config_.power.deep_sleep_timeout_ms;
    power["enable_wake_on_light"] = config_.power.enable_wake_on_light;
    power["light_threshold"] = config_.power.light_threshold;
    power["disable_unused_peripherals"] = config_.power.disable_unused_peripherals;
    power["reduce_clock_speed"] = config_.power.reduce_clock_speed;
    power["adc_sample_delay_ms"] = config_.power.adc_sample_delay_ms;
    power["low_battery_threshold"] = config_.power.low_battery_threshold;
    power["critical_battery_threshold"] = config_.power.critical_battery_threshold;
    power["enable_battery_monitoring"] = config_.power.enable_battery_monitoring;
    
    // Logger configuration
    JsonObject logger = doc["logger"].to<JsonObject>();
    logger["log_file_path"] = config_.logger.log_file_path;
    logger["buffer_size"] = config_.logger.buffer_size;
    logger["flush_threshold"] = config_.logger.flush_threshold;
    logger["enable_compression"] = config_.logger.enable_compression;
    logger["enable_timestamp"] = config_.logger.enable_timestamp;
    logger["min_lux_threshold"] = config_.logger.min_lux_threshold;
    logger["max_lux_threshold"] = config_.logger.max_lux_threshold;
    logger["filter_noise"] = config_.logger.filter_noise;
    logger["min_quality_threshold"] = config_.logger.min_quality_threshold;
    logger["max_file_size_bytes"] = config_.logger.max_file_size_bytes;
    logger["max_log_days"] = config_.logger.max_log_days;
    logger["enable_rotation"] = config_.logger.enable_rotation;
    
    // Signal configuration
    JsonObject signal = doc["signal"].to<JsonObject>();
    signal["moving_average_window"] = config_.signal.moving_average_window;
    signal["low_pass_cutoff"] = config_.signal.low_pass_cutoff;
    signal["high_pass_cutoff"] = config_.signal.high_pass_cutoff;
    signal["enable_median_filter"] = config_.signal.enable_median_filter;
    signal["median_window"] = config_.signal.median_window;
    signal["noise_threshold"] = config_.signal.noise_threshold;
    signal["enable_outlier_removal"] = config_.signal.enable_outlier_removal;
    signal["outlier_threshold"] = config_.signal.outlier_threshold;
    signal["enable_trend_detection"] = config_.signal.enable_trend_detection;
    signal["trend_window"] = config_.signal.trend_window;
    signal["enable_peak_detection"] = config_.signal.enable_peak_detection;
    signal["peak_threshold"] = config_.signal.peak_threshold;
    signal["enable_adaptive_filter"] = config_.signal.enable_adaptive_filter;
    signal["adaptation_rate"] = config_.signal.adaptation_rate;
    signal["noise_floor"] = config_.signal.noise_floor;
    
    // Write to file
    File config_file = SPIFFS.open(config_file_path_, FILE_WRITE);
    if (!config_file) {
        return false;
    }
    
    size_t bytes_written = serializeJsonPretty(doc, config_file);
    config_file.close();
    
    return bytes_written > 0;
}

bool ConfigManager::loadCalibration() {
    if (!spiffs_initialized_) {
        return false;
    }
    
    File cal_file = SPIFFS.open(calibration_file_path_, FILE_READ);
    if (!cal_file) {
        return false;
    }
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, cal_file);
    cal_file.close();
    
    if (error) {
        return false;
    }
    
    calibration_data_.dark_reference = doc["dark_reference"] | 0.0f;
    calibration_data_.light_reference = doc["light_reference"] | 1000.0f;
    calibration_data_.sensitivity = doc["sensitivity"] | 1.0f;
    calibration_data_.offset = doc["offset"] | 0.0f;
    calibration_data_.calibration_timestamp = doc["calibration_timestamp"] | 0;
    calibration_data_.is_valid = doc["is_valid"] | false;
    strncpy(calibration_data_.calibration_method, 
            doc["calibration_method"] | "None", 
            MAX_METHOD_LEN - 1);
    
    return true;
}

bool ConfigManager::saveCalibration() {
    if (!spiffs_initialized_) {
        return false;
    }
    
    JsonDocument doc;
    
    doc["dark_reference"] = calibration_data_.dark_reference;
    doc["light_reference"] = calibration_data_.light_reference;
    doc["sensitivity"] = calibration_data_.sensitivity;
    doc["offset"] = calibration_data_.offset;
    doc["calibration_timestamp"] = calibration_data_.calibration_timestamp;
    doc["is_valid"] = calibration_data_.is_valid;
    doc["calibration_method"] = calibration_data_.calibration_method;
    
    File cal_file = SPIFFS.open(calibration_file_path_, FILE_WRITE);
    if (!cal_file) {
        return false;
    }
    
    size_t bytes_written = serializeJsonPretty(doc, cal_file);
    cal_file.close();
    
    return bytes_written > 0;
}

const SystemConfig& ConfigManager::getConfig() const {
    return config_;
}

bool ConfigManager::updateConfig(const SystemConfig& config) {
    ConfigValidation validation = validateConfig(config);
    if (!validation.is_valid) {
        return false;
    }
    
    config_ = config;
    return saveConfig();
}

ConfigValidation ConfigManager::validateConfig(const SystemConfig& config) const {
    ConfigValidation result = {true, 0, 0, "", ""};
    
    // Validate sensor configuration
    ConfigValidation sensor_val = validateSensorConfig(config.sensor);
    if (!sensor_val.is_valid) {
        result.is_valid = false;
        result.error_count += sensor_val.error_count;
        strncpy(result.last_error, sensor_val.last_error, sizeof(result.last_error) - 1);
    }
    result.warning_count += sensor_val.warning_count;
    
    // Validate power configuration
    ConfigValidation power_val = validatePowerConfig(config.power);
    if (!power_val.is_valid) {
        result.is_valid = false;
        result.error_count += power_val.error_count;
        strncpy(result.last_error, power_val.last_error, sizeof(result.last_error) - 1);
    }
    result.warning_count += power_val.warning_count;
    
    // Validate logger configuration
    ConfigValidation logger_val = validateLoggerConfig(config.logger);
    if (!logger_val.is_valid) {
        result.is_valid = false;
        result.error_count += logger_val.error_count;
        strncpy(result.last_error, logger_val.last_error, sizeof(result.last_error) - 1);
    }
    result.warning_count += logger_val.warning_count;
    
    // Validate signal configuration
    ConfigValidation signal_val = validateSignalConfig(config.signal);
    if (!signal_val.is_valid) {
        result.is_valid = false;
        result.error_count += signal_val.error_count;
        strncpy(result.last_error, signal_val.last_error, sizeof(result.last_error) - 1);
    }
    result.warning_count += signal_val.warning_count;
    
    return result;
}

ConfigValidation ConfigManager::validateSensorConfig(const SensorConfig& sensor_config) const {
    ConfigValidation result = {true, 0, 0, "", ""};
    
    // ESP32 ADC1 pins are GPIO 32-39
    if (sensor_config.adc_pin < 32 || sensor_config.adc_pin > 39) {
        result.is_valid = false;
        result.error_count++;
        strncpy(result.last_error, "ADC pin must be GPIO 32-39", sizeof(result.last_error) - 1);
    }
    
    if (sensor_config.adc_resolution == 0 || sensor_config.adc_resolution > 12) {
        result.is_valid = false;
        result.error_count++;
        strncpy(result.last_error, "ADC resolution must be 1-12 bits", sizeof(result.last_error) - 1);
    }
    
    if (sensor_config.reference_voltage <= 0.0f || sensor_config.reference_voltage > 3.6f) {
        result.is_valid = false;
        result.error_count++;
        strncpy(result.last_error, "Reference voltage must be 0-3.6V", sizeof(result.last_error) - 1);
    }
    
    if (sensor_config.sample_rate_ms == 0) {
        result.is_valid = false;
        result.error_count++;
        strncpy(result.last_error, "Sample rate cannot be zero", sizeof(result.last_error) - 1);
    }
    
    if (sensor_config.oversampling == 0) {
        result.warning_count++;
        strncpy(result.last_warning, "Oversampling disabled", sizeof(result.last_warning) - 1);
    }
    
    return result;
}

ConfigValidation ConfigManager::validatePowerConfig(const PowerConfig& power_config) const {
    ConfigValidation result = {true, 0, 0, "", ""};
    
    if (power_config.sleep_timeout_ms == 0) {
        result.warning_count++;
        strncpy(result.last_warning, "Sleep timeout disabled", sizeof(result.last_warning) - 1);
    }
    
    if (power_config.low_battery_threshold <= power_config.critical_battery_threshold) {
        result.is_valid = false;
        result.error_count++;
        strncpy(result.last_error, "Low battery threshold must exceed critical", sizeof(result.last_error) - 1);
    }
    
    return result;
}

ConfigValidation ConfigManager::validateLoggerConfig(const LoggerConfig& logger_config) const {
    ConfigValidation result = {true, 0, 0, "", ""};
    
    if (logger_config.buffer_size == 0) {
        result.is_valid = false;
        result.error_count++;
        strncpy(result.last_error, "Buffer size cannot be zero", sizeof(result.last_error) - 1);
    }
    
    if (logger_config.flush_threshold > logger_config.buffer_size) {
        result.is_valid = false;
        result.error_count++;
        strncpy(result.last_error, "Flush threshold exceeds buffer", sizeof(result.last_error) - 1);
    }
    
    if (logger_config.min_lux_threshold >= logger_config.max_lux_threshold) {
        result.is_valid = false;
        result.error_count++;
        strncpy(result.last_error, "Invalid lux threshold range", sizeof(result.last_error) - 1);
    }
    
    return result;
}

ConfigValidation ConfigManager::validateSignalConfig(const SignalConfig& signal_config) const {
    ConfigValidation result = {true, 0, 0, "", ""};
    
    if (signal_config.moving_average_window == 0) {
        result.warning_count++;
        strncpy(result.last_warning, "Moving average disabled", sizeof(result.last_warning) - 1);
    }
    
    if (signal_config.outlier_threshold <= 0.0f) {
        result.warning_count++;
        strncpy(result.last_warning, "Outlier detection threshold too low", sizeof(result.last_warning) - 1);
    }
    
    return result;
}

const CalibrationData& ConfigManager::getCalibrationData() const {
    return calibration_data_;
}

bool ConfigManager::updateCalibrationData(const CalibrationData& calibration) {
    calibration_data_ = calibration;
    return saveCalibration();
}

bool ConfigManager::calibrateSensor(float dark_value, float light_value, float light_lux) {
    if (dark_value >= light_value || light_lux <= 0.0f) {
        return false;
    }
    
    calibration_data_.dark_reference = dark_value;
    calibration_data_.light_reference = light_lux;
    calibration_data_.sensitivity = (light_value - dark_value) / light_lux;
    calibration_data_.offset = dark_value;
    calibration_data_.calibration_timestamp = millis();
    calibration_data_.is_valid = true;
    strncpy(calibration_data_.calibration_method, "Two-point", MAX_METHOD_LEN - 1);
    
    // Update sensor configuration with calibrated values
    config_.sensor.dark_offset = dark_value;
    config_.sensor.sensitivity = calibration_data_.sensitivity;
    
    return saveCalibration() && saveConfig();
}

bool ConfigManager::resetToDefaults() {
    config_ = getDefaultConfig();
    calibration_data_ = getDefaultCalibrationData();
    return saveConfig() && saveCalibration();
}

void ConfigManager::setConfigChangeCallback(ConfigChangeCallback callback) {
    config_change_callback_ = callback;
}

void ConfigManager::notifyConfigChange(const char* key, const char* value) {
    if (config_change_callback_) {
        config_change_callback_(key, value);
    }
}

SystemConfig ConfigManager::getDefaultConfig() {
    SystemConfig config;
    
    // Default sensor configuration (ESP32 ADC1 on GPIO 34)
    config.sensor.adc_pin = 34;
    config.sensor.adc_resolution = 12;
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
    config.power.sleep_timeout_ms = 30000;
    config.power.deep_sleep_timeout_ms = 300000;
    config.power.enable_wake_on_light = true;
    config.power.light_threshold = 0.1f;
    config.power.disable_unused_peripherals = true;
    config.power.reduce_clock_speed = true;
    config.power.adc_sample_delay_ms = 1;
    config.power.low_battery_threshold = 3.2f;
    config.power.critical_battery_threshold = 3.0f;
    config.power.enable_battery_monitoring = true;
    
    // Default logger configuration
    strncpy(config.logger.log_file_path, "/logs", MAX_PATH_LEN - 1);
    config.logger.buffer_size = 100;
    config.logger.flush_threshold = 50;
    config.logger.enable_compression = false;
    config.logger.enable_timestamp = true;
    config.logger.min_lux_threshold = 0.0f;
    config.logger.max_lux_threshold = 100000.0f;
    config.logger.filter_noise = true;
    config.logger.min_quality_threshold = 50;
    config.logger.max_file_size_bytes = 1024 * 1024;
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
    strncpy(config.device_id, "light_sensor_001", MAX_DEVICE_ID_LEN - 1);
    strncpy(config.firmware_version, "1.0.0", MAX_VERSION_LEN - 1);
    config.enable_debug_mode = false;
    config.system_timeout_ms = 300000;
    config.enable_watchdog = true;
    config.watchdog_timeout_ms = 8000;
    
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
    strncpy(calibration.calibration_method, "None", MAX_METHOD_LEN - 1);
    return calibration;
}

// ConfigPresets Implementation
SystemConfig ConfigPresets::getLowPowerPreset() {
    SystemConfig config = ConfigManager::getDefaultConfig();
    
    config.sensor.sample_rate_ms = 5000;
    config.sensor.oversampling = 1;
    config.sensor.low_power_mode = true;
    config.sensor.sleep_duration_ms = 1000;
    
    config.power.sleep_timeout_ms = 10000;
    config.power.deep_sleep_timeout_ms = 60000;
    config.power.disable_unused_peripherals = true;
    config.power.reduce_clock_speed = true;
    
    config.logger.buffer_size = 50;
    config.logger.flush_threshold = 25;
    
    config.signal.moving_average_window = 3;
    config.signal.enable_median_filter = false;
    config.signal.enable_adaptive_filter = false;
    
    return config;
}

SystemConfig ConfigPresets::getHighAccuracyPreset() {
    SystemConfig config = ConfigManager::getDefaultConfig();
    
    config.sensor.sample_rate_ms = 100;
    config.sensor.oversampling = 16;
    config.sensor.auto_gain = true;
    
    config.logger.buffer_size = 500;
    config.logger.flush_threshold = 100;
    config.logger.filter_noise = true;
    config.logger.min_quality_threshold = 80;
    
    config.signal.moving_average_window = 10;
    config.signal.enable_median_filter = true;
    config.signal.median_window = 5;
    config.signal.enable_outlier_removal = true;
    config.signal.outlier_threshold = 1.5f;
    config.signal.enable_adaptive_filter = true;
    
    return config;
}

SystemConfig ConfigPresets::getBalancedPreset() {
    return ConfigManager::getDefaultConfig();
}

SystemConfig ConfigPresets::getDevelopmentPreset() {
    SystemConfig config = ConfigManager::getDefaultConfig();
    
    config.sensor.sample_rate_ms = 500;
    config.enable_debug_mode = true;
    
    config.logger.enable_timestamp = true;
    config.logger.min_quality_threshold = 0;
    
    config.signal.enable_trend_detection = true;
    config.signal.enable_peak_detection = true;
    
    return config;
}

SystemConfig ConfigPresets::getPreset(const char* preset_name) {
    if (strcmp(preset_name, "low_power") == 0) {
        return getLowPowerPreset();
    } else if (strcmp(preset_name, "high_accuracy") == 0) {
        return getHighAccuracyPreset();
    } else if (strcmp(preset_name, "balanced") == 0) {
        return getBalancedPreset();
    } else if (strcmp(preset_name, "development") == 0) {
        return getDevelopmentPreset();
    }
    
    return ConfigManager::getDefaultConfig();
}

}  // namespace LightSensor
