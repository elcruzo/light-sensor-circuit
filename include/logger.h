#pragma once

#include <cstdint>
#include <FS.h>

namespace LightSensor {

/**
 * @brief Log levels
 */
enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARNING = 2,
    ERROR = 3,
    CRITICAL = 4
};

/**
 * @brief Log output destinations
 */
enum class LogOutput {
    SERIAL,     // Serial output
    FILE,       // SPIFFS file output
    BOTH,       // Both serial and file
    NONE        // No output
};

/**
 * @brief ESP32 Logger class using Serial and SPIFFS
 */
class Logger {
public:
    static Logger& getInstance();
    
    void setLevel(LogLevel level);
    void setOutput(LogOutput output);
    
    void log(LogLevel level, const char* message);
    void debug(const char* message);
    void info(const char* message);
    void warning(const char* message);
    void error(const char* message);
    void critical(const char* message);
    
    bool setLogFile(const char* filename);
    void closeLogFile();
    
private:
    Logger();
    ~Logger() { closeLogFile(); }
    
    // Prevent copying
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    LogLevel level_;
    LogOutput output_;
    File log_file_;
    bool file_is_open_;
    char log_file_path_[64];
    
    void formatMessage(LogLevel level, const char* message, char* buffer, size_t buffer_size) const;
    const char* levelToString(LogLevel level) const;
};

}  // namespace LightSensor
