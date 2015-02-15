# Native Wudoo VM Assembly Language

Programs written to run on Wudoo VM must be compiled to its bytecode.
A typical intermediate step may be to first translate them into native Wudoo assembly language.

Wudoo assembly language are simple text files with extension either `.asm` or `.wudoo`.


----

## Instructions

Wudoo asm features two types of instructions:

- asm instructions, e.g. `.mark: foo` which are processed by the assembler and may or may not generate any bytecode,
- CPU instructions, e.g. `jump :foo` which are generate bytecode which can be then loaded into the CPU,

Each instruction set is explained in respective section below.


----

## Assembler instructions

This section explains and lists Wudoo assembler instructions.


### `.mark:`

Used to mark a place in code.
Markers can be used in `jump` and `branch` CPU instructions.


### `.name:`

Assign a name to the register.


### `.def:` and `.end`

Used to define functions.


### `.dec:`

Declare a function.


### `.dex:`

Declare a function and do not mangle its name.


### `.main:`

**Syntax**: `.main: <function_name>`

Used to set main function name.
Defaults to `main`.

Assembler will abort translation if no `main` function is defined and `.main:` did not set an alternative.


### `.link:`

**Syntax**: `.link: <module>`

Used to statically link link modules and libraries.
