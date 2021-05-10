#include "test_light_sensor.h"
#include "test_power_manager.h"
#include "test_data_logger.h"
#include "test_signal_processor.h"
#include "test_config_manager.h"
#include "test_utils.h"
#include <iostream>

int main() {
    std::cout << "Running all light sensor circuit tests..." << std::endl;
    std::cout << "=========================================" << std::endl;
    
    int total_tests = 0;
    int passed_tests = 0;
    
    // Run light sensor tests
    std::cout << "\n1. Light Sensor Tests" << std::endl;
    std::cout << "--------------------" << std::endl;
    int result = runLightSensorTests();
    total_tests++;
    if (result == 0) {
        passed_tests++;
        std::cout << "✓ Light sensor tests PASSED" << std::endl;
    } else {
        std::cout << "✗ Light sensor tests FAILED" << std::endl;
    }
    
    // Run power manager tests
    std::cout << "\n2. Power Manager Tests" << std::endl;
    std::cout << "----------------------" << std::endl;
    result = runPowerManagerTests();
    total_tests++;
    if (result == 0) {
        passed_tests++;
        std::cout << "✓ Power manager tests PASSED" << std::endl;
    } else {
        std::cout << "✗ Power manager tests FAILED" << std::endl;
    }
    
    // Run data logger tests
    std::cout << "\n3. Data Logger Tests" << std::endl;
    std::cout << "--------------------" << std::endl;
    result = runDataLoggerTests();
    total_tests++;
    if (result == 0) {
        passed_tests++;
        std::cout << "✓ Data logger tests PASSED" << std::endl;
    } else {
        std::cout << "✗ Data logger tests FAILED" << std::endl;
    }
    
    // Run signal processor tests
    std::cout << "\n4. Signal Processor Tests" << std::endl;
    std::cout << "-------------------------" << std::endl;
    result = runSignalProcessorTests();
    total_tests++;
    if (result == 0) {
        passed_tests++;
        std::cout << "✓ Signal processor tests PASSED" << std::endl;
    } else {
        std::cout << "✗ Signal processor tests FAILED" << std::endl;
    }
    
    // Run config manager tests
    std::cout << "\n5. Config Manager Tests" << std::endl;
    std::cout << "-----------------------" << std::endl;
    result = runConfigManagerTests();
    total_tests++;
    if (result == 0) {
        passed_tests++;
        std::cout << "✓ Config manager tests PASSED" << std::endl;
    } else {
        std::cout << "✗ Config manager tests FAILED" << std::endl;
    }
    
    // Run utility tests
    std::cout << "\n6. Utility Tests" << std::endl;
    std::cout << "----------------" << std::endl;
    result = runUtilsTests();
    total_tests++;
    if (result == 0) {
        passed_tests++;
        std::cout << "✓ Utility tests PASSED" << std::endl;
    } else {
        std::cout << "✗ Utility tests FAILED" << std::endl;
    }
    
    // Summary
    std::cout << "\n=========================================" << std::endl;
    std::cout << "Test Summary: " << passed_tests << "/" << total_tests << " test suites passed" << std::endl;
    
    if (passed_tests == total_tests) {
        std::cout << "🎉 ALL TESTS PASSED! 🎉" << std::endl;
        return 0;
    } else {
        std::cout << "❌ Some tests failed" << std::endl;
        return 1;
    }
}
