# ESP32 Light Sensor Tests

This library requires real ESP32 hardware to test. Desktop simulation is not supported.

## Hardware Test Procedure

### Requirements
- ESP32 development board
- Light sensor (photodiode or phototransistor) connected to GPIO 34
- Serial terminal (115200 baud)

### Running Tests

1. Connect hardware as described in the main README
2. Build and upload:
   ```bash
   pio run -t upload
   ```
3. Open serial monitor:
   ```bash
   pio device monitor
   ```
4. Verify:
   - System initializes without errors
   - Light readings change when you cover/uncover the sensor
   - Readings are stable (not jumping randomly)

### Calibration Test

1. Cover the sensor completely (dark condition)
2. Note the raw reading
3. Expose sensor to bright light (known lux value)
4. Note the raw reading
5. Use these values to calibrate via config

### Power Management Test

1. Let device sit idle for 30+ seconds
2. Verify it enters low power mode (reduced readings)
3. Wave hand over sensor
4. Verify it wakes up and resumes normal operation

### Data Logging Test

1. Let device run for 5+ minutes
2. Connect to SPIFFS:
   ```bash
   pio run -t uploadfs
   pio device monitor
   ```
3. Check that log files are created in /logs
