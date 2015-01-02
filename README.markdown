# Wudoo VM

A simple, bytecode driven, register-based virtual machine.

It is a project to learn how such software is created and
to help me in my computer language implementation studies.

----

## Programming

Wudoo can be programmed in an assembler-like language which must be compiled into bytecode, or
via  **Bytecode Programming API** (a.k.a. bpapi) which then generates bytecode.

The internal assembler uses the bpapi.

It is also possible to program directly in bytecode by putting bytes in proper order into an array and
dumping it to a file, or feeding to Wudoo CPU.


----

## License

The code is licensed under GNU GPL v3.
