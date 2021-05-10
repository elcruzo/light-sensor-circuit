#pragma once

#include "light_sensor.h"
#include <vector>
#include <queue>
#include <memory>
#include <fstream>
#include <string>
#include <mutex>
#include <atomic>

namespace LightSensor {

/**
 * @brief Data logging configuration
 */
struct LoggerConfig {
    // Storage configuration
    std::string log_file_path;        // Path to log file
    size_t buffer_size;               // Buffer size for readings
    size_t flush_threshold;           // Threshold for flushing buffer
    bool enable_compression;          // Enable data compression
    bool enable_timestamp;            // Include timestamps in logs
    
    // Data filtering
    float min_lux_threshold;          // Minimum lux value to log
    float max_lux_threshold;          // Maximum lux value to log
    bool filter_noise;                // Enable noise filtering
    uint8_t min_quality_threshold;    // Minimum quality threshold
    
    // Storage management
    size_t max_file_size_bytes;       // Maximum log file size
    uint32_t max_log_days;            // Maximum days to keep logs
    bool enable_rotation;             // Enable log file rotation
};

/**
 * @brief Data statistics
 */
struct DataStats {
    uint32_t total_readings;          // Total number of readings
    uint32_t valid_readings;          // Number of valid readings
    uint32_t filtered_readings;       // Number of filtered readings
    float min_lux;                    // Minimum lux value recorded
    float max_lux;                    // Maximum lux value recorded
    float average_lux;                // Average lux value
    float std_deviation;              // Standard deviation
    uint32_t buffer_overflow_count;   // Number of buffer overflows
    size_t current_buffer_size;       // Current buffer size
};

/**
 * @brief Abstract base class for data storage
 */
class IDataStorage {
public:
    virtual ~IDataStorage() = default;
    
    /**
     * @brief Initialize storage
     * @return true if initialization successful
     */
    virtual bool initialize() = 0;
    
    /**
     * @brief Write data to storage
     * @param data Data to write
     * @return true if write successful
     */
    virtual bool write(const SensorReading& data) = 0;
    
    /**
     * @brief Flush pending data
     * @return true if flush successful
     */
    virtual bool flush() = 0;
    
    /**
     * @brief Close storage
     */
    virtual void close() = 0;
    
    /**
     * @brief Get available storage space
     * @return Available space in bytes
     */
    virtual size_t getAvailableSpace() const = 0;
};

/**
 * @brief File-based data storage implementation
 */
class FileDataStorage : public IDataStorage {
public:
    explicit FileDataStorage(const LoggerConfig& config);
    ~FileDataStorage() override;
    
    bool initialize() override;
    bool write(const SensorReading& data) override;
    bool flush() override;
    void close() override;
    size_t getAvailableSpace() const override;
    
private:
    LoggerConfig config_;
    std::ofstream log_file_;
    std::string current_file_path_;
    size_t current_file_size_;
    bool is_initialized_;
    
    /**
     * @brief Create new log file
     * @return true if file creation successful
     */
    bool createNewLogFile();
    
    /**
     * @brief Check if log rotation is needed
     * @return true if rotation needed
     */
    bool needsRotation() const;
    
    /**
     * @brief Rotate log file
     * @return true if rotation successful
     */
    bool rotateLogFile();
    
    /**
     * @brief Format reading for logging
     * @param reading Reading to format
     * @return Formatted string
     */
    std::string formatReading(const SensorReading& reading) const;
};

/**
 * @brief Memory-based data storage implementation
 */
class MemoryDataStorage : public IDataStorage {
public:
    explicit MemoryDataStorage(const LoggerConfig& config);
    ~MemoryDataStorage() override = default;
    
    bool initialize() override;
    bool write(const SensorReading& data) override;
    bool flush() override;
    void close() override;
    size_t getAvailableSpace() const override;
    
    /**
     * @brief Get stored data
     * @return Vector of stored readings
     */
    std::vector<SensorReading> getData() const;
    
    /**
     * @brief Clear stored data
     */
    void clear();
    
private:
    LoggerConfig config_;
    std::vector<SensorReading> data_buffer_;
    mutable std::mutex buffer_mutex_;
    bool is_initialized_;
};

/**
 * @brief Real-time data logger with buffering
 */
class DataLogger {
public:
    explicit DataLogger(const LoggerConfig& config);
    ~DataLogger();
    
    /**
     * @brief Initialize logger
     * @return true if initialization successful
     */
    bool initialize();
    
    /**
     * @brief Log sensor reading
     * @param reading Reading to log
     * @return true if logging successful
     */
    bool logReading(const SensorReading& reading);
    
    /**
     * @brief Start continuous logging
     * @param sensor Sensor to log from
     */
    void startLogging(std::shared_ptr<ILightSensor> sensor);
    
    /**
     * @brief Stop continuous logging
     */
    void stopLogging();
    
    /**
     * @brief Flush all pending data
     * @return true if flush successful
     */
    bool flush();
    
    /**
     * @brief Get logging statistics
     * @return Data statistics
     */
    DataStats getStats() const;
    
    /**
     * @brief Configure logger
     * @param config New configuration
     */
    void configure(const LoggerConfig& config);
    
    /**
     * @brief Set storage implementation
     * @param storage Storage implementation to use
     */
    void setStorage(std::unique_ptr<IDataStorage> storage);
    
    /**
     * @brief Process logging (call in main loop)
     */
    void process();
    
    /**
     * @brief Check if logging is active
     * @return true if logging is active
     */
    bool isLogging() const;
    
private:
    LoggerConfig config_;
    std::unique_ptr<IDataStorage> storage_;
    std::queue<SensorReading> buffer_;
    mutable std::mutex buffer_mutex_;
    std::atomic<bool> is_logging_;
    std::atomic<bool> should_stop_;
    DataStats stats_;
    
    std::shared_ptr<ILightSensor> sensor_;
    std::chrono::steady_clock::time_point last_log_time_;
    
    /**
     * @brief Check if reading should be logged
     * @param reading Reading to check
     * @return true if should be logged
     */
    bool shouldLogReading(const SensorReading& reading) const;
    
    /**
     * @brief Update statistics
     * @param reading Reading to update stats with
     */
    void updateStats(const SensorReading& reading);
    
    /**
     * @brief Process buffer (flush if needed)
     */
    void processBuffer();
    
    /**
     * @brief Calculate statistics from readings
     * @param readings Readings to calculate stats from
     * @return Calculated statistics
     */
    DataStats calculateStats(const std::vector<SensorReading>& readings) const;
};

} // namespace LightSensor
