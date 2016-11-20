# Viua VM ![Latest Release](https://img.shields.io/github/tag/marekjm/viuavm.svg)

[![Build Status](https://travis-ci.org/marekjm/viuavm.svg)](https://travis-ci.org/marekjm/viuavm)
[![Coverity Scan Build Status](https://img.shields.io/coverity/scan/7140.svg)](https://scan.coverity.com/projects/marekjm-viuavm)
[![Open issues](https://img.shields.io/github/issues/marekjm/viuavm.svg)](https://github.com/marekjm/viuavm/issues)

![License](https://img.shields.io/github/license/marekjm/viuavm.svg)


> A register-based virtual machine programmable in custom assembly lookalike language with
> strong emphasis on reliability, predictability, and concurrency.

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


### Use cases

Viua is a runtime environment focused on reliability, predictability, and concurrency.
It is suitable for long-running software like servers, message queues, and system daemons.
The VM is able to fully utilise all cores of the CPU it's running on (tested on a system with 8 hardware threads) so can
generate high CPU loads, but is relatively light on RAM and should not contain any memory leaks (all runtime tests are
run under Valgrind to ensure this).

The virtual machine is covered by ~380 tests to provide safety, and guard against possible regressions.
It ships with an assembler, and a static analyser, but does not provide any higher-level language compiler.


----


#### Design goals

- **predictable execution**: it is easier to reason about code when you know exactly how it will behave
- **predictable memory behaviour**: in Viua you do not have to guess when the memory will be released, or
  remember about the possibility of a gargabe collector kicking in and interrupting your program;
  Viua manages memory without a GC in a strictly scope-based (where "scope" means "virtual stack frame") manner
- **massive parallelism**: Viua architecture supports spawning massive amounts of independent, VM-based lightweight processes that can
  run in parallel (Viua is capable of providing true parallelism given sufficient hardware resources, i.e. at least two CPU cores)
- **concurrent I/O and FFI**: I/O operations and FFI calls cannot block the VM as they are executed on dedicated schedulers, so block only the
  process that called them
- **easy scatter/gather processing**: processes communicate using messages but machine supports a *join* instruction - which synchronises virtual
  processes execution, it blocks calling process until called process finishes and receives return value of called process (**WIP**)
- **safe inter-process communication** via message-passing (with queueing)
- **soft-realtime capabilities**: join and receive operations with timeouts (throwing exceptions if nothing has been received) make Viua
  a VM suitable to host soft-realtime programs (**WIP**)
- **fast debugging**: error handling is performed with exceptions (even across virtual processes), and unserviced exceptions cause the machine
  to generate precise and detailed stack traces; running programs are also debuggable with GDB
- **reliability**: programs running on Viua should be able to run until they are told to stop, not until they crash;
  machine helps writing reliable programs by providing a framework for detailed exception-based error communication and
  a way to register a per-process watchdog that handles processes killed by runaway exceptions (the exception is serviced by watchdog instead of
  killing the whole VM)


Some features also supported by the VM:

- separate compilation of Viua code modules
- static and dynamic linking of Viua-native libraries
- built-in, simple algorithm supporting multiple inheritance of user-defined types in Viua
- straightforward ways to use both dynamic and static method dispatch on objects
- first-class functions
- closures (with multiple way of capturing objects inside a closure)
- passing function parameters by value and move (non-copying pass)
- copy-free function returns
- inter-function tail calls
- support for pointer-aliases (with no arithmetic, and no assignment - pointers may be only used for reading and mutating objects)

For enhanced reliability, Viua assembler provides a built-in static analyser that is able to detect most common errors related to
register manipulation at compile time.
It provides traced errors whenever possible, i.e. when SA detects an error and is able to trace execution path that would trigger it,
a sequence of instructions (with source code locations) leading to the detected error is presented to the user instead of a single offending
instruction.


Current limitations include:

- severly limited introspection,
- no way to express atoms (i.e. all names must be known at compile time, and there is no way to tell the machine "Here, take this string,
  convert it to atom and return corresponding function/class/etc.")
- calling Viua code from C++ is not tested
- debugging information encoded in compiled files is limited
- speed: Viua is not the fastest VM around
- VM cannot distinguish FFI calls that are CPU or I/O bound (**WIP**), so if the program performs many I/O operations it may saturate all
  FFI schedulers (which will effectively prevent the program from quickly executing more FFI calls; virtual processes are still running, and
  Viua functions are still freely callable, though)


##### Software state notice

Viua VM is an alpha-stage software.
Even though great care is taken not to introduce bugs during development, it is inevitable that some will make their way into the codebase.
Viua in its current state *is not* production ready; bytecode definition and format will be altered, opcodes may be removed and
added without deprecation warning, and various external and internal APIs may change without prior notice.
Suitable announcements will be made when the VM reaches beta, RC and release stages.


#### Influences

The way Viua works has mostly been influenced by
Python (objects as dictionaries, the way multiple inheritance works),
C++ (static and dynamic method dispatch), and
Erlang (message passing, indepenedent VM-based lightweight processes as units of concurrency).
Maybe also a little bit of Java (more like JVM actually) and Ruby.
Some parts of Viua assembly syntax may remind some people of Lisp.


----


## Programming in Viua

Viua can be programmed in an assembly-like language which must be compiled into bytecode.
A typical session is shown below (assuming current working directory is the local clone of Viua repository):

```
 ]$ vi some_file.asm
 ]$ ./build/bin/vm/asm -o some_file.out some_file.asm
# static analysis or syntax errors...
 ]$ vi some_file.asm
 ]$ ./build/bin/vm/asm -o some_file.out some_file.asm
 ]$ ./build/bin/vm/kernel some_file.out
# runtime exceptions...
 ]$ vi some_file.asm
 ]$ ./build/bin/vm/asm -o some_file.out some_file.asm
 ]$ ./build/bin/vm/kernel some_file.out
```


----


# Development

Some development-related information.
Required tools:

* `g++`: GNU Compiler Collection's C++ compiler version 5.0 or later
* `clang++`: clang C++ compiler version 3.6.1 or later,
* `python`: Python programming language 3.x for test suite (optional),
* `valgrind`: for memory leak testing (optional; by default enabled, disabling required setting `MEMORY_LEAK_CHECKS_ENABLE` variable in `tests/tests.py` to `False`),

Other C++11 capable compilers may work but testing is only performed for G++ and Clang++.

Testing is only performed on Linux.
Compilation on on various BSD distributions should also work (but it may be problematic when compiling with Clang++).
Compilation on other operating systems is not tested.


## Cloning the repository

Best way to clone a repository (ensuring that the most recent code, and all submodules are fetched) is:

```
git clone --recursive --branch devel https://github.com/marekjm/viuavm
```


## Compilation

> Before compiling, Git submodule for `linenoise` library must be initialised.

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


----

### Contact information

Project website: [viuavm.org](http://viuavm.org/)

Maintainer: &lt;marekjm at ozro dot pw&gt;

Bugtracker: https://github.com/marekjm/viuavm/issues
