# Tatanka binaries

Tatanka binaries contain compiled Tatanka bytecodes and the encoded size of compiled program.

First 16 bytes must be treated by VM as `uint16_t` encoded size of the bytecode.
Second 16 bytes must be treated as an offset (`uint16_t`) under which to start execution.

Bytes between 32. and the byte denoted by the second `uint16_t` are function definitions.
