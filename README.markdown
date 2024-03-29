# Viua VM ![Latest Release](https://img.shields.io/github/tag/marekjm/viuavm.svg)

[![builds.sr.ht status](https://builds.sr.ht/~maelkum.svg?search=viuavm)](https://builds.sr.ht/~maelkum?search=viuavm)
[![CII Best Practices](https://bestpractices.coreinfrastructure.org/projects/581/badge)](https://bestpractices.coreinfrastructure.org/projects/581)

![License](https://img.shields.io/github/license/marekjm/viuavm.svg)


> A register-based, parallel virtual machine programmable in custom assembly lookalike language with
> strong emphasis on reliability, predictability, and concurrency.

#### Building the code

```
]$ ./configure      # (1) configure the project
]$ make -j          # (2) build it
```

Calling `./configure` is not required if you do not want or need to run tests
under Valgirnd. Configuring will create necessary suppresion file for your
version of Valgrind.

**Note**: the default build is run with `-O0` (without any optimisations).
You can compile Viua VM with higher optimisation leves, but keep in mind that
the compilation might not succeed on machines with less than 4GB RAM if you use
the `-O3` optimisation level.

This is tracked by issue `7c06177872c3a718510a54e6513820f8fe0fb99b` in the
embedded issue repository.


#### Hello World in Viua VM

```
.function: main/0
    allocate_registers %3 local

    text %1 local "Hello World!\n"
    integer %2 local 1

    io_write %2 local %2 local %1 local
    io_wait void %2 local 1s

    izero %0 local
    return
.end
```

See `README.asm` file to see commented version of this sample program.

For more examples visit either [documentation](http://docs.viuavm.org/) or
[Rosetta](http://rosettacode.org/wiki/Viua_VM_assembly) page for Viua VM.
Check [Weekly](http://weekly.viuavm.org/) blog for news and developments in
Viua VM.


### Use cases

Viua is a runtime environment focused on reliability, predictability, and
concurrency. It is suitable for writing long-running software that runs in the
background providing infrastructure services:

- servers (e.g. IRC)
- message queues
- system and application daemons (e.g. cron)

The VM is not ready to be used in places where performance matters.
It is best to employ it for tasks which do not require human interaction or near
real-time responses.

The VM should be able to fully utilise all cores of the CPU it's running on to
execute the virtual processes and FFI calls of the software it runs. If
configured to do so it may generate high CPU loads, but is relatively light on
RAM and should not contain any memory leaks (all runtime tests are run under
Valgrind to ensure this).

The virtual machine is covered by more than 500 tests to provide safety, and
guard against possible regressions. It ships with an assembler and a static
analyser, but does not provide any higher-level language compiler or debugger.


----


#### Design goals

- **predictable execution**: it is easier to reason about code when you know
  exactly how it will behave
- **predictable value lifetimes**: in Viua you do not have to guess when the
  memory will be released, when objects will be destroyed, or remember about the
  possibility of a gargabe collector kicking in and interrupting your program.
  Viua manages resources without a GC in a strictly scope-based (where "scope"
  means "virtual stack frame") manner
- **massive concurrency**: Viua architecture supports spawning massive amounts
  of independent, VM-based lightweight processes that can run in parallel (Viua
  is capable of providing true parallelism given sufficient hardware resources,
  i.e. at least two physical CPU cores)
- **parallel I/O and FFI schedulers**: I/O operations and FFI calls cannot block
  the VM as they are executed on dedicated schedulers, so block only the virtual
  process that called them without affecting other virtual processes
- **easy scatter/gather processing**: processes communicate using messages but
  machine supports a *join* instruction - which synchronises virtual processes
  execution, it blocks calling process until called process finishes and
  receives return value of called process
- **safe inter-process communication** via message-passing (with queueing)
- **soft-realtime capabilities**: it is possible to set a timeout on process
  joins, message receives, and all I/O operations. It should make Viua a VM
  suitable to host soft-realtime programs that need to provide some timing
  guarantees
- **fast debugging**: error handling is performed with exceptions (even across
  virtual processes), and unserviced exceptions cause the machine to generate
  precise and detailed stack traces
- **reliability**: programs running on Viua should be able to run until they are
  told to stop, not until they crash; machine helps writing reliable programs by
  providing a framework for detailed exception-based error communication and a
  way to register a per-process watchdog that handles processes killed by
  runaway exceptions (the exception is serviced by watchdog instead of killing
  the whole VM)


Some features also supported by the VM:

- separate compilation of Viua code modules
- static and dynamic linking of Viua-native libraries
- straightforward ways to use both dynamic and static function call dispatch
- first-class functions
- closures (with multiple way of capturing objects inside a closure)
- passing function parameters by value, by move (non-copying pass), and by pointer
- copy-free function returns
- inter-function tail calls
- several variants of fixed-size integer arithmetic: wrapping, checked, and
  saturating

For enhanced reliability, Viua assembler provides a built-in static analyser
that is able to detect most common errors related to register manipulation at
compile time (type mismatches, invalid register access). It provides traced
errors whenever possible, i.e. when it detects an error and is able to trace
execution path that would trigger it, a sequence of instructions (with source
code locations) leading to the detected error is presented to the user instead
of a single offending instruction.


Current limitations include:

- severly limited introspection
- calling Viua code from C++ is not tested
- debugging information encoded in compiled files is limited
- speed: Viua is not the fastest VM around
- lack of libraries


##### Software state notice

Viua VM is an alpha-stage software.

Even though great care is taken not to introduce bugs during development, it is
inevitable that some will make their way into the codebase. Viua in its current
state *is not* production ready; bytecode definition and format will be altered,
opcodes may be removed and added without deprecation warning, and various
external and internal APIs may change without prior notice.

Suitable announcements will be made when the VM reaches beta, RC and release
stages.


#### Influences

The way Viua works has mostly been influenced by
C++ (static and dynamic method dispatch, move semantics), and
Erlang (message passing, indepenedent VM-based lightweight processes as units of
concurrency).


----


## Programming in Viua

Viua can be programmed in an assembly-like language which must be compiled into
bytecode. A typical session is shown below (assuming current working directory
is the local clone of Viua repository):

```
 ]$ vim some_file.asm
 ]$ ./build/bin/vm/asm -o some_file.bc some_file.asm
# static analysis or syntax errors...
 ]$ vim some_file.asm
 ]$ ./build/bin/vm/asm -o some_file.bc some_file.asm
 ]$ ./build/bin/vm/kernel some_file.bc
# runtime exceptions...
 ]$ vim some_file.asm
 ]$ ./build/bin/vm/asm -o some_file.bc some_file.asm
 ]$ ./build/bin/vm/kernel some_file.bc
```

----

## Contributing

Please read [CONTRIBUTING.markdown](./CONTRIBUTING.markdown) for details on
development setup, and the process for submitting patches, bug reports, and
feature suggestions.

Issues are tracked using the [Issue](https://git.sr.ht/~maelkum/issue) tool.

Patches should be submitted via email. See "Contact information" at the bottom
of this file.

The code can be cloned either
from [Savannah](http://git.savannah.nongnu.org/cgit/viuavm.git) or
from [Sourcehut](https://git.sr.ht/~maelkum/viuavm).


----

## License

The code is licensed under GNU GPL v3.


----

### Contact information

Project website: [viuavm.org](http://viuavm.org/)

Maintainer: &lt;marekjm at ozro dot pw&gt;
