#pragma once

#include "light_sensor.h"
#include <cstdint>
#include <functional>
#include <FS.h>

namespace LightSensor {

// Maximum path length for log files
static const size_t MAX_LOG_PATH_LEN = 64;

/**
 * @brief Data logging configuration
 */
struct LoggerConfig {
    // Storage configuration
    char log_file_path[MAX_LOG_PATH_LEN];
    size_t buffer_size;
    size_t flush_threshold;
    bool enable_compression;
    bool enable_timestamp;
    
    // Data filtering
    float min_lux_threshold;
    float max_lux_threshold;
    bool filter_noise;
    uint8_t min_quality_threshold;
    
    // Storage management
    size_t max_file_size_bytes;
    uint32_t max_log_days;
    bool enable_rotation;
};

/**
 * @brief Data statistics
 */
struct DataStats {
    uint32_t total_readings;
    uint32_t valid_readings;
    uint32_t filtered_readings;
    float min_lux;
    float max_lux;
    float average_lux;
    float std_deviation;
    uint32_t buffer_overflow_count;
    size_t current_buffer_size;
};

/**
 * @brief Abstract base class for data storage
 */
class IDataStorage {
public:
    virtual ~IDataStorage() = default;
    virtual bool initialize() = 0;
    virtual bool write(const SensorReading& data) = 0;
    virtual bool flush() = 0;
    virtual void close() = 0;
    virtual size_t getAvailableSpace() const = 0;
};

/**
 * @brief SPIFFS-based data storage implementation for ESP32
 */
class SPIFFSDataStorage : public IDataStorage {
public:
    explicit SPIFFSDataStorage(const LoggerConfig& config);
    ~SPIFFSDataStorage() override;
    
    bool initialize() override;
    bool write(const SensorReading& data) override;
    bool flush() override;
    void close() override;
    size_t getAvailableSpace() const override;
    
private:
    LoggerConfig config_;
    File log_file_;
    char current_file_path_[MAX_LOG_PATH_LEN];
    size_t current_file_size_;
    bool is_initialized_;
    
    bool createNewLogFile();
    bool needsRotation() const;
    bool rotateLogFile();
    void formatReading(const SensorReading& reading, char* buffer, size_t buffer_size) const;
};

/**
 * @brief Memory-based circular buffer storage
 */
class MemoryDataStorage : public IDataStorage {
public:
    static const size_t MAX_BUFFER_SIZE = 100;
    
    explicit MemoryDataStorage(const LoggerConfig& config);
    ~MemoryDataStorage() override = default;
    
    bool initialize() override;
    bool write(const SensorReading& data) override;
    bool flush() override;
    void close() override;
    size_t getAvailableSpace() const override;
    
    size_t getDataCount() const;
    bool getData(size_t index, SensorReading& reading) const;
    void clear();
    
private:
    LoggerConfig config_;
    SensorReading data_buffer_[MAX_BUFFER_SIZE];
    size_t buffer_head_;
    size_t buffer_count_;
    bool is_initialized_;
};

/**
 * @brief ESP32 Data logger with SPIFFS storage
 */
class DataLogger {
public:
    static const size_t MAX_QUEUE_SIZE = 50;
    
    explicit DataLogger(const LoggerConfig& config);
    ~DataLogger();
    
    bool initialize();
    bool logReading(const SensorReading& reading);
    void startLogging(ILightSensor* sensor);
    void stopLogging();
    bool flush();
    DataStats getStats() const;
    void configure(const LoggerConfig& config);
    void setStorage(IDataStorage* storage);
    void process();
    bool isLogging() const;
    
private:
    LoggerConfig config_;
    IDataStorage* storage_;
    bool owns_storage_;
    
    // Simple circular buffer instead of std::queue
    SensorReading buffer_[MAX_QUEUE_SIZE];
    size_t buffer_head_;
    size_t buffer_tail_;
    size_t buffer_count_;
    
    volatile bool is_logging_;
    volatile bool should_stop_;
    DataStats stats_;
    
    ILightSensor* sensor_;
    uint32_t last_log_time_ms_;
    
    bool shouldLogReading(const SensorReading& reading) const;
    void updateStats(const SensorReading& reading);
    void processBuffer();
    bool enqueue(const SensorReading& reading);
    bool dequeue(SensorReading& reading);
};

}  // namespace LightSensor
