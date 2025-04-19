#!/bin/bash

# Colors for better output readability
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${YELLOW}===== Testing Symbol Visibility =====${NC}"
echo ""

# Make sure we have our binaries
echo -e "${YELLOW}Building project...${NC}"
make clean
make
if [ ! -f "isos_loader" ] || [ ! -f "libmylib.so" ] || [ ! -f "libmylib_hidden.so" ]; then
    echo -e "${RED}Build failed! Make sure all source files are present.${NC}"
    exit 1
fi

# Function to run test with a bit more safety
run_test() {
    local lib=$1
    local func=$2
    local expected=$3
    
    echo -e "\n${YELLOW}Testing $func in $lib:${NC}"
    output=$(./isos_loader -v $lib $func 2>&1)
    
    # Check if the output contains the expected string
    if [[ $output == *"$expected"* ]]; then
        echo -e "${GREEN}SUCCESS${NC}: Found expected output: '$expected'"
    else
        echo -e "${RED}FAILURE${NC}: Expected '$expected' but got different output"
        echo "Output: $output"
    fi
}

echo -e "\n${YELLOW}=== Testing normal library (default visibility) ===${NC}"
run_test "./libmylib.so" "foo_exported" "foo_exported() => Adresse: 0x"
run_test "./libmylib.so" "bar_exported" "bar_exported() => Adresse: 0x"

echo -e "\n${YELLOW}=== Testing hidden library (only explicitly exported symbols visible) ===${NC}"
run_test "./libmylib_hidden.so" "foo_exported" "foo_exported() => Adresse: 0x"
run_test "./libmylib_hidden.so" "bar_exported" "bar_exported() => Adresse: 0x"

echo -e "\n${YELLOW}=== Testing hidden function in regular library ===${NC}"
run_test "./libmylib.so" "hidden_func" "hidden_func() => Adresse: 0x"

echo -e "\n${YELLOW}=== Testing hidden function in hidden library ===${NC}"
run_test "./libmylib_hidden.so" "hidden_func" "not found"

# Also test the symbol visibility with nm
echo -e "\n${YELLOW}=== Checking symbols in normal library ===${NC}"
nm -D libmylib.so

echo -e "\n${YELLOW}=== Checking symbols in hidden library ===${NC}"
nm -D libmylib_hidden.so

echo -e "\n${YELLOW}===== Test Complete =====${NC}"