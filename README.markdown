# Viua VM ![Latest Release](https://img.shields.io/github/tag/marekjm/viuavm.svg)

[![Build Status](https://travis-ci.org/marekjm/viuavm.svg)](https://travis-ci.org/marekjm/viuavm)
[![Coverity Scan Build Status](https://img.shields.io/coverity/scan/7140.svg)](https://scan.coverity.com/projects/marekjm-viuavm)
[![Open issues](https://img.shields.io/github/issues/marekjm/viuavm.svg)](https://github.com/marekjm/viuavm/issues)

![License](https://img.shields.io/github/license/marekjm/viuavm.svg)


> A simple register-based virtual machine.


#### Design goals

- predictable execution: it is easier to reason about code when you know exactly how it will behave
- predictable memory behaviour: in Viua you do not have to guess when the memory will be released, or
  remember about the possibility of a gargabe collector kicking in and interrupting your program;
  Viua manages memory without a GC in a strictly scope-based manner
- massive parallelism: parallelism model in Viua supports spawning massive amounts of independent, VM-based threads - and
  provides means to either detach spawned threads or join them later during execution;
  inter-thread communication is performed via message-passing
- fast debugging: error handling is performed with exceptions (even across threads), and unserviced exceptions cause the machine
  to generate precise and detailed stack traces;
  running programs are also debuggable with GDB or Viua-specific debugger


Some features also supported by the VM:

- static and dynamic linking of Viua-native libraries,
- polymorhism and multiple inheritance,
- straightforward ways to use both dynamic and static method dispatching on objects,
- first-class functions,
- closures,


Current limitations include:

- severly limited introspection,
- no way to express atoms (i.e. all names must be known at compile time, and there is no way to tell the machine "Here, take this atom and return corresponding function/class/etc."),
- calling Viua code from C++ is not tested,
- multithreaded code must be debugged with GDB instead of debugger supplied by the VM,
- debugging information encoded in compiled files is limited,


##### Software state notice

Viua VM is an alpha-stage software.
Even though great care is taken not to introduce bugs during development, it is inevitable that some will make their way into the codebase.
Viua in its current state *is not* production ready; bytecode definition and format will be altered, opcodes may be removed and
added without deprecation warning, and various external and internal APIs may change without prior notice.
Suitable announcements will be made when the VM reaches beta, RC and release stages.


----


## Programming in Viua

Viua can be programmed in an assembly-like language which must be compiled into bytecode.
Typical code-n-debug cycle is shown below (assuming current working directory
is the local clone of Viua repository):

```
vi some_file.asm
./build/bin/vm/asm -o some_file.out some_file.asm
./build/bin/vm/cpu some_file.out
./build/bin/vm/vdb some_file.out
```


----


# Development

Some development-related information.
Required tools:

* `g++`: GNU Compiler Collection's C++ compiler version 4.9 and above (mandatory),
* `clang++`: clang C++ compiler version 3.6.1 and above (if not using GCC, clang builds are **not** guaranteed to work),
* `python`: Python programming language 3.x for test suite (optional),
* `valgrind`: for memory leak testing (optional; by default enabled, disabling required setting `MEMORY_LEAK_CHECKS_ENABLE` variable in `tests/tests.py` to `False`),


## Compilation

Before compiling, Git submodule for `linenoise` library must be initialised.

Compilation is simple and can be executed by typing `make` in the shell.
Full, clean compilation can also be performed by the `./scripts/recompile` script.
The script will run `make clean` production, detect number of cores the machine compilation is done on has, and
run `make` with `-j` option adjusted to take advantage of multithreaded `make`-ing.

Incremental recompilation can be performed with either `make` or `./scripts/compile` (the latter will detect the number of
cores available and adjust `-j` option in Make).


## Testing

There is a simple test suite located in `tests/` directory.
It can be invoked by `make test` command.
Python 3 must be installed on the machine to run the tests.

Code used for unit tests can be found in `sample/` directory.


### Viua development scripts

In the `scripts/` directory, you can find scripts that are used during development of Viua.
The shell installed in dev environment is ZSH but the scripts should be compatible with BASH as well.


## Git workflow

Each feature and fix is developed in a separate branch.
Bugs which are discovered during development of a certain feature,
may be fixed in the same branch as their parent issue.
This is also true for small features.

**Branch structure:**

- `master`: master branch - contains stable, working version of VM code,
- `devel`: development branch - all fixes and features are first merged here,
- `issue/<number>/<slug>` or `issue/<slug>`: for issues (both enhancement and bug fixes),


## Patch submissions

Patch submissions and issue reports are both welcome.

Rememeber, though, to provide appropriate test cases with code patches you submit.
It will be appreciated and will make the merge process faster.


----

## License

The code is licensed under GNU GPL v3.
