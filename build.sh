#!/bin/bash

# Light Sensor Circuit - Build Script
# This script builds the project for different platforms

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Default values
BUILD_TYPE="Release"
PLATFORM="desktop"
CLEAN=false
TESTS=false
EXAMPLES=true
INSTALL=false

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to show usage
show_usage() {
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  -t, --type TYPE        Build type (Debug|Release) [default: Release]"
    echo "  -p, --platform PLAT    Platform (desktop|embedded|arduino) [default: desktop]"
    echo "  -c, --clean            Clean build directory before building"
    echo "  --tests                Build and run tests"
    echo "  --no-examples          Skip building examples"
    echo "  --install              Install after building"
    echo "  -h, --help             Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0                     # Build for desktop (Release)"
    echo "  $0 -t Debug -p arduino # Build for Arduino (Debug)"
    echo "  $0 -c --tests          # Clean build and run tests"
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -t|--type)
            BUILD_TYPE="$2"
            shift 2
            ;;
        -p|--platform)
            PLATFORM="$2"
            shift 2
            ;;
        -c|--clean)
            CLEAN=true
            shift
            ;;
        --tests)
            TESTS=true
            shift
            ;;
        --no-examples)
            EXAMPLES=false
            shift
            ;;
        --install)
            INSTALL=true
            shift
            ;;
        -h|--help)
            show_usage
            exit 0
            ;;
        *)
            print_error "Unknown option: $1"
            show_usage
            exit 1
            ;;
    esac
done

# Validate build type
if [[ "$BUILD_TYPE" != "Debug" && "$BUILD_TYPE" != "Release" ]]; then
    print_error "Invalid build type: $BUILD_TYPE"
    print_error "Must be 'Debug' or 'Release'"
    exit 1
fi

# Validate platform
if [[ "$PLATFORM" != "desktop" && "$PLATFORM" != "embedded" && "$PLATFORM" != "arduino" ]]; then
    print_error "Invalid platform: $PLATFORM"
    print_error "Must be 'desktop', 'embedded', or 'arduino'"
    exit 1
fi

print_status "Building Light Sensor Circuit"
print_status "Build type: $BUILD_TYPE"
print_status "Platform: $PLATFORM"

# Create build directory
BUILD_DIR="build_${PLATFORM}_$(echo ${BUILD_TYPE} | tr '[:upper:]' '[:lower:]')"
print_status "Build directory: $BUILD_DIR"

# Clean build directory if requested
if [[ "$CLEAN" == true ]]; then
    print_status "Cleaning build directory..."
    rm -rf "$BUILD_DIR"
fi

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure CMake
print_status "Configuring CMake..."

CMAKE_ARGS="-DCMAKE_BUILD_TYPE=$BUILD_TYPE"

case "$PLATFORM" in
    "desktop")
        CMAKE_ARGS="$CMAKE_ARGS -DDESKTOP_BUILD=ON"
        ;;
    "embedded")
        CMAKE_ARGS="$CMAKE_ARGS -DEMBEDDED_BUILD=ON"
        ;;
    "arduino")
        CMAKE_ARGS="$CMAKE_ARGS -DARDUINO_BUILD=ON"
        ;;
esac

if [[ "$EXAMPLES" == false ]]; then
    CMAKE_ARGS="$CMAKE_ARGS -DBUILD_EXAMPLES=OFF"
fi

if [[ "$TESTS" == true ]]; then
    CMAKE_ARGS="$CMAKE_ARGS -DBUILD_TESTS=ON"
fi

cmake .. $CMAKE_ARGS

# Build
print_status "Building..."
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

print_success "Build completed successfully!"

# Run tests if requested
if [[ "$TESTS" == true ]]; then
    print_status "Running tests..."
    if [[ -f "tests/light_sensor_tests" ]]; then
        ./tests/light_sensor_tests
        print_success "Tests completed successfully!"
    elif [[ -f "tests/simple_tests" ]]; then
        ./tests/simple_tests
        print_success "Tests completed successfully!"
    else
        print_warning "No test executable found"
    fi
fi

# Install if requested
if [[ "$INSTALL" == true ]]; then
    print_status "Installing..."
    make install
    print_success "Installation completed successfully!"
fi

# Show build results
print_status "Build results:"
echo "  Build directory: $(pwd)"
echo "  Libraries: $(find . -name "*.a" -o -name "*.so" | wc -l) found"
echo "  Executables: $(find . -name "*example*" -o -name "*test*" | wc -l) found"

if [[ "$EXAMPLES" == true ]]; then
    print_status "Example programs built:"
    find . -name "*example*" -type f -executable | while read -r file; do
        echo "  - $(basename "$file")"
    done
fi

print_success "Build script completed!"
