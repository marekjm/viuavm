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


## Git Workflow

Each feature and fix is developed in a separate branch.
Bugs which are discovered during development of a certain feature of bug fixing,
may be fixed in the same branch as their parent issue.
This is also true for small features.

**Branch structure:**

- `master`: master branch - contains stable, working version of VM code,
- `devel`: development branch - all fixes and features are first merged here,
- `feature/issue/<number>/<title>`: for features and enhancements,
- `fix/issue/<number>/<title>`: for bugixes,
- `vm/bytecode`: special branch related to general bytecode development outside of issue system,
- `vm/assembler`: branch used for various small fixes and developments related to VM's assembler,
- `vm/cpu`: branch used for various small fixes and developments related to CPU code,


Explained with arrows:

```
fix/* ←----→ devel ←----→ feature/*
               |
               ↓
             master
```


----

## License

The code is licensed under GNU GPL v3.
