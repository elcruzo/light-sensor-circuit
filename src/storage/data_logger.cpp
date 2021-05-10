#include "data_logger.h"
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <cmath>
#include <vector>
#include <queue>
#include <mutex>
#include <atomic>

#ifdef ARDUINO
#include <Arduino.h>
#include <SD.h>
#else
#include <fstream>
#include <iostream>
#endif

namespace LightSensor {

// FileDataStorage Implementation
FileDataStorage::FileDataStorage(const LoggerConfig& config)
    : config_(config), current_file_size_(0), is_initialized_(false) {
}

FileDataStorage::~FileDataStorage() {
    close();
}

bool FileDataStorage::initialize() {
    if (is_initialized_) {
        return true;
    }
    
    return createNewLogFile();
}

bool FileDataStorage::write(const SensorReading& data) {
    if (!is_initialized_ || !log_file_.is_open()) {
        return false;
    }
    
    // Check if rotation is needed
    if (needsRotation()) {
        if (!rotateLogFile()) {
            return false;
        }
    }
    
    std::string formatted_data = formatReading(data);
    log_file_ << formatted_data << std::endl;
    
    if (log_file_.fail()) {
        return false;
    }
    
    current_file_size_ += formatted_data.length() + 1; // +1 for newline
    return true;
}

bool FileDataStorage::flush() {
    if (log_file_.is_open()) {
        log_file_.flush();
        return !log_file_.fail();
    }
    return true;
}

void FileDataStorage::close() {
    if (log_file_.is_open()) {
        log_file_.close();
    }
    is_initialized_ = false;
}

size_t FileDataStorage::getAvailableSpace() const {
    // This is a simplified implementation
    // In a real system, you'd check actual storage space
    return 1024 * 1024; // 1MB placeholder
}

bool FileDataStorage::createNewLogFile() {
    // Generate timestamp-based filename
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << config_.log_file_path << "/light_sensor_";
    ss << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S");
    ss << ".log";
    
    current_file_path_ = ss.str();
    
    #ifdef ARDUINO
    // Arduino SD card implementation
    if (!SD.begin()) {
        return false;
    }
    log_file_ = SD.open(current_file_path_.c_str(), FILE_WRITE);
    #else
    // Standard C++ file implementation
    log_file_.open(current_file_path_, std::ios::out | std::ios::app);
    #endif
    
    if (!log_file_.is_open()) {
        return false;
    }
    
    // Write header
    log_file_ << "# Light Sensor Data Log" << std::endl;
    log_file_ << "# Format: timestamp_ms,raw_value,lux_value,voltage,quality" << std::endl;
    
    is_initialized_ = true;
    current_file_size_ = 0;
    return true;
}

bool FileDataStorage::needsRotation() const {
    return config_.enable_rotation && 
           current_file_size_ > config_.max_file_size_bytes;
}

bool FileDataStorage::rotateLogFile() {
    close();
    return createNewLogFile();
}

std::string FileDataStorage::formatReading(const SensorReading& reading) const {
    std::stringstream ss;
    
    if (config_.enable_timestamp) {
        ss << reading.timestamp_ms << ",";
    }
    
    ss << std::fixed << std::setprecision(6);
    ss << reading.raw_value << ",";
    ss << reading.lux_value << ",";
    ss << reading.voltage << ",";
    ss << static_cast<int>(reading.quality);
    
    return ss.str();
}

// MemoryDataStorage Implementation
MemoryDataStorage::MemoryDataStorage(const LoggerConfig& config)
    : config_(config), is_initialized_(false) {
    data_buffer_.reserve(config_.buffer_size);
}

bool MemoryDataStorage::initialize() {
    is_initialized_ = true;
    return true;
}

bool MemoryDataStorage::write(const SensorReading& data) {
    if (!is_initialized_) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(buffer_mutex_);
    
    if (data_buffer_.size() >= config_.buffer_size) {
        // Buffer full, remove oldest entry
        data_buffer_.erase(data_buffer_.begin());
    }
    
    data_buffer_.push_back(data);
    return true;
}

bool MemoryDataStorage::flush() {
    // Memory storage doesn't need flushing
    return true;
}

void MemoryDataStorage::close() {
    std::lock_guard<std::mutex> lock(buffer_mutex_);
    data_buffer_.clear();
    is_initialized_ = false;
}

size_t MemoryDataStorage::getAvailableSpace() const {
    std::lock_guard<std::mutex> lock(buffer_mutex_);
    return config_.buffer_size - data_buffer_.size();
}

std::vector<SensorReading> MemoryDataStorage::getData() const {
    std::lock_guard<std::mutex> lock(buffer_mutex_);
    return data_buffer_;
}

void MemoryDataStorage::clear() {
    std::lock_guard<std::mutex> lock(buffer_mutex_);
    data_buffer_.clear();
}

// DataLogger Implementation
DataLogger::DataLogger(const LoggerConfig& config)
    : config_(config), is_logging_(false), should_stop_(false) {
    
    // Initialize statistics
    stats_ = {0, 0, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0, 0};
}

DataLogger::~DataLogger() {
    stopLogging();
    flush();
}

bool DataLogger::initialize() {
    if (!storage_) {
        // Create default file storage
        storage_ = std::make_unique<FileDataStorage>(config_);
    }
    
    return storage_->initialize();
}

bool DataLogger::logReading(const SensorReading& reading) {
    if (!shouldLogReading(reading)) {
        stats_.filtered_readings++;
        return true; // Filtered out, but not an error
    }
    
    {
        std::lock_guard<std::mutex> lock(buffer_mutex_);
        
        if (buffer_.size() >= config_.buffer_size) {
            stats_.buffer_overflow_count++;
            return false; // Buffer overflow
        }
        
        buffer_.push(reading);
    }
    
    updateStats(reading);
    processBuffer();
    
    return true;
}

void DataLogger::startLogging(std::shared_ptr<ILightSensor> sensor) {
    if (is_logging_) {
        return;
    }
    
    sensor_ = sensor;
    is_logging_ = true;
    should_stop_ = false;
    last_log_time_ = std::chrono::steady_clock::now();
    
    // Set up sensor callback
    if (sensor_) {
        sensor_->startSampling([this](const SensorReading& reading) {
            if (!should_stop_) {
                logReading(reading);
            }
        });
    }
}

void DataLogger::stopLogging() {
    if (!is_logging_) {
        return;
    }
    
    should_stop_ = true;
    is_logging_ = false;
    
    if (sensor_) {
        sensor_->stopSampling();
    }
    
    flush();
}

bool DataLogger::flush() {
    std::lock_guard<std::mutex> lock(buffer_mutex_);
    
    if (!storage_) {
        return false;
    }
    
    bool success = true;
    while (!buffer_.empty()) {
        const SensorReading& reading = buffer_.front();
        if (!storage_->write(reading)) {
            success = false;
            break;
        }
        buffer_.pop();
    }
    
    if (success) {
        storage_->flush();
    }
    
    return success;
}

DataStats DataLogger::getStats() const {
    std::lock_guard<std::mutex> lock(buffer_mutex_);
    DataStats current_stats = stats_;
    current_stats.current_buffer_size = buffer_.size();
    return current_stats;
}

void DataLogger::configure(const LoggerConfig& config) {
    config_ = config;
    
    // Reinitialize storage if needed
    if (storage_) {
        storage_->close();
        storage_ = std::make_unique<FileDataStorage>(config_);
        storage_->initialize();
    }
}

void DataLogger::setStorage(std::unique_ptr<IDataStorage> storage) {
    if (is_logging_) {
        stopLogging();
    }
    
    storage_ = std::move(storage);
    if (storage_) {
        storage_->initialize();
    }
}

void DataLogger::process() {
    if (is_logging_ && sensor_) {
        // Check if it's time for next reading
        auto now = std::chrono::steady_clock::now();
        auto time_since_last = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - last_log_time_).count();
        
        if (time_since_last >= 1000) { // Log every second
            SensorReading reading = sensor_->read();
            logReading(reading);
            last_log_time_ = now;
        }
    }
    
