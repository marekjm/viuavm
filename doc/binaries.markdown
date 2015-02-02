# Wudoo VM binaries

Wudoo binaries contain compiled Wudoo bytecodes and the encoded size of compiled program.

First 16 bytes must be treated by VM as `uint16_t` encoded size of the bytecode.
Second 16 bytes must be treated as an offset (`uint16_t`) under which to start execution.

Bytes between 32. and the byte denoted by the second `uint16_t` are function definitions.


----

## Structure

```
uint16_t        - size of the function-name-to-id mapping section,

char*           - function ID encoded as null-terminated string
byte*           - bytecode address to call for this ID

uint16_t        - bytecode size
uint16_t        - first executable instruction

byte*           - instructions
```
