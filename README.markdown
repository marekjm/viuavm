# Viua VM ![Latest Release](https://img.shields.io/github/tag/marekjm/viuavm.svg)

[![Build Status](https://travis-ci.org/marekjm/viuavm.svg)](https://travis-ci.org/marekjm/viuavm)
[![Coverity Scan Build Status](https://img.shields.io/coverity/scan/7140.svg)](https://scan.coverity.com/projects/marekjm-viuavm)
[![Open issues](https://img.shields.io/github/issues/marekjm/viuavm.svg)](https://github.com/marekjm/viuavm/issues)

![License](https://img.shields.io/github/license/marekjm/viuavm.svg)


> A simple, register-based virtual machine programmable in custom assembly lookalike language with
> strong emphasis on reliability and predictability.

#### Hello World in Viua VM

```
.function: main/0
    ; store string "Hello World!" in register 1
    strstore 1 "Hello World!"

    ; print contents of register 1
    print 1

    ; set exit code
    ; return values are stored in zeroeth register
    izero 0
    ; and return from the main function
    return
.end
```


----


#### Design goals

- **predictable execution**: it is easier to reason about code when you know exactly how it will behave
- **predictable memory behaviour**: in Viua you do not have to guess when the memory will be released, or
  remember about the possibility of a gargabe collector kicking in and interrupting your program;
  Viua manages memory without a GC in a strictly scope-based manner
- **massive parallelism**: parallelism model in Viua supports spawning massive amounts of independent, VM-based lightweight processes and
  provides means to either detach spawned processes or join them later during execution
- **safe inter-process communication** via message-passing (with queueing)
- **fast debugging**: error handling is performed with exceptions (even across processes), and unserviced exceptions cause the machine
  to generate precise and detailed stack traces;
  running programs are also debuggable with GDB or Viua-specific debugger
- **reliability**: programs running on Viua should be able to run until they are told to stop, not until they crash;
  machine helps writing reliable programs by providing a framework for detailed exception-based error communication and
  a way to register a watchdog process that handles processes killed by runaway exceptions (the exception is serviced by watchdog instead of
  killing the whole VM)


Some features also supported by the VM:

- separate compilation,
- static and dynamic linking of Viua-native libraries,
- multiple inheritance,
- straightforward ways to use both dynamic and static method dispatch on objects,
- first-class functions,
- closures (with multiple way of enclosing objects inside a closure),
- passing function parameters by value, pointer and move (non-copying pass),
- copy-free function returns,


Current limitations include:

- severly limited introspection,
- no way to express atoms (i.e. all names must be known at compile time, and there is no way to tell the machine "Here, take this string, convert it to atom and return corresponding function/class/etc."),
- calling Viua code from C++ is not tested,
- concurrent code must be debugged with GDB instead of debugger supplied by the VM,
- debugging information encoded in compiled files is limited,
- speed: Viua is not the fastest VM around,


##### Software state notice

Viua VM is an alpha-stage software.
Even though great care is taken not to introduce bugs during development, it is inevitable that some will make their way into the codebase.
Viua in its current state *is not* production ready; bytecode definition and format will be altered, opcodes may be removed and
added without deprecation warning, and various external and internal APIs may change without prior notice.
Suitable announcements will be made when the VM reaches beta, RC and release stages.


#### Influences

The way Viua works has mostly been influenced by
Python (objects as dictionaries, the way multiple inheritance works),
C++ (move semantics, static and dynamic method dispatch), and
Erlang (message passing, indepenedent VM-based lightweight processes as units of concurrency).
Maybe also a little bit of Java (more like JVM actually) and Ruby.
Syntax of Viua assembly may remind some people of Lisp (and for a good reason).


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

* `g++`: GNU Compiler Collection's C++ compiler version 5.0 and above (mandatory; development started with 4.9 so it should also be supported even if
  compilation is no longer tested with 4.9),
* `clang++`: clang C++ compiler version 3.6.1 and above,
* `python`: Python programming language 3.x for test suite (optional),
* `valgrind`: for memory leak testing (optional; by default enabled, disabling required setting `MEMORY_LEAK_CHECKS_ENABLE` variable in `tests/tests.py` to `False`),

Other C++11 capable compilers may work but testing is only performed for G++ and Clang++.

Testing is only performed on Linux.
Compilation on BSD should also work but it may be problematic when compiling with Clang++.
Compilation on other operating systems is not tested.


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
