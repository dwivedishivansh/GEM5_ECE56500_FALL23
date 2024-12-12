#!/bin/bash

# Define variables
SCONS_COMMAND="scons-3 USE_HDF5=0 -j $(nproc)"
BUILD_PATH="./build/X86/gem5.fast"

# Execute the command
$SCONS_COMMAND $BUILD_PATH