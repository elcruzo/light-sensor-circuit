#include "config_manager.h"
#include <cassert>
#include <iostream>

using namespace LightSensor;

void testConfigManagerInitialization() {
    std::cout << "Testing config manager initialization..." << std::endl;
    
    ConfigManager config_manager("test_config.json");
    
    // Test initialization
    assert(config_manager.initialize() == true);
    
    // Test getting default config
    SystemConfig config = config_manager.getConfig();
    assert(config.device_id.length() > 0);
    assert(config.firmware_version.length() > 0);
    
    std::cout << "✓ Config manager initialization passed" << std::endl;
}

void testConfigurationValidation() {
    std::cout << "Testing configuration validation..." << std::endl;
    
    ConfigManager config_manager("test_config.json");
    config_manager.initialize();
    
    // Get default config
    SystemConfig config = config_manager.getConfig();
    
    // Test validation
    ConfigValidation validation = config_manager.validateConfig(config);
    assert(validation.is_valid == true);
    
    // Test invalid config
    SystemConfig invalid_config = config;
    invalid_config.sensor.adc_resolution = 0; // Invalid
    invalid_config.sensor.reference_voltage = -1.0f; // Invalid
    
    ConfigValidation invalid_validation = config_manager.validateConfig(invalid_config);
    assert(invalid_validation.is_valid == false);
    assert(invalid_validation.errors.size() > 0);
    
    std::cout << "✓ Configuration validation passed" << std::endl;
}

void testCalibrationData() {
    std::cout << "Testing calibration data..." << std::endl;
    
    ConfigManager config_manager("test_config.json");
    config_manager.initialize();
    
    // Get initial calibration data
    CalibrationData initial_calibration = config_manager.getCalibrationData();
    assert(initial_calibration.is_valid == false); // Should be invalid initially
    
    // Test calibration
    float dark_value = 0.1f;
    float light_value = 0.8f;
    float light_lux = 1000.0f;
    
    assert(config_manager.calibrateSensor(dark_value, light_value, light_lux) == true);
    
    // Check updated calibration data
    CalibrationData updated_calibration = config_manager.getCalibrationData();
    assert(updated_calibration.is_valid == true);
    assert(updated_calibration.dark_reference == dark_value);
    assert(updated_calibration.light_reference == light_lux);
    assert(updated_calibration.sensitivity > 0.0f);
    
    std::cout << "✓ Calibration data passed" << std::endl;
    std::cout << "  Dark reference: " << updated_calibration.dark_reference << std::endl;
    std::cout << "  Light reference: " << updated_calibration.light_reference << std::endl;
    std::cout << "  Sensitivity: " << updated_calibration.sensitivity << std::endl;
}

void testConfigurationPresets() {
    std::cout << "Testing configuration presets..." << std::endl;
    
    // Test low power preset
    SystemConfig low_power_config = ConfigPresets::getLowPowerPreset();
    assert(low_power_config.sensor.sample_rate_ms > 1000); // Should be slower
    assert(low_power_config.sensor.oversampling == 1); // Minimal oversampling
    assert(low_power_config.power.sleep_timeout_ms < 60000); // Quick sleep
    
    // Test high accuracy preset
    SystemConfig high_accuracy_config = ConfigPresets::getHighAccuracyPreset();
    assert(high_accuracy_config.sensor.sample_rate_ms < 1000); // Should be faster
    assert(high_accuracy_config.sensor.oversampling > 1); // More oversampling
    assert(high_accuracy_config.signal.enable_outlier_removal == true);
    
    // Test balanced preset
    SystemConfig balanced_config = ConfigPresets::getBalancedPreset();
    assert(balanced_config.sensor.sample_rate_ms == 1000); // Default 1 second
    
    // Test development preset
    SystemConfig dev_config = ConfigPresets::getDevelopmentPreset();
    assert(dev_config.enable_debug_mode == true);
    assert(dev_config.logger.min_quality_threshold == 0); // Log all data
    
    std::cout << "✓ Configuration presets passed" << std::endl;
}

void testConfigValueAccess() {
    std::cout << "Testing configuration value access..." << std::endl;
    
    ConfigManager config_manager("test_config.json");
    config_manager.initialize();
    
    // Test getting config values
    std::string device_id = config_manager.getConfigValue("device_id");
    assert(device_id.length() > 0);
    
    std::string firmware_version = config_manager.getConfigValue("firmware_version");
    assert(firmware_version.length() > 0);
    
    // Test setting config values
    std::string new_device_id = "test_device_123";
    assert(config_manager.setConfigValue("device_id", new_device_id) == true);
    
    std::string updated_device_id = config_manager.getConfigValue("device_id");
    assert(updated_device_id == new_device_id);
    
    std::cout << "✓ Configuration value access passed" << std::endl;
}

void testJsonExportImport() {
    std::cout << "Testing JSON export/import..." << std::endl;
    
    ConfigManager config_manager("test_config.json");
    config_manager.initialize();
    
    // Test JSON export
    std::string json_config = config_manager.exportToJson();
    assert(json_config.length() > 0);
    assert(json_config.find("device_id") != std::string::npos);
    assert(json_config.find("sensor") != std::string::npos);
    
    // Test JSON import
    assert(config_manager.importFromJson(json_config) == true);
    
    std::cout << "✓ JSON export/import passed" << std::endl;
}

void testResetToDefaults() {
    std::cout << "Testing reset to defaults..." << std::endl;
    
    ConfigManager config_manager("test_config.json");
    config_manager.initialize();
    
    // Modify some values
    config_manager.setConfigValue("device_id", "modified_device");
    config_manager.setConfigValue("enable_debug_mode", "true");
    
    // Reset to defaults
    assert(config_manager.resetToDefaults() == true);
    
    // Check that values are reset
    SystemConfig config = config_manager.getConfig();
    assert(config.device_id == "light_sensor_001"); // Default value
    assert(config.enable_debug_mode == false); // Default value
    
    std::cout << "✓ Reset to defaults passed" << std::endl;
}

int runConfigManagerTests() {
    try {
        testConfigManagerInitialization();
        testConfigurationValidation();
        testCalibrationData();
        testConfigurationPresets();
        testConfigValueAccess();
        testJsonExportImport();
        testResetToDefaults();
        
        return 0;
    } catch (const std::exception& e) {
        std::cout << "\n✗ Test failed: " << e.what() << std::endl;
        return 1;
    }
}
