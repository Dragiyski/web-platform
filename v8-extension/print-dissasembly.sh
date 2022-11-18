#!/bin/bash

# Print section headers (used to orient into the file)
objdump -Ch ./build/Release/native.node

# Print the disassembly with demangled names
objdump -drRC -Mintel,amd64 ./build/Release/native.node

# Print the readonly data (containing all const char *)
objdump -s -j .rodata ./build/Release/native.node
