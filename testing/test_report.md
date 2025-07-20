# Assignment 3 Testing
<p> Author: Rand Halasa </p>
<p> Date: Sunday, May 12th, 2024. 11:59 pm </p>
<p> AI used (yes/no): no. </p>

## Test 1 .bpkg file
### Description:
This test case ensures that bpkg parsing runs properly and returns the expected result given expected input. 
The input .bpkg file has all the elements in the correct order. 
The expected output when running the program with the -all_hashes flag is all the hashes in the file.

## Test 2 .bpkg file
### Description:
This test case checks if the program runs properly given zero hashes (edge case).
The input will be a .bpkg with all the elements in the expected order, but nhashes and nchunks will be set to zero and hashes and chunks will be empty.
Using the -all_hashes flag, the expected output is for nothing to be printed, but for the program to run without errors.

## Test 3 .bpkg file
### Description:
Testing a corrupt file.
The input will be a corrupt file where ident is not a hexadecimal string and more than 1024 characters.
The expected output is "Unable to load pkg and tree"

## Test 4 .bpkg file
### Description:
Testing a corrupt file.
The input will be a corrupt file where filename is longer than 256 characters.
Expected output is "Unable to load pkg and tree"

## Test 5 .bpkg file
### Description:
Testing a corrupt file.
Input will be a .bpkg file where size is a string
Expected output is "Unable to load pkg and tree"

## Test 6 .bpkg file
### Description:
Testing a corrupt file.
The input will be a corrupt file where nhashes and number of hashes do not match
The expected output is "Unable to load pkg and tree"

## Test 7 .bpkg file
### Description:
Testing a corrupt file.
The input will be a corrupt file where nchunks is a string
The expected output is "Unable to load pkg and tree"

## Test 8 .bpkg file
### Description:
Testing a corrupt file.
Passing in an empty file, and files with one of the elements missing
The expected output is "Unable to load pkg and tree" for all

## Test 9 .bpkg file
### Description:
Testing a corrupt file.
Passing in files with the wrong order
The expected output is "Unable to load pkg and tree" for all

## Test 10 .bpkg file
### Description:
Testing a corrupt file. 
Input file will have nchunks that do not equal nhashes+1
The expected output should be "Unable to load pkg and tree" for all

## Test 11 .bpkg file
### Description:
Testing corrupted data file
Input file content does not equal the .bpkg file when hashed
Output should raise "Corrupted file" error

## Test 1 merkle
### Description:
Test that a program works properly with incomplete merkle tree
Pass in a .bpkg file that will result in an incomplete merkle tree
Output using the -get_root flag returns the root hash of the tree. Program works as expected

## Test 2 merkle
### Description:
Test inorder traversal
Pass in a complete .bpkg file
The expected output using the -bst flag should return the rightmost node hash

## Test 3 merkle
### Description:
Test that the program assumes there will be no two identical chunks in two different locations
Pass in a complete .bpkg file with repeating chunks
Output using the -get_root flag returns the root hash of the tree. Program works as expected.

## Test 4 merkle
### Description:
Check that hash of root node is equal to hash in .bpkg file
Expected output is for both to be the same using -get_root function