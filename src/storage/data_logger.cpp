#include "data_logger.h"
#include <Arduino.h>
#include <SPIFFS.h>
#include <cmath>
#include <cstring>
#include <cstdio>

namespace LightSensor {

// SPIFFSDataStorage Implementation
SPIFFSDataStorage::SPIFFSDataStorage(const LoggerConfig& config)
    : config_(config), current_file_size_(0), is_initialized_(false) {
    memset(current_file_path_, 0, sizeof(current_file_path_));
}

SPIFFSDataStorage::~SPIFFSDataStorage() {
    close();
}

bool SPIFFSDataStorage::initialize() {
    if (is_initialized_) {
        return true;
    }
    
    // Initialize SPIFFS if not already done
    if (!SPIFFS.begin(true)) {
        return false;
    }
    
    return createNewLogFile();
}

bool SPIFFSDataStorage::write(const SensorReading& data) {
    if (!is_initialized_ || !log_file_) {
        return false;
    }
    
    // Check if rotation is needed
    if (needsRotation()) {
        if (!rotateLogFile()) {
            return false;
        }
    }
    
    char formatted_data[128];
    formatReading(data, formatted_data, sizeof(formatted_data));
    
    size_t bytes_written = log_file_.println(formatted_data);
    if (bytes_written == 0) {
        return false;
    }
    
    current_file_size_ += bytes_written;
    return true;
}

bool SPIFFSDataStorage::flush() {
    if (log_file_) {
        log_file_.flush();
        return true;
    }
    return true;
}

void SPIFFSDataStorage::close() {
    if (log_file_) {
        log_file_.close();
    }
    is_initialized_ = false;
}

size_t SPIFFSDataStorage::getAvailableSpace() const {
    return SPIFFS.totalBytes() - SPIFFS.usedBytes();
}

bool SPIFFSDataStorage::createNewLogFile() {
    // Generate timestamp-based filename
    uint32_t timestamp = millis();
    snprintf(current_file_path_, sizeof(current_file_path_), 
             "%s/sensor_%lu.log", config_.log_file_path, timestamp);
    
    // Ensure directory exists (SPIFFS doesn't have directories, so just use the path)
    log_file_ = SPIFFS.open(current_file_path_, FILE_WRITE);
    
    if (!log_file_) {
        return false;
    }
    
    // Write header
    log_file_.println("# Light Sensor Data Log");
    log_file_.println("# Format: timestamp_ms,raw_value,lux_value,voltage,quality");
    
    is_initialized_ = true;
    current_file_size_ = 0;
    return true;
}

bool SPIFFSDataStorage::needsRotation() const {
    return config_.enable_rotation && 
           current_file_size_ > config_.max_file_size_bytes;
}

bool SPIFFSDataStorage::rotateLogFile() {
    close();
    return createNewLogFile();
}

void SPIFFSDataStorage::formatReading(const SensorReading& reading, char* buffer, size_t buffer_size) const {
    if (config_.enable_timestamp) {
        snprintf(buffer, buffer_size, "%lu,%.6f,%.6f,%.6f,%u",
                 reading.timestamp_ms,
                 reading.raw_value,
                 reading.lux_value,
                 reading.voltage,
                 reading.quality);
    } else {
        snprintf(buffer, buffer_size, "%.6f,%.6f,%.6f,%u",
                 reading.raw_value,
                 reading.lux_value,
                 reading.voltage,
                 reading.quality);
    }
}

// MemoryDataStorage Implementation
MemoryDataStorage::MemoryDataStorage(const LoggerConfig& config)
    : config_(config), buffer_head_(0), buffer_count_(0), is_initialized_(false) {
}

bool MemoryDataStorage::initialize() {
    is_initialized_ = true;
    return true;
}

bool MemoryDataStorage::write(const SensorReading& data) {
    if (!is_initialized_) {
        return false;
    }
    
    size_t actual_buffer_size = config_.buffer_size < MAX_BUFFER_SIZE ? 
                                 config_.buffer_size : MAX_BUFFER_SIZE;
    
    data_buffer_[buffer_head_] = data;
    buffer_head_ = (buffer_head_ + 1) % actual_buffer_size;
    
    if (buffer_count_ < actual_buffer_size) {
        buffer_count_++;
    }
    
    return true;
}

bool MemoryDataStorage::flush() {
    return true;  // Memory storage doesn't need flushing
}

void MemoryDataStorage::close() {
    clear();
    is_initialized_ = false;
}

size_t MemoryDataStorage::getAvailableSpace() const {
    size_t actual_buffer_size = config_.buffer_size < MAX_BUFFER_SIZE ? 
                                 config_.buffer_size : MAX_BUFFER_SIZE;
    return actual_buffer_size - buffer_count_;
}

size_t MemoryDataStorage::getDataCount() const {
    return buffer_count_;
}

bool MemoryDataStorage::getData(size_t index, SensorReading& reading) const {
    if (index >= buffer_count_) {
        return false;
    }
    
    size_t actual_buffer_size = config_.buffer_size < MAX_BUFFER_SIZE ? 
                                 config_.buffer_size : MAX_BUFFER_SIZE;
    size_t actual_index = (buffer_head_ - buffer_count_ + index + actual_buffer_size) % actual_buffer_size;
    reading = data_buffer_[actual_index];
    return true;
}

void MemoryDataStorage::clear() {
    buffer_head_ = 0;
    buffer_count_ = 0;
}

// DataLogger Implementation
DataLogger::DataLogger(const LoggerConfig& config)
    : config_(config), storage_(nullptr), owns_storage_(false),
      buffer_head_(0), buffer_tail_(0), buffer_count_(0),
      is_logging_(false), should_stop_(false),
      sensor_(nullptr), last_log_time_ms_(0) {
    
    // Initialize statistics
    stats_ = {0, 0, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0, 0};
}

DataLogger::~DataLogger() {
    stopLogging();
    flush();
    
    if (owns_storage_ && storage_) {
        delete storage_;
    }
}

bool DataLogger::initialize() {
    if (!storage_) {
        // Create default SPIFFS storage
        storage_ = new SPIFFSDataStorage(config_);
        owns_storage_ = true;
    }
    
    return storage_->initialize();
}

bool DataLogger::logReading(const SensorReading& reading) {
    if (!shouldLogReading(reading)) {
        stats_.filtered_readings++;
        return true;  // Filtered out, but not an error
    }
    
    if (!enqueue(reading)) {
        stats_.buffer_overflow_count++;
        return false;  // Buffer overflow
    }
    
    updateStats(reading);
    processBuffer();
    
    return true;
}

void DataLogger::startLogging(ILightSensor* sensor) {
    if (is_logging_ || !sensor) {
        return;
    }
    
    sensor_ = sensor;
    is_logging_ = true;
    should_stop_ = false;
    last_log_time_ms_ = millis();
}

void DataLogger::stopLogging() {
    if (!is_logging_) {
        return;
    }
    
    should_stop_ = true;
    is_logging_ = false;
    sensor_ = nullptr;
    
    flush();
}

bool DataLogger::flush() {
    if (!storage_) {
        return false;
    }
    
    SensorReading reading;
    while (dequeue(reading)) {
        if (!storage_->write(reading)) {
            return false;
        }
    }
    
    storage_->flush();
    return true;
}

DataStats DataLogger::getStats() const {
    DataStats current_stats = stats_;
    current_stats.current_buffer_size = buffer_count_;
    return current_stats;
}

void DataLogger::configure(const LoggerConfig& config) {
    config_ = config;
    
    // Reinitialize storage if needed
    if (storage_ && owns_storage_) {
        storage_->close();
        delete storage_;
        storage_ = new SPIFFSDataStorage(config_);
        storage_->initialize();
    }
}

void DataLogger::setStorage(IDataStorage* storage) {
    if (is_logging_) {
        stopLogging();
    }
    
    if (owns_storage_ && storage_) {
        delete storage_;
    }
    
    storage_ = storage;
    owns_storage_ = false;
    
    if (storage_) {
        storage_->initialize();
    }
}

void DataLogger::process() {
    if (is_logging_ && sensor_) {
        uint32_t now = millis();
        
        // Check if it's time for next reading
        if (now - last_log_time_ms_ >= 1000) {
            SensorReading reading = sensor_->read();
            logReading(reading);
            last_log_time_ms_ = now;
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
    if (buffer_count_ >= config_.flush_threshold) {
        flush();
    }
}

bool DataLogger::enqueue(const SensorReading& reading) {
    if (buffer_count_ >= MAX_QUEUE_SIZE) {
        return false;
    }
    
    buffer_[buffer_tail_] = reading;
    buffer_tail_ = (buffer_tail_ + 1) % MAX_QUEUE_SIZE;
    buffer_count_++;
    
    return true;
}

bool DataLogger::dequeue(SensorReading& reading) {
    if (buffer_count_ == 0) {
        return false;
    }
    
    reading = buffer_[buffer_head_];
    buffer_head_ = (buffer_head_ + 1) % MAX_QUEUE_SIZE;
    buffer_count_--;
    
    return true;
}

}  // namespace LightSensor
