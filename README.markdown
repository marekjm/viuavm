# Wudoo VM

A simple, bytecode driven, register-based virtual machine.

I develop Wudoo VM to learn how such software is created and
to help myself in my computer language implementation studies.

----

## Programming

Wudoo can be programmed in an assembler-like language which must then be compiled into bytecode, or
via  **Bytecode Generation API** which then generates bytecode.

The internal assembler uses the bytecode generation API.


----

## Development

Some development-related information.


### Wudoo development scripts

In the `scripts/` directory, you can find scripts that are used during development of Wudoo.
The shell installed in dev environment is ZSH but the scripts should be compatible with BASH as well.


## Testing

There is a simple test suite (written in Python 3) located in `tests/` directory.
It can be invoked by `make test` command.

Code used for unit tests can be found in `sample/` directory.


----

## License

The code is licensed under GNU GPL v3.
