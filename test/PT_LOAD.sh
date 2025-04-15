#!/bin/bash

# Colors for better output readability
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${YELLOW}===== PT_LOAD Segments Validation Test =====${NC}"
echo ""

# Make sure we have our binaries
echo -e "${YELLOW}Building project...${NC}"
make clean
make
if [ ! -f "isos_loader" ] || [ ! -f "libmylib.so" ]; then
    echo -e "${RED}Build failed! Make sure all source files are present.${NC}"
    exit 1
fi

# Create a temp directory for test files
TEMP_DIR="./elf_test_files"
mkdir -p $TEMP_DIR

# Function to run a test and report results
run_test() {
    local test_name="$1"
    local command="$2"
    local expected_result="$3"
    
    echo -e "${YELLOW}Test: $test_name${NC}"
    
    # Run the command and capture output and exit code
    output=$(eval "$command" 2>&1)
    echo "$output" 
    exit_code=$?
    
    # Check if we got expected result (either in output or exit code)
    if [[ $expected_result == "SUCCESS" && $exit_code -eq 0 ]]; then
        echo -e "${GREEN}PASSED${NC}"
    elif [[ $expected_result == "FAIL" && $exit_code -ne 0 ]]; then
        echo -e "${GREEN}PASSED${NC} (correctly detected invalid file)"
    elif [[ $output == *"$expected_result"* ]]; then
        echo -e "${GREEN}PASSED${NC} (found expected message: '$expected_result')"
    else
        echo -e "${RED}FAILED${NC}"
        echo "Command: $command"
        echo "Exit code: $exit_code"
        echo "Output: $output"
    fi
    echo ""
}

# Test 1: Test valid library with PT_LOAD segments
run_test "Valid shared library with PT_LOAD segments" \
         "./isos_loader -v ./libmylib.so foo_exported" \
         "Load segments found"

# Test 2: Test libc with multiple PT_LOAD segments
run_test "Multiple PT_LOAD segments in libc" \
         "./isos_loader -v /usr/lib/x86_64-linux-gnu/libc.so.6 printf" \
         "Total memory size required"

# Clean up
echo -e "${YELLOW}Cleaning up test files...${NC}"
rm -rf $TEMP_DIR

echo -e "${YELLOW}===== Test Complete =====${NC}"