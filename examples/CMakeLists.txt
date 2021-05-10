# Examples CMakeLists.txt

# Basic sensor example
add_executable(basic_sensor_example basic_sensor_example.cpp)
target_link_libraries(basic_sensor_example light_sensor_core)

# Data logging example
add_executable(data_logging_example data_logging_example.cpp)
target_link_libraries(data_logging_example light_sensor_core)

# Power management example
add_executable(power_management_example power_management_example.cpp)
target_link_libraries(power_management_example light_sensor_core)

# Signal processing example
add_executable(signal_processing_example signal_processing_example.cpp)
target_link_libraries(signal_processing_example light_sensor_core)

# Complete system example
add_executable(complete_system_example complete_system_example.cpp)
target_link_libraries(complete_system_example light_sensor_core)

# Arduino example (if building for Arduino)
if(ARDUINO_BUILD)
    add_executable(arduino_example arduino_example.cpp)
    target_link_libraries(arduino_example light_sensor_core)
endif()

# Set output directories for examples
set_target_properties(
    basic_sensor_example
    data_logging_example
    power_management_example
    signal_processing_example
    complete_system_example
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
)

if(ARDUINO_BUILD)
    set_target_properties(arduino_example PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
    )
endif()
