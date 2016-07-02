# Main function

Viua VM allows the program to use several flavours of the main function, differing in
their arity (i.e. the number of parameters taken).
A list of legal signatures is provided below:

- `main/0`: receives no parameters,
- `main/1`: receives a vector with all commandline parameters,
- `main/2`: receives two parameters; first the name of the program, and vector with the
  rest of the commandline parameters second,

----

## Linking main function

Main function does not have to be located in "main" translation unit of the final program.
For example, take a look at the following program:

**example.asm**

```
.function: hello_world/0
    print (strstore 1 "Hello World!")
    return
.end
```

**main.asm**

```
.signature: hello_world/0

.function: main/0
    frame 0
    call hello_world/0

    izero 0
    return
.end
```

It can be compiled using this commands:

```
# create a library with main/0 function
viua-asm -c -o main.lib main.asm

# create a final executable
viua-asm example.asm main.lib
```

Note that during compilation of the `example.asm` the `main/0` function was statically
linked into the main translation unit of the final program.
A translation unit is considered to be the "main" unit of a program if:

- after linking, it contains any variant of the `main/` function,
- it is compiled as an executable,

The `main/` function **MUST** be contained in the final executable - it cannot be linked
during runtime (it is the only function, apart from `__entry`, the cannot be linked
dynamically).
