#!/bin/bash

# Build script for vibe-gi project
# Usage: ./build.sh [options]
# Options:
#   clean     - Clean build directory before building
#   debug     - Build in debug mode (default)
#   release   - Build in release mode
#   run       - Run the executable after building
#   help      - Show this help message

set -e  # Exit on any error

PROJECT_NAME="vibe-gi"
BUILD_DIR="build"
CMAKE_BUILD_TYPE="Debug"
CLEAN_BUILD=false
RUN_AFTER_BUILD=false

# Parse command line arguments
for arg in "$@"; do
    case $arg in
        clean)
            CLEAN_BUILD=true
            ;;
        debug)
            CMAKE_BUILD_TYPE="Debug"
            ;;
        release)
            CMAKE_BUILD_TYPE="Release"
            ;;
        run)
            RUN_AFTER_BUILD=true
            ;;
        help)
            echo "Build script for $PROJECT_NAME"
            echo "Usage: ./build.sh [options]"
            echo ""
            echo "Options:"
            echo "  clean     - Clean build directory before building"
            echo "  debug     - Build in debug mode (default)"
            echo "  release   - Build in release mode"
            echo "  run       - Run the executable after building"
            echo "  help      - Show this help message"
            echo ""
            echo "Examples:"
            echo "  ./build.sh               # Build in debug mode"
            echo "  ./build.sh release       # Build in release mode"
            echo "  ./build.sh clean debug   # Clean and build in debug mode"
            echo "  ./build.sh release run   # Build in release mode and run"
            exit 0
            ;;
        *)
            echo "Unknown option: $arg"
            echo "Use './build.sh help' for usage information"
            exit 1
            ;;
    esac
done

echo "Building $PROJECT_NAME in $CMAKE_BUILD_TYPE mode..."

# Clean build directory if requested
if [ "$CLEAN_BUILD" = true ]; then
    echo "Cleaning build directory..."
    rm -rf "$BUILD_DIR"
fi

# Create build directory if it doesn't exist
if [ ! -d "$BUILD_DIR" ]; then
    echo "Creating build directory..."
    mkdir -p "$BUILD_DIR"
fi

# Change to build directory
cd "$BUILD_DIR"

# Configure with CMake
echo "Configuring with CMake..."
cmake -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" ..

# Build the project
echo "Building project..."
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

echo "Build completed successfully!"

# Run the executable if requested
if [ "$RUN_AFTER_BUILD" = true ]; then
    echo "Running $PROJECT_NAME..."
    ./"$PROJECT_NAME"
fi 