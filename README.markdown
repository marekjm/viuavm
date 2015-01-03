# Wudoo VM

A simple, bytecode driven, register-based virtual machine.

It is a project I do to learn how such software is created and
to help me in my computer language implementation studies.

----

## Programming

Wudoo can be programmed in an assembler-like language which must be compiled into bytecode, or
via  **Bytecode Programming API** (a.k.a. bpapi) which then generates bytecode.

The internal assembler uses the bpapi.

It is also possible to program directly in bytecode by putting bytes in proper order into an array and
dumping it to a file, or feeding to Wudoo CPU.


----

## Development

Some development-related information.


### Wudoo development scripts

In the `scripts/` directory, you can find scripts that are used during development of Wudoo.
The shell installed in dev environment is ZSH but the scripts should be compatible with BASH as well.

Some of the scripts may be written in Python 3 in the future.


### Sample asm code

Sample asm code are very small programs which test a small subset of VM instructions.
They are be used in lieu of real test suite in early phase of development.

**Correct outputs**:

- for numerical features (e.g. integers, floats, uints etc.): the output should be `1`,
- for logical features (e.g. `ilt` or `ieq` instructions): teh output should be `true`,
- for byte printing: the output should be `Hello World!`,
- for string printing: the output should be `Hello World!`,


----

## License

The code is licensed under GNU GPL v3.
