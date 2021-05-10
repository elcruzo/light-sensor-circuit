# Tests CMakeLists.txt

# Find testing framework
find_package(GTest QUIET)
if(NOT GTest_FOUND)
    message(STATUS "GTest not found, using simple test framework")
    set(USE_GTEST FALSE)
else()
    set(USE_GTEST TRUE)
    enable_testing()
endif()

# Test source files
set(TEST_SOURCES
    test_runner.cpp
    test_light_sensor.cpp
    test_power_manager.cpp
    test_data_logger.cpp
    test_signal_processor.cpp
    test_config_manager.cpp
    test_utils.cpp
)

if(USE_GTEST)
    # Create test executable with GTest
    add_executable(light_sensor_tests ${TEST_SOURCES})
    target_link_libraries(light_sensor_tests 
        light_sensor_core 
        GTest::GTest 
        GTest::Main
    )
    
    # Add tests
    add_test(NAME LightSensorTests COMMAND light_sensor_tests)
    
    # Set test properties
    set_target_properties(light_sensor_tests PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
    )
    
else()
    # Create simple test executable
    add_executable(simple_tests ${TEST_SOURCES})
    target_link_libraries(simple_tests light_sensor_core)
    
    # Set test properties
    set_target_properties(simple_tests PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
    )
    
    # Add custom test command
    add_test(NAME SimpleTests COMMAND simple_tests)
endif()

# Memory check tests (if available)
if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT ARDUINO_BUILD)
    find_program(VALGRIND_EXECUTABLE valgrind)
    if(VALGRIND_EXECUTABLE)
        add_test(NAME MemoryCheck 
            COMMAND ${VALGRIND_EXECUTABLE} --leak-check=full --error-exitcode=1 
            $<TARGET_FILE:light_sensor_tests>)
        set_tests_properties(MemoryCheck PROPERTIES
            LABELS "memory"
            TIMEOUT 300
        )
    endif()
endif()

# Coverage tests (if available)
if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT ARDUINO_BUILD)
    find_program(GCOV_EXECUTABLE gcov)
    find_program(LCOV_EXECUTABLE lcov)
    find_program(GENHTML_EXECUTABLE genhtml)
    
    if(GCOV_EXECUTABLE AND LCOV_EXECUTABLE AND GENHTML_EXECUTABLE)
        # Add coverage flags
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage -fprofile-arcs -ftest-coverage")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
        
        # Add coverage test
        add_test(NAME CoverageTest
            COMMAND ${CMAKE_COMMAND} -E chdir ${CMAKE_BINARY_DIR}
            ${LCOV_EXECUTABLE} --capture --directory . --output-file coverage.info
        )
        
        add_test(NAME CoverageReport
            COMMAND ${CMAKE_COMMAND} -E chdir ${CMAKE_BINARY_DIR}
            ${GENHTML_EXECUTABLE} coverage.info --output-directory coverage_html
        )
    endif()
endif()
