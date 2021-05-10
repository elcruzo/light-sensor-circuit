#include "light_sensor.h"
#include "power_manager.h"
#include "data_logger.h"
#include "signal_processor.h"
#include "config_manager.h"
#include "logger.h"

using namespace LightSensor;

// Global objects
ConfigManager* config_manager;
ADCLightSensor* sensor;
PowerManager* power_manager;
SignalProcessor* signal_processor;
DataLogger* data_logger;

// Timing variables
unsigned long last_sample_time = 0;
unsigned long last_power_check_time = 0;
const unsigned long SAMPLE_INTERVAL = 1000;  // 1 second
const unsigned long POWER_CHECK_INTERVAL = 5000;  // 5 seconds

void setup() {
    // Initialize serial communication
    Serial.begin(115200);
    while (!Serial) {
        delay(10);
    }
    
    Serial.println("Light Sensor Circuit - Arduino Example");
    
    // Initialize logger
    Logger& logger = Logger::getInstance();
    logger.setLevel(LogLevel::INFO);
    logger.setOutput(LogOutput::SERIAL);
    
    logger.info("Initializing system...");
    
    // Initialize configuration manager
    config_manager = new ConfigManager("config.json");
    if (!config_manager->initialize()) {
        logger.error("Failed to initialize configuration manager");
        return;
    }
    
    // Load configuration
    SystemConfig config = config_manager->getConfig();
    logger.info("Configuration loaded");
    
    // Create sensor
    sensor = new ADCLightSensor(config.sensor);
    if (!sensor->initialize()) {
        logger.error("Failed to initialize sensor");
        return;
    }
    
    // Create power manager
    power_manager = new PowerManager(config.power);
    if (!power_manager->initialize()) {
        logger.error("Failed to initialize power manager");
        return;
    }
    
    // Create signal processor
    signal_processor = new SignalProcessor(config.signal);
    
    // Create data logger
    data_logger = new DataLogger(config.logger);
    if (!data_logger->initialize()) {
        logger.error("Failed to initialize data logger");
        return;
    }
    
    // Set up power event callback
    power_manager->setPowerEventCallback([](PowerMode mode, WakeSource source) {
        Logger& logger = Logger::getInstance();
        String mode_str = (mode == PowerMode::ACTIVE) ? "ACTIVE" :
                         (mode == PowerMode::LOW_POWER) ? "LOW_POWER" :
                         (mode == PowerMode::SLEEP) ? "SLEEP" : "DEEP_SLEEP";
        logger.info("Power mode changed to: " + String(mode_str).c_str());
    });
    
    // Perform sensor calibration
    logger.info("Performing sensor calibration...");
    logger.info("Please cover the sensor for dark calibration");
    delay(3000);
    
    SensorReading dark_reading = sensor->read();
    logger.info("Dark reading: " + String(dark_reading.lux_value) + " lux");
    
    logger.info("Please expose sensor to bright light for light calibration");
    delay(3000);
    
    SensorReading light_reading = sensor->read();
    logger.info("Light reading: " + String(light_reading.lux_value) + " lux");
    
    // Calibrate sensor
    if (config_manager->calibrateSensor(dark_reading.raw_value, light_reading.raw_value, 1000.0f)) {
        logger.info("Sensor calibration completed and saved");
    } else {
        logger.warning("Sensor calibration failed");
    }
    
    // Start data logging
    logger.info("Starting data logging...");
    data_logger->startLogging(std::shared_ptr<ILightSensor>(sensor));
    
    logger.info("System initialized successfully");
    last_sample_time = millis();
    last_power_check_time = millis();
}

void loop() {
    unsigned long current_time = millis();
    
    // Sample sensor
    if (current_time - last_sample_time >= SAMPLE_INTERVAL) {
        SensorReading reading = sensor->read();
        
        // Process signal
        SignalAnalysis analysis = signal_processor->processReading(reading);
        
        // Log processed data
        SensorReading processed_reading = reading;
        processed_reading.lux_value = analysis.filtered_value;
        processed_reading.quality = analysis.quality_score;
        
        data_logger->logReading(processed_reading);
        
        // Log analysis results
        Logger& logger = Logger::getInstance();
        logger.info("Sample - Raw: " + String(reading.lux_value) + 
                   " lux, Filtered: " + String(analysis.filtered_value) + 
                   " lux, Quality: " + String(analysis.quality_score) + 
                   ", SNR: " + String(analysis.signal_to_noise_ratio));
        
        if (analysis.is_outlier) {
            logger.warning("Outlier detected in reading");
        }
        
        if (analysis.is_peak) {
            logger.info("Peak detected in reading");
        }
        
        if (analysis.trend_confidence > 0.7f) {
            String trend = analysis.trend_slope > 0 ? "increasing" : "decreasing";
            logger.info("Trend detected: " + trend + " (confidence: " + 
                       String(analysis.trend_confidence) + ")");
        }
        
        last_sample_time = current_time;
    }
    
    // Check power management
    if (current_time - last_power_check_time >= POWER_CHECK_INTERVAL) {
        power_manager->process();
        
        // Update battery voltage (mock - in real implementation, read from ADC)
        float battery_voltage = 3.7f; // Mock battery voltage
        power_manager->updateBatteryVoltage(battery_voltage);
        
        if (power_manager->isBatteryLow()) {
            Logger& logger = Logger::getInstance();
            logger.warning("Battery is low");
        }
        
        if (power_manager->isBatteryCritical()) {
            Logger& logger = Logger::getInstance();
            logger.error("Battery is critical - entering deep sleep");
            power_manager->setPowerMode(PowerMode::DEEP_SLEEP);
        }
        
        last_power_check_time = current_time;
    }
    
    // Process data logger
    data_logger->process();
    
    // Small delay to prevent excessive CPU usage
    delay(10);
}

// Cleanup function (called when system shuts down)
void cleanup() {
    if (data_logger) {
        data_logger->stopLogging();
        delete data_logger;
    }
    
    if (signal_processor) {
        delete signal_processor;
    }
    
    if (power_manager) {
        delete power_manager;
    }
    
    if (sensor) {
        delete sensor;
    }
    
    if (config_manager) {
        delete config_manager;
    }
}