    processBuffer();
}

bool DataLogger::isLogging() const {
    return is_logging_;
}

bool DataLogger::shouldLogReading(const SensorReading& reading) const {
    if (!reading.is_valid) {
        return false;
    }
    
    if (reading.lux_value < config_.min_lux_threshold || 
        reading.lux_value > config_.max_lux_threshold) {
        return false;
    }
    
    if (reading.quality < config_.min_quality_threshold) {
        return false;
    }
    
    return true;
}

void DataLogger::updateStats(const SensorReading& reading) {
    stats_.total_readings++;
    
    if (reading.is_valid) {
        stats_.valid_readings++;
        
        if (stats_.min_lux == 0.0f || reading.lux_value < stats_.min_lux) {
            stats_.min_lux = reading.lux_value;
        }
        
        if (reading.lux_value > stats_.max_lux) {
            stats_.max_lux = reading.lux_value;
        }
        
        // Update running average
        stats_.average_lux = (stats_.average_lux * (stats_.valid_readings - 1) + 
                             reading.lux_value) / stats_.valid_readings;
    }
}

void DataLogger::processBuffer() {
    std::lock_guard<std::mutex> lock(buffer_mutex_);
    
    if (buffer_.size() >= config_.flush_threshold) {
        if (storage_) {
            while (!buffer_.empty()) {
                const SensorReading& reading = buffer_.front();
                storage_->write(reading);
                buffer_.pop();
            }
            storage_->flush();
        }
    }
}

DataStats DataLogger::calculateStats(const std::vector<SensorReading>& readings) const {
    DataStats stats = {0, 0, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0, 0};
    
    if (readings.empty()) {
        return stats;
    }
    
    stats.total_readings = readings.size();
    
    float sum = 0.0f;
    float sum_squared = 0.0f;
    
    for (const auto& reading : readings) {
        if (reading.is_valid) {
            stats.valid_readings++;
            sum += reading.lux_value;
            sum_squared += reading.lux_value * reading.lux_value;
            
            if (stats.min_lux == 0.0f || reading.lux_value < stats.min_lux) {
                stats.min_lux = reading.lux_value;
            }
            
            if (reading.lux_value > stats.max_lux) {
                stats.max_lux = reading.lux_value;
            }
        }
    }
    
    if (stats.valid_readings > 0) {
        stats.average_lux = sum / stats.valid_readings;
        
        // Calculate standard deviation
        float variance = (sum_squared / stats.valid_readings) - (stats.average_lux * stats.average_lux);
        stats.std_deviation = std::sqrt(std::max(0.0f, variance));
    }
    
    return stats;
}

} // namespace LightSensor
