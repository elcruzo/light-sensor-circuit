#include "logger.h"
#include "timer.h"
#include <cassert>
#include <iostream>
#include <thread>
#include <chrono>

using namespace LightSensor;

void testLogger() {
    std::cout << "Testing logger..." << std::endl;
    
    Logger& logger = Logger::getInstance();
    
    // Test level setting
    logger.setLevel(LogLevel::DEBUG);
    logger.setOutput(LogOutput::CONSOLE);
    
    // Test logging (should not crash)
    logger.debug("Debug message");
    logger.info("Info message");
    logger.warning("Warning message");
    logger.error("Error message");
    logger.critical("Critical message");
    
    // Test file logging
    assert(logger.setLogFile("/tmp/test.log") == true);
    logger.info("File log message");
    logger.closeLogFile();
    
    std::cout << "✓ Logger tests passed" << std::endl;
}

void testTimer() {
    std::cout << "Testing timer..." << std::endl;
    
    Timer timer;
    
    // Test initial state
    assert(timer.elapsedMs() >= 0);
    assert(timer.elapsedUs() >= 0);
    assert(timer.elapsedSeconds() >= 0.0f);
    assert(timer.hasElapsed(0) == true); // Should be true for 0 timeout
    
    // Test reset
    timer.reset();
    assert(timer.elapsedMs() < 100); // Should be very small after reset
    
    // Test timeout
    assert(timer.hasElapsed(1000) == false); // Should be false for 1 second timeout
    
    // Wait a bit and test again
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    assert(timer.elapsedMs() >= 100);
    assert(timer.hasElapsed(50) == true); // Should be true for 50ms timeout
    
    std::cout << "✓ Timer tests passed" << std::endl;
    std::cout << "  Elapsed ms: " << timer.elapsedMs() << std::endl;
    std::cout << "  Elapsed us: " << timer.elapsedUs() << std::endl;
    std::cout << "  Elapsed seconds: " << timer.elapsedSeconds() << std::endl;
}

void testTimerPrecision() {
    std::cout << "Testing timer precision..." << std::endl;
    
    Timer timer;
    
    // Test microsecond precision
    uint32_t start_us = timer.elapsedUs();
    std::this_thread::sleep_for(std::chrono::microseconds(1000)); // 1ms
    uint32_t end_us = timer.elapsedUs();
    
    uint32_t elapsed_us = end_us - start_us;
    assert(elapsed_us >= 1000); // Should be at least 1000 microseconds
    assert(elapsed_us < 2000); // Should be less than 2000 microseconds (with some tolerance)
    
    std::cout << "✓ Timer precision tests passed" << std::endl;
    std::cout << "  Expected ~1000us, got " << elapsed_us << "us" << std::endl;
}

void testTimerMultipleInstances() {
    std::cout << "Testing multiple timer instances..." << std::endl;
    
    Timer timer1;
    Timer timer2;
    
    // Start both timers
    timer1.reset();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    timer2.reset();
    
    // Wait a bit more
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    // Check that timers are independent
    uint32_t elapsed1 = timer1.elapsedMs();
    uint32_t elapsed2 = timer2.elapsedMs();
    
    assert(elapsed1 > elapsed2); // timer1 should have more elapsed time
    assert(elapsed1 >= 100); // timer1 should have at least 100ms
    assert(elapsed2 >= 50); // timer2 should have at least 50ms
    
    std::cout << "✓ Multiple timer instances passed" << std::endl;
    std::cout << "  Timer1 elapsed: " << elapsed1 << "ms" << std::endl;
    std::cout << "  Timer2 elapsed: " << elapsed2 << "ms" << std::endl;
}

int runUtilsTests() {
    try {
        testLogger();
        testTimer();
        testTimerPrecision();
        testTimerMultipleInstances();
        
        return 0;
    } catch (const std::exception& e) {
        std::cout << "\n✗ Test failed: " << e.what() << std::endl;
        return 1;
    }
}
