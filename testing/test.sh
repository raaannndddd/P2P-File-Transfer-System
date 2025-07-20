#!/bin/bash

# Function to compare two files
compare_files() {
    if cmp -s "$1" "$2"; then
        echo "Test passed: $1"
    else
        echo "Test failed: $1"
    fi
}

for input_file in tests/bpkg_tests/test*.*.in; do
    test_name="${input_file%.in}"
    expected_output_file="$test_name.expected"
    actual_output_file="$test_name.out"

    echo "Running test: $test_name"

    # Run the C program with input from the test file
    ./pkgmain "$input_file" > "$actual_output_file" 2> ./stderr

    # Compare the actual output with the expected output
    compare_files "$actual_output_file" "$expected_output_file"
done

for input_file in tests/merkle_tests/test*.*.in; do
    test_name="${input_file%.in}"
    expected_output_file="$test_name.expected"
    actual_output_file="$test_name.out"

    echo "Running test: $test_name"

    # Run the C program with input from the test file
    ./pkgmain "$input_file" > "$actual_output_file" 2> ./stderr

    # Compare the actual output with the expected output
    compare_files "$actual_output_file" "$expected_output_file"
done