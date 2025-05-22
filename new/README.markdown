# Viua VM

Viua is a bytecode virtual machine designed for reliable execution of massively
concurrent programs. Internals of the VM are designed to allow running as much
of the code as reasonably possible in parallel, taking advantage of manycore
processors. The ISA presented to users exposes an execution model based on the
Actor Model.

```
; Name of the entry point function can be customised, but it should be "main" to
; follow an established convention and avoid surprising fellow programmers.
.function: [[entry_point]] main
    ; Load immediate integer to local register 1. This is a pseudoinstruction
    ; that is automatically expanded by the assembler.
    li $1.l, 42u

    ; By default, display some debugging information either to standard error or
    ; to file descriptor supplied in the VIUA_VM_TRACE_FD environment variable.
    ebreak

    ; Return from the function. The "void" operand can be omitted (it is the
    ; default) or a register index may be specified to return a value.
    return void
.end
```

The above code can be assembled and run using the following commands:

```
]$ ./build/libexec/viua/asm -o readme.o readme.asm
]$ ./build/libexec/viua/ld -o readme.elf readme.o
]$ ./build/libexec/viua/vm readme.elf
```

...and inspected using the following commands:

```
]$ file readme.*
]$ readelf readme.elf
]$ ./build/libexec/viua/readelf readme.elf
]$ ./build/libexec/viua/dis readme.elf
```

In case of bugs, or when step by step execution is useful, a simple debugger is
also available:

```
]$ ./build/libexec/viua/repl
```

## Tools

A set of tools is provided for programmers to interact and use the VM. These
are:

- `vm`: the actual VM implementation used to run executable ELF files containing
  Viua bytecode
- `asm` and `dis`: the assembler and the disassembler
- `ld`: the linker
- `readelf`: a `readelf(1)` specific to Viua ELFs (the usual one is still useful
  as Viua ELFs are valid ELF files)
- `repl`: a primitive debugger

--------------------------------------------------------------------------------

## Compiling

The whole toolchain can be compiled using GCC or Clang. The build is automated
using GNU Make:

```
]$ make -j      # or make VERBOSE=true -j to see full compiler invocations
]$ ./build/libexec/viua/vm --version
]$ ./build/libexec/viua/vm --version --verbose
```

See [BUILD](./BUILD.markdown) for more information.

--------------------------------------------------------------------------------

## Installation

After having built the toolchain, you can install it with the following command:

```
]$ make install
```

If you need to install into a directory different than /usr/local, you must
override the default prefix with:

```
]$ make prefix=~/.local install
```

--------------------------------------------------------------------------------

## Transparent execution

Linux uses `binfmt_misc` loader to provide support for various executable file
formats. See https://www.kernel.org/doc/html/latest/admin-guide/binfmt-misc.html
for more information.

Viua VM binaries can be executed "transparently" (ie, without explicitly passing
them to the interpreter) using this infrastructure. For one-time testing you can
use the following command, executed as root:

    # cat ./binfmt.d/viua-exec.conf > /proc/sys/fs/binfmt_misc/register

For a more permanent solution install the file in /usr/local/lib/binfmt.d or
consult `binfmt.d(5)` for information about appropriate installation directory
on your system. Remember to check and adjust the interpreter's location if
necessary!

--------------------------------------------------------------------------------

## Contributing

For guidelines for potential contributors see the
[CONTRIBUTING](CONTRIBUTING.markdown) file.

--------------------------------------------------------------------------------

## Copyright

Copyright © 2021-2025 Marek Marecki

Viua VM is Free Software published under GNU GPL v3 license.

### Contact information

Project website: [viuavm.org](http://viuavm.org/)

See [MAINTAINERS](MAINTAINERS.txt) file.
