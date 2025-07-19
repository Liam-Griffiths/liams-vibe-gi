#!/bin/bash

# Clean script for vibe-gi project
# Removes all build artifacts

echo "Cleaning vibe-gi project..."

# Remove build directory
if [ -d "build" ]; then
    echo "Removing build directory..."
    rm -rf build
    echo "Build directory removed."
else
    echo "Build directory does not exist."
fi

# Remove any leftover object files
find . -name "*.o" -type f -delete 2>/dev/null || true
find . -name "*.a" -type f -delete 2>/dev/null || true
find . -name "*.so" -type f -delete 2>/dev/null || true
find . -name "*.dylib" -type f -delete 2>/dev/null || true

# Remove CMake cache files (if any exist outside build directory)
find . -name "CMakeCache.txt" -not -path "./build/*" -delete 2>/dev/null || true
find . -name "CMakeFiles" -type d -not -path "./build/*" -exec rm -rf {} + 2>/dev/null || true

# Remove .DS_Store files (macOS)
find . -name ".DS_Store" -delete 2>/dev/null || true

echo "Project cleaned successfully!" 