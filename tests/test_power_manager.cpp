#include "power_manager.h"
#include <cassert>
#include <iostream>
#include <thread>
#include <chrono>

using namespace LightSensor;

void testPowerManagerInitialization() {
    std::cout << "Testing power manager initialization..." << std::endl;
    
    PowerConfig config;
    config.sleep_timeout_ms = 30000;
    config.deep_sleep_timeout_ms = 300000;
    config.enable_wake_on_light = true;
    config.light_threshold = 0.1f;
    config.disable_unused_peripherals = true;
    config.reduce_clock_speed = true;
    config.adc_sample_delay_ms = 1;
    config.low_battery_threshold = 3.2f;
    config.critical_battery_threshold = 3.0f;
    config.enable_battery_monitoring = true;
    
    PowerManager power_manager(config);
    
    // Test initialization
    assert(power_manager.initialize() == true);
    assert(power_manager.getCurrentMode() == PowerMode::ACTIVE);
    
    std::cout << "✓ Power manager initialization passed" << std::endl;
}

void testPowerModeChanges() {
    std::cout << "Testing power mode changes..." << std::endl;
    
    PowerConfig config;
    PowerManager power_manager(config);
    power_manager.initialize();
    
    // Test mode changes
    power_manager.setPowerMode(PowerMode::LOW_POWER);
    assert(power_manager.getCurrentMode() == PowerMode::LOW_POWER);
    
    power_manager.setPowerMode(PowerMode::SLEEP);
    assert(power_manager.getCurrentMode() == PowerMode::SLEEP);
    
    power_manager.setPowerMode(PowerMode::ACTIVE);
    assert(power_manager.getCurrentMode() == PowerMode::ACTIVE);
    
    std::cout << "✓ Power mode changes passed" << std::endl;
}

void testBatteryMonitoring() {
    std::cout << "Testing battery monitoring..." << std::endl;
    
    PowerConfig config;
    config.enable_battery_monitoring = true;
    config.low_battery_threshold = 3.2f;
    config.critical_battery_threshold = 3.0f;
    
    PowerManager power_manager(config);
    power_manager.initialize();
    
    // Test normal battery level
    power_manager.updateBatteryVoltage(3.7f);
    assert(power_manager.isBatteryLow() == false);
    assert(power_manager.isBatteryCritical() == false);
    
    // Test low battery level
    power_manager.updateBatteryVoltage(3.1f);
    assert(power_manager.isBatteryLow() == true);
    assert(power_manager.isBatteryCritical() == false);
    
    // Test critical battery level
    power_manager.updateBatteryVoltage(2.9f);
    assert(power_manager.isBatteryLow() == true);
    assert(power_manager.isBatteryCritical() == true);
    
    std::cout << "✓ Battery monitoring passed" << std::endl;
}

void testPowerStats() {
    std::cout << "Testing power statistics..." << std::endl;
    
    PowerConfig config;
    PowerManager power_manager(config);
    power_manager.initialize();
    
    // Get initial stats
    PowerStats stats = power_manager.getPowerStats();
    assert(stats.wake_count >= 0);
    assert(stats.average_current_ma >= 0.0f);
    assert(stats.peak_current_ma >= 0.0f);
    
    std::cout << "✓ Power statistics passed" << std::endl;
}

int runPowerManagerTests() {
    try {
        testPowerManagerInitialization();
        testPowerModeChanges();
        testBatteryMonitoring();
        testPowerStats();
        
        return 0;
    } catch (const std::exception& e) {
        std::cout << "\n✗ Test failed: " << e.what() << std::endl;
        return 1;
    }
}
