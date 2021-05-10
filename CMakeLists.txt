cmake_minimum_required(VERSION 3.16)
project(LightSensorCircuit VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler-specific options
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
endif()

# Platform detection
if(CMAKE_SYSTEM_NAME STREQUAL "Arduino")
    set(ARDUINO_BUILD TRUE)
    message(STATUS "Building for Arduino platform")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Generic")
    set(EMBEDDED_BUILD TRUE)
    message(STATUS "Building for embedded platform")
else()
    set(DESKTOP_BUILD TRUE)
    message(STATUS "Building for desktop platform")
endif()

# Include directories
include_directories(include)

# Source files
set(CORE_SOURCES
    src/core/light_sensor.cpp
)

set(POWER_SOURCES
    src/power/power_manager.cpp
)

set(STORAGE_SOURCES
    src/storage/data_logger.cpp
)

set(SIGNAL_SOURCES
    src/signal/signal_processor.cpp
)

set(CONFIG_SOURCES
    src/config/config_manager.cpp
)

set(UTILS_SOURCES
    src/utils/logger.cpp
    src/utils/timer.cpp
)

# Create library
add_library(light_sensor_core STATIC
    ${CORE_SOURCES}
    ${POWER_SOURCES}
    ${STORAGE_SOURCES}
    ${SIGNAL_SOURCES}
    ${CONFIG_SOURCES}
    ${UTILS_SOURCES}
)

# Platform-specific configurations
if(ARDUINO_BUILD)
    # Arduino-specific settings
    target_compile_definitions(light_sensor_core PRIVATE ARDUINO=1)
    target_compile_definitions(light_sensor_core PRIVATE F_CPU=16000000UL)
    
    # Arduino libraries (if using PlatformIO or similar)
    find_library(ARDUINO_CORE ArduinoCore)
    if(ARDUINO_CORE)
        target_link_libraries(light_sensor_core ${ARDUINO_CORE})
    endif()
    
elseif(EMBEDDED_BUILD)
    # Embedded platform settings
    target_compile_definitions(light_sensor_core PRIVATE EMBEDDED=1)
    
    # Add embedded-specific libraries
    target_link_libraries(light_sensor_core m)
    
else()
    # Desktop build settings
    target_compile_definitions(light_sensor_core PRIVATE DESKTOP=1)
    
    # Add threading library for desktop
    find_package(Threads REQUIRED)
    target_link_libraries(light_sensor_core Threads::Threads)
    
    # Add math library
    target_link_libraries(light_sensor_core m)
endif()

# Set output directories
set_target_properties(light_sensor_core PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Create examples
add_subdirectory(examples)

# Create tests
enable_testing()
add_subdirectory(tests)

# Installation
install(TARGETS light_sensor_core
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/ DESTINATION include/light_sensor)

# Package configuration
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    LightSensorCircuitConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/LightSensorCircuitConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/LightSensorCircuitConfig.cmake
    INSTALL_DESTINATION lib/cmake/LightSensorCircuit
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/LightSensorCircuitConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/LightSensorCircuitConfigVersion.cmake
    DESTINATION lib/cmake/LightSensorCircuit
)

# Print build information
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")

# Optional: Generate compile_commands.json for IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
