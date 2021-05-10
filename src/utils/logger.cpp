#include "logger.h"
#include <iostream>
#include <iomanip>
#include <sstream>

#ifdef ARDUINO
#include <Arduino.h>
#else
#include <chrono>
#endif

namespace LightSensor {

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

void Logger::setLevel(LogLevel level) {
    level_ = level;
}

void Logger::setOutput(LogOutput output) {
    output_ = output;
}

void Logger::log(LogLevel level, const std::string& message) {
    if (level < level_) {
        return;
    }
    
    std::string formatted_message = formatMessage(level, message);
    
    switch (output_) {
        case LogOutput::SERIAL:
            #ifdef ARDUINO
            Serial.println(formatted_message.c_str());
            #else
            std::cout << formatted_message << std::endl;
            #endif
            break;
            
        case LogOutput::CONSOLE:
            std::cout << formatted_message << std::endl;
            break;
            
        case LogOutput::FILE:
            if (file_stream_.is_open()) {
                file_stream_ << formatted_message << std::endl;
                file_stream_.flush();
            }
            break;
            
        case LogOutput::NONE:
            // No output
            break;
    }
}

void Logger::debug(const std::string& message) {
    log(LogLevel::DEBUG, message);
}

void Logger::info(const std::string& message) {
    log(LogLevel::INFO, message);
}

void Logger::warning(const std::string& message) {
    log(LogLevel::WARNING, message);
}

void Logger::error(const std::string& message) {
    log(LogLevel::ERROR, message);
}

void Logger::critical(const std::string& message) {
    log(LogLevel::CRITICAL, message);
}

bool Logger::setLogFile(const std::string& filename) {
    if (file_stream_.is_open()) {
        file_stream_.close();
    }
    
    file_stream_.open(filename, std::ios::out | std::ios::app);
    return file_stream_.is_open();
}

void Logger::closeLogFile() {
    if (file_stream_.is_open()) {
        file_stream_.close();
    }
}

std::string Logger::formatMessage(LogLevel level, const std::string& message) const {
    std::stringstream ss;
    
    // Add timestamp
    ss << "[" << getTimestamp() << "] ";
    
    // Add level
    ss << "[" << levelToString(level) << "] ";
    
    // Add message
    ss << message;
    
    return ss.str();
}

std::string Logger::getTimestamp() const {
    #ifdef ARDUINO
    return std::to_string(millis());
    #else
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    ss << "." << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
    #endif
}

std::string Logger::levelToString(LogLevel level) const {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARNING: return "WARN";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::CRITICAL: return "CRIT";
        default: return "UNKNOWN";
    }
}

} // namespace LightSensor
