# Tatanka binaries

Tatanka binaries contain compiled Tatanka bytecodes and the encoded size of compiled program.
First 16 bytes must be treated by VM as `uint16_t` encoded size of the bytecode.
