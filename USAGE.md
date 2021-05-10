# Light Sensor Circuit - Usage Guide

This guide explains how to use the light sensor circuit software system.

## Quick Start

### 1. Building the Project

```bash
# Build for desktop (default)
./build.sh

# Build for Arduino
./build.sh -p arduino

# Build with tests
./build.sh --tests

# Clean build
./build.sh -c
```

### 2. Running Examples

```bash
# Basic sensor example
./build_desktop_release/examples/basic_sensor_example

# Complete system example
./build_desktop_release/examples/complete_system_example

# Signal processing example
./build_desktop_release/examples/signal_processing_example
```

## Configuration

### Default Configuration

The system comes with sensible defaults, but you can customize:

```cpp
#include "config_manager.h"

ConfigManager config_manager("config.json");
SystemConfig config = config_manager.getConfig();

// Modify configuration
config.sensor.sample_rate_ms = 500;  // 500ms sampling
config.power.sleep_timeout_ms = 10000;  // 10s sleep timeout

// Apply configuration
config_manager.updateConfig(config);
```

### Configuration Presets

```cpp
// Low power preset
SystemConfig low_power_config = ConfigPresets::getLowPowerPreset();

// High accuracy preset
SystemConfig high_accuracy_config = ConfigPresets::getHighAccuracyPreset();

// Development preset
SystemConfig dev_config = ConfigPresets::getDevelopmentPreset();
```

## Basic Usage

### 1. Simple Sensor Reading

```cpp
#include "light_sensor.h"

// Configure sensor
SensorConfig config;
config.adc_pin = A0;
config.adc_resolution = 10;
config.reference_voltage = 3.3f;

// Create and initialize sensor
ADCLightSensor sensor(config);
sensor.initialize();

// Read sensor
SensorReading reading = sensor.read();
std::cout << "Lux: " << reading.lux_value << std::endl;
```

### 2. Continuous Sampling

```cpp
// Start continuous sampling
sensor.startSampling([](const SensorReading& reading) {
    std::cout << "Lux: " << reading.lux_value << std::endl;
});

// Stop sampling
sensor.stopSampling();
```

### 3. Data Logging

```cpp
#include "data_logger.h"

// Configure logger
LoggerConfig logger_config;
logger_config.log_file_path = "/logs";
logger_config.buffer_size = 100;

// Create logger
DataLogger logger(logger_config);
logger.initialize();

// Start logging
logger.startLogging(sensor);

// Stop logging
logger.stopLogging();
```

### 4. Power Management

```cpp
#include "power_manager.h"

// Configure power management
PowerConfig power_config;
power_config.sleep_timeout_ms = 30000;
power_config.enable_wake_on_light = true;

// Create power manager
PowerManager power_manager(power_config);
power_manager.initialize();

// Process power management (call in main loop)
power_manager.process();
```

### 5. Signal Processing

```cpp
#include "signal_processor.h"

// Configure signal processing
SignalConfig signal_config;
signal_config.moving_average_window = 5;
signal_config.enable_outlier_removal = true;

// Create signal processor
SignalProcessor processor(signal_config);

// Process readings
SensorReading reading = sensor.read();
SignalAnalysis analysis = processor.processReading(reading);

std::cout << "Filtered: " << analysis.filtered_value << std::endl;
std::cout << "Quality: " << analysis.quality_score << std::endl;
```

## Complete System Example

```cpp
#include "light_sensor.h"
#include "power_manager.h"
#include "data_logger.h"
#include "signal_processor.h"
#include "config_manager.h"

int main() {
    // Initialize configuration
    ConfigManager config_manager("config.json");
    config_manager.initialize();
    
    SystemConfig config = config_manager.getConfig();
    
    // Create components
    auto sensor = std::make_shared<ADCLightSensor>(config.sensor);
    PowerManager power_manager(config.power);
    SignalProcessor processor(config.signal);
    DataLogger logger(config.logger);
    
    // Initialize all components
    sensor->initialize();
    power_manager.initialize();
    logger.initialize();
    
    // Start data logging
    logger.startLogging(sensor);
    
    // Main loop
    while (true) {
        // Read and process sensor
        SensorReading reading = sensor->read();
        SignalAnalysis analysis = processor.processReading(reading);
        
        // Log processed data
        SensorReading processed_reading = reading;
        processed_reading.lux_value = analysis.filtered_value;
        logger.logReading(processed_reading);
        
        // Process power management
        power_manager.process();
        
        // Process logger
        logger.process();
        
        // Small delay
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    return 0;
}
```

## Arduino Integration

For Arduino projects, use the Arduino example:

```cpp
#include "light_sensor.h"
#include "power_manager.h"
#include "data_logger.h"

void setup() {
    // Initialize components
    // ... (see arduino_example.cpp)
}

void loop() {
    // Main loop
    // ... (see arduino_example.cpp)
}
```

## Calibration

### Two-Point Calibration

```cpp
// Cover sensor for dark calibration
SensorReading dark_reading = sensor.read();

// Expose to known light source
SensorReading light_reading = sensor.read();

// Calibrate (assuming 1000 lux reference)
config_manager.calibrateSensor(
    dark_reading.raw_value, 
    light_reading.raw_value, 
    1000.0f  // Reference lux value
);
```

## Troubleshooting

### Common Issues

1. **Sensor not initializing**
   - Check ADC pin configuration
   - Verify reference voltage settings
   - Ensure proper power supply

2. **Inaccurate readings**
   - Perform calibration
   - Check for electrical noise
   - Verify signal conditioning

3. **High power consumption**
   - Enable low power mode
   - Increase sleep intervals
   - Disable unused peripherals

4. **Data logging issues**
   - Check file system permissions
   - Verify buffer size settings
   - Monitor available storage

### Debug Mode

Enable debug logging:

```cpp
Logger& logger = Logger::getInstance();
logger.setLevel(LogLevel::DEBUG);
logger.setOutput(LogOutput::CONSOLE);
```

## Performance Optimization

### Low Power Mode

```cpp
// Configure for low power
SystemConfig config = ConfigPresets::getLowPowerPreset();

// Enable power management
power_manager.setPowerMode(PowerMode::LOW_POWER);
```

### High Accuracy Mode

```cpp
// Configure for high accuracy
SystemConfig config = ConfigPresets::getHighAccuracyPreset();

// Use high oversampling
config.sensor.oversampling = 16;
```

## API Reference

See the header files in `include/` for detailed API documentation:

- `light_sensor.h` - Sensor interface
- `power_manager.h` - Power management
- `data_logger.h` - Data logging
- `signal_processor.h` - Signal processing
- `config_manager.h` - Configuration management
