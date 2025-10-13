#include "logger.h"
#include <Arduino.h>
#include <SPIFFS.h>

namespace LightSensor {

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

Logger::Logger() 
    : level_(LogLevel::INFO), output_(LogOutput::SERIAL), 
      file_is_open_(false) {
}

void Logger::setLevel(LogLevel level) {
    level_ = level;
}

void Logger::setOutput(LogOutput output) {
    output_ = output;
}

void Logger::log(LogLevel level, const char* message) {
    if (level < level_) {
        return;
    }
    
    char formatted_message[256];
    formatMessage(level, message, formatted_message, sizeof(formatted_message));
    
    switch (output_) {
        case LogOutput::SERIAL:
            Serial.println(formatted_message);
            break;
            
        case LogOutput::FILE:
            if (file_is_open_) {
                log_file_.println(formatted_message);
                log_file_.flush();
            }
            break;
            
        case LogOutput::BOTH:
            Serial.println(formatted_message);
            if (file_is_open_) {
                log_file_.println(formatted_message);
                log_file_.flush();
            }
            break;
            
        case LogOutput::NONE:
            break;
    }
}

void Logger::debug(const char* message) {
    log(LogLevel::DEBUG, message);
}

void Logger::info(const char* message) {
    log(LogLevel::INFO, message);
}

void Logger::warning(const char* message) {
    log(LogLevel::WARNING, message);
}

void Logger::error(const char* message) {
    log(LogLevel::ERROR, message);
}

void Logger::critical(const char* message) {
    log(LogLevel::CRITICAL, message);
}

bool Logger::setLogFile(const char* filename) {
    if (file_is_open_) {
        log_file_.close();
        file_is_open_ = false;
    }
    
    // Initialize SPIFFS if not already done
    if (!SPIFFS.begin(true)) {
        return false;
    }
    
    log_file_ = SPIFFS.open(filename, FILE_APPEND);
    if (!log_file_) {
        return false;
    }
    
    file_is_open_ = true;
    strncpy(log_file_path_, filename, sizeof(log_file_path_) - 1);
    log_file_path_[sizeof(log_file_path_) - 1] = '\0';
    
    return true;
}

void Logger::closeLogFile() {
    if (file_is_open_) {
        log_file_.close();
        file_is_open_ = false;
    }
}

void Logger::formatMessage(LogLevel level, const char* message, char* buffer, size_t buffer_size) const {
    uint32_t timestamp = millis();
    const char* level_str = levelToString(level);
    
    snprintf(buffer, buffer_size, "[%lu] [%s] %s", timestamp, level_str, message);
}

const char* Logger::levelToString(LogLevel level) const {
    switch (level) {
        case LogLevel::DEBUG:    return "DEBUG";
        case LogLevel::INFO:     return "INFO";
        case LogLevel::WARNING:  return "WARN";
        case LogLevel::ERROR:    return "ERROR";
        case LogLevel::CRITICAL: return "CRIT";
        default:                 return "UNKNOWN";
    }
}

}  // namespace LightSensor
