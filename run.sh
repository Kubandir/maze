#!/bin/bash

set -e

PROJECT_NAME="maze"
BUILD_DIR="build"
BIN_DIR="bin"
SRC_DIR="src"
BACKUP_DIR="backup"

# ANSI color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
RESET='\033[0m'

# Function definitions
log_info() {
    echo -e "${GREEN}[INFO]${RESET} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${RESET} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${RESET} $1" >&2
}

# Show help
show_help() {
    echo -e "${BLUE}Usage:${RESET} ./run.sh [dev|release|backup]"
    echo
    echo -e "${YELLOW}Commands:${RESET}"
    echo "  dev     - Build with minimal optimizations for development and run"
    echo "  release - Build with full optimizations and security features and run"
    echo "  backup  - Create a backup of the project"
    echo
}

# Build the project
build_project() {
    local build_type=$1
    local cmake_build_type=""

    if [ "$build_type" == "dev" ]; then
        cmake_build_type="Debug"
        log_info "Building project in development mode"
    elif [ "$build_type" == "release" ]; then
        cmake_build_type="Release"
        log_info "Building project in release mode with optimizations"
    else
        log_error "Unknown build type: $build_type"
        exit 1
    fi

    # Create build directory if it doesn't exist
    mkdir -p $BUILD_DIR

    # Remove existing binary if it exists
    if [ -f "$BIN_DIR/$PROJECT_NAME" ]; then
        log_info "Removing existing binary..."
        rm "$BIN_DIR/$PROJECT_NAME"
    fi

    # Configure with CMake
    log_info "Configuring with CMake..."
    cd $BUILD_DIR
    cmake -DCMAKE_BUILD_TYPE=$cmake_build_type ..

    # Build
    log_info "Building project..."
    cmake --build . -- -j$(nproc)

    # Link to bin directory
    cd ..
    mkdir -p $BIN_DIR

    if [ -f "$BUILD_DIR/bin/$PROJECT_NAME" ]; then
        cp "$BUILD_DIR/bin/$PROJECT_NAME" "$BIN_DIR/"
        log_info "Binary created at $BIN_DIR/$PROJECT_NAME"

        # Additional steps for release builds
        if [ "$build_type" == "release" ]; then
            log_info "Applying additional release optimizations..."
            strip --strip-all "$BIN_DIR/$PROJECT_NAME"

            # Compress binary to reduce size (if upx is installed)
            if command -v upx &> /dev/null; then
                log_info "Compressing binary with UPX..."
                upx --best --lzma "$BIN_DIR/$PROJECT_NAME"
            else
                log_warn "UPX not installed. Binary compression skipped."
                log_warn "Install UPX for better compression: sudo apt install upx"
            fi
        fi

        # Run the program
        log_info "Running the application..."
        "$BIN_DIR/$PROJECT_NAME"
    else
        log_error "Build failed! Binary not found."
        exit 1
    fi
}

# Backup the project to git
backup_project() {
    # Check if commit message was provided
    if [ "$#" -eq 0 ]; then
        log_error "Missing commit message. Usage: ./run.sh backup \"Your commit message\""
        exit 1
    fi

    local commit_message="$1"
    log_info "Backing up to git repository..."

    # Check if git is initialized
    if [ ! -d ".git" ]; then
        log_error "Git repository not initialized. Run 'git init' first."
        exit 1
    fi

    # Add all files
    log_info "Adding files to git..."
    git add .

    # Commit with provided message
    log_info "Committing changes: \"$commit_message\""
    git commit -m "$commit_message"

    # Set main branch
    log_info "Setting main branch..."
    git branch -M main

    # Push to remote
    log_info "Pushing to remote repository..."
    if git push -u origin main; then
        log_info "Backup to git completed successfully"
    else
        log_error "Failed to push to remote repository"
        log_warn "Ensure remote 'origin' is configured: git remote add origin <repository-url>"
        exit 1
    fi
}

# Main execution
if [ "$#" -eq 0 ]; then
    show_help
    exit 0
fi

case "$1" in
    "dev"|"release")
        build_project "$1"
        ;;
    "backup")
        shift  # Remove 'backup' from arguments
        backup_project "$*"  # Pass all remaining arguments as commit message
        ;;
    "help"|"-h"|"--help")
        show_help
        ;;
    *)
        log_error "Unknown command: $1"
        show_help
        exit 1
        ;;
esac

exit 0
