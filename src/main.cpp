/**
 * ESP32 Light Sensor - Main Entry Point
 * 
 * This is a production-ready light sensor system for ESP32.
 * Connect a photodiode or phototransistor to GPIO 34 (ADC1_CH6).
 * 
 * Hardware Requirements:
 * - ESP32 development board
 * - Light sensor (photodiode/phototransistor) on GPIO 34
 * - Optional: battery voltage divider on GPIO 35
 */

#include <Arduino.h>
#include <SPIFFS.h>
#include "light_sensor.h"
#include "power_manager.h"
#include "data_logger.h"
#include "signal_processor.h"
#include "config_manager.h"
#include "logger.h"
#include "timer.h"

using namespace LightSensor;

// Global instances
ConfigManager* configManager = nullptr;
ADCLightSensor* sensor = nullptr;
PowerManager* powerManager = nullptr;
DataLogger* dataLogger = nullptr;
SignalProcessor* signalProcessor = nullptr;
Logger& logger = Logger::getInstance();

// Battery monitoring pin (optional)
static const uint8_t BATTERY_PIN = 35;

// Forward declarations
void initializeSystem();
void processReading();
void checkBattery();

void setup() {
    // Initialize serial
    Serial.begin(115200);
    while (!Serial && millis() < 3000) {
        delay(10);
    }
    
    Serial.println();
    Serial.println("========================================");
    Serial.println("  ESP32 Light Sensor System v1.0.0");
    Serial.println("========================================");
    Serial.println();
    
    // Initialize the system
    initializeSystem();
}

void loop() {
    static uint32_t last_reading_time = 0;
    static uint32_t last_battery_check = 0;
    const uint32_t now = millis();
    
    // Get current config
    const SystemConfig& config = configManager->getConfig();
    
    // Take sensor readings at configured rate
    if (now - last_reading_time >= config.sensor.sample_rate_ms) {
        processReading();
        last_reading_time = now;
    }
    
    // Check battery every 10 seconds
    if (config.power.enable_battery_monitoring && now - last_battery_check >= 10000) {
        checkBattery();
        last_battery_check = now;
    }
    
    // Process sensor (handles continuous sampling if enabled)
    sensor->process();
    
    // Process data logger
    dataLogger->process();
    
    // Process power management
    powerManager->process();
    
    // Small delay to prevent tight loop
    delay(10);
}

void initializeSystem() {
    // Set up logger
    logger.setLevel(LogLevel::INFO);
    logger.setOutput(LogOutput::SERIAL);
    logger.info("Initializing system...");
    
    // Initialize configuration manager
    configManager = new ConfigManager("/config.json");
    if (!configManager->initialize()) {
        logger.error("Failed to initialize config manager!");
        logger.info("Using default configuration");
    } else {
        logger.info("Configuration loaded from SPIFFS");
    }
    
    const SystemConfig& config = configManager->getConfig();
    
    // Enable debug logging if configured
    if (config.enable_debug_mode) {
        logger.setLevel(LogLevel::DEBUG);
        logger.debug("Debug mode enabled");
    }
    
    // Initialize sensor
    sensor = new ADCLightSensor(config.sensor);
    if (!sensor->initialize()) {
        logger.critical("Failed to initialize light sensor!");
        logger.error("Check that GPIO 34 is connected to a light sensor");
        
        // Blink LED to indicate error (if available)
        while (true) {
            delay(500);
        }
    }
    logger.info("Light sensor initialized on GPIO 34");
    
    // Initialize power manager
    powerManager = new PowerManager(config.power);
    if (!powerManager->initialize()) {
        logger.error("Failed to initialize power manager");
    } else {
        logger.info("Power manager initialized");
    }
    
    // Initialize data logger
    dataLogger = new DataLogger(config.logger);
    if (!dataLogger->initialize()) {
        logger.warning("Failed to initialize data logger - logging disabled");
    } else {
        logger.info("Data logger initialized");
    }
    
    // Initialize signal processor
    signalProcessor = new SignalProcessor(config.signal);
    logger.info("Signal processor initialized");
    
    // Check if calibration is valid
    const CalibrationData& calibration = configManager->getCalibrationData();
    if (!calibration.is_valid) {
        logger.warning("Sensor not calibrated - readings may be inaccurate");
        logger.info("Run calibration procedure for accurate lux readings");
    } else {
        char msg[64];
        snprintf(msg, sizeof(msg), "Calibration: dark=%.2f, sensitivity=%.4f", 
                 calibration.dark_reference, calibration.sensitivity);
        logger.info(msg);
    }
    
    logger.info("System initialization complete");
    Serial.println();
}

void processReading() {
    // Read sensor
    SensorReading reading = sensor->read();
    
    if (!reading.is_valid) {
        logger.warning("Invalid sensor reading");
        return;
    }
    
    // Process signal
    SignalAnalysis analysis = signalProcessor->processReading(reading);
    
    // Log the reading
    dataLogger->logReading(reading);
    
    // Record activity for power management
    powerManager->recordActivity();
    
    // Output reading
    const SystemConfig& config = configManager->getConfig();
    if (config.enable_debug_mode) {
        char msg[128];
        snprintf(msg, sizeof(msg), 
                 "Lux: %.2f (filtered: %.2f), Quality: %u, SNR: %.2f",
                 reading.lux_value, analysis.filtered_value, 
                 analysis.quality_score, analysis.signal_to_noise_ratio);
        logger.debug(msg);
    }
    
    // Check for trends
    if (analysis.trend_confidence > 0.8f) {
        if (analysis.trend_slope > 0) {
            logger.debug("Light level increasing");
        } else if (analysis.trend_slope < 0) {
            logger.debug("Light level decreasing");
        }
    }
    
    // Check for outliers
    if (analysis.is_outlier) {
        logger.warning("Outlier detected in reading");
    }
}

void checkBattery() {
    // Read battery voltage (assumes voltage divider: Vbat -> 100k -> GPIO35 -> 100k -> GND)
    int raw = analogRead(BATTERY_PIN);
    float voltage = (raw / 4095.0f) * 3.3f * 2.0f;  // *2 for voltage divider
    
    powerManager->updateBatteryVoltage(voltage);
    
    if (powerManager->isBatteryCritical()) {
        char msg[48];
        snprintf(msg, sizeof(msg), "CRITICAL: Battery at %.2fV!", voltage);
        logger.critical(msg);
    } else if (powerManager->isBatteryLow()) {
        char msg[48];
        snprintf(msg, sizeof(msg), "Low battery: %.2fV", voltage);
        logger.warning(msg);
    }
}
