#!/bin/bash

# Colors for better output readability
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${YELLOW}===== ELF Parser Test Script =====${NC}"
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

# Test 1: Valid shared library (our own)
run_test "Valid shared library (libmylib.so)" \
         "./isos_loader -v ./libmylib.so foo_exported" \
         "SUCCESS"

# Test 2: System C library
run_test "System C library" \
         "./isos_loader -v /usr/lib/x86_64-linux-gnu/libc.so.6 printf" \
         "SUCCESS"

# Test 3: Math library
run_test "Math library" \
         "./isos_loader -v /usr/lib/x86_64-linux-gnu/libm.so.6 sin" \
         "SUCCESS"

# Test 4: Non-existent library
run_test "Non-existent library" \
         "./isos_loader -v ./does_not_exist.so foo" \
         "FAIL"

# Test 5: Non-ELF file
echo "This is not an ELF file" > $TEMP_DIR/not_elf.so
run_test "Non-ELF file" \
         "./isos_loader -v $TEMP_DIR/not_elf.so foo" \
         "Not an ELF file"

# Test 6: Create a corrupted ELF file (magic number)
cp libmylib.so $TEMP_DIR/corrupt_magic.so
dd if=/dev/urandom of=$TEMP_DIR/corrupt_magic.so bs=1 count=4 conv=notrunc 2>/dev/null
run_test "Corrupted ELF magic number" \
         "./isos_loader -v $TEMP_DIR/corrupt_magic.so foo_exported" \
         "Not an ELF file"


# Clean up
echo -e "${YELLOW}Cleaning up test files...${NC}"
rm -rf $TEMP_DIR

echo -e "${YELLOW}===== Test Script Complete =====${NC}"

#fonction dont exixt & fonction that exist in the same time 