# ESP32 Light Sensor

Light sensor library for ESP32. Requires actual hardware.

## Hardware

- **ESP32** (ESP32, ESP32-S3, or ESP32-C3)
- **Light sensor** on GPIO 34 (photodiode, phototransistor, or LDR)
- **Optional**: Battery voltage divider on GPIO 35

## Wiring

```
Light Sensor:
    Anode (+)  → 3.3V
    Cathode (-) → GPIO 34
    Cathode (-) → 10kΩ resistor → GND

Battery Monitor (optional):
    Battery (+) → 100kΩ → GPIO 35 → 100kΩ → GND
```

## Build

```bash
# Install PlatformIO
pip install platformio

# Build
pio run

# Upload config files to SPIFFS
pio run -t uploadfs

# Upload firmware
pio run -t upload

# Monitor serial output
pio device monitor
```

## Configuration

Edit `data/config.json`:

```json
{
  "sensor": {
    "adc_pin": 34,
    "sample_rate_ms": 1000,
    "oversampling": 4
  }
}
```

## Calibration

1. Cover sensor → note reading (dark reference)
2. Expose to known light level → note reading
3. Update `data/calibration.json`:

```json
{
  "dark_reference": 0.05,
  "light_reference": 1000.0,
  "sensitivity": 0.003,
  "is_valid": true
}
```

## Structure

```
src/
├── main.cpp        # Entry point
├── core/           # Sensor driver
├── power/          # Power management
├── storage/        # Data logging (SPIFFS)
├── signal/         # Filtering
├── config/         # JSON config
└── utils/          # Logger, timer
```

## Boards

```bash
pio run -e esp32dev   # ESP32
pio run -e esp32-s3   # ESP32-S3
pio run -e esp32-c3   # ESP32-C3
```
