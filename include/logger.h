#pragma once

#include <string>
#include <fstream>

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
    SERIAL,     // Serial output (Arduino)
    CONSOLE,    // Console output
    FILE,       // File output
    NONE        // No output
};

/**
 * @brief Simple logger class
 */
class Logger {
public:
    static Logger& getInstance();
    
    void setLevel(LogLevel level);
    void setOutput(LogOutput output);
    
    void log(LogLevel level, const std::string& message);
    void debug(const std::string& message);
    void info(const std::string& message);
    void warning(const std::string& message);
    void error(const std::string& message);
    void critical(const std::string& message);
    
    bool setLogFile(const std::string& filename);
    void closeLogFile();
    
private:
    Logger() : level_(LogLevel::INFO), output_(LogOutput::CONSOLE) {}
    ~Logger() { closeLogFile(); }
    
    LogLevel level_;
    LogOutput output_;
    std::ofstream file_stream_;
    
    std::string formatMessage(LogLevel level, const std::string& message) const;
    std::string getTimestamp() const;
    std::string levelToString(LogLevel level) const;
};

} // namespace LightSensor
