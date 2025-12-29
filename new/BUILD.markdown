# Building instructions

The compilation process is automated using GNU Make, and in its simplest form
looks like this:

    ]$ make -j

--------------------------------------------------------------------------------

## System requirements

Before you can compile the toolchain you have to ensure that your system meets a
few requirements.


### Compiler

Both GCC and Clang can be used to compile the toolchain, as long as they
implement C++23.


### Operating system

The following are supported:

 - Linux/glibc
 - Linux/musl
 - FreeBSD
 - OpenBSD


### Libraries

Common libraries:

 - libb2: https://blake2.net/
 - libblake3: https://github.com/BLAKE3-team/BLAKE3
 - libmd: https://www.hadrons.org/software/libmd/
 - libuuid: https://github.com/util-linux/util-linux

Linux libraries:

 - liburing: https://git.kernel.dk/cgit/liburing (default, but not necessary if
   you decide to use the "classic" I/O implementation)

There are no special library requirements for FreeBSD or OpenBSD.

OpenBSD requires manual compilation and installation of the b2sum tool before
the VM can be compiled.

--------------------------------------------------------------------------------

## I/O implementations

The VM can use two APIs to implement I/O:

 - classic POSIX I/O
 - `io_uring` (default on Linux, unavailable on FreeBSD)

The build will automatically select the implementation for the current platform,
but you can override the default choice with the `io_impl` flag:

    ]$ make io_impl=classic -j
    ]$ make io_impl=io_uring -j

--------------------------------------------------------------------------------

## Build presets

There are several presets:

 - `debug`: a build optimised for debugging, with sanitisers disabled to avoid
   them interfering with GDB or Valgrind
 - `default`: an unoptimised build, with sanitisers and plenty of compiler
   warnings enabled
 - `release`: an optimised build, with sanitisers disabled
 - `test`: a build with basic optimisations enabled, and with sanitisers also
   enabled

The presets are defined in `Makefile.d/Preset/` directory.

How to choose a preset?

    ]$ make PRESET=preset -j

The `default` preset is intended for day to day development work and manual
testing; the `test` preset should be used in CI environments.
Use the `debug` preset when inspecting problems with the VM.
The `release` preset is intended to be used for deployment and end-user
installations.

--------------------------------------------------------------------------------

## Tweaking flags

Various aspects of the build can be tweaked.
However, it is recommended to use presets unless you know what you are doing.

### C++ compiler and standard

    ]$ make CXX=... CXXSTD=...

### Sanitisers

    ]$ make CXXFLAGS_SANITISER=...

### Compiler options

    ]$ make CXXFLAGS_OPTION=...

### Compiler warnings

    ]$ make CXXFLAGS_NOERROR=... CXXFLAGS_WARNING=...

The `CXXFLAGS_WARNING` variable sets the list of enabled compiler warnings --
which will be treated as errors, since the build *always* runs with the
`-Werror` flag enabled.

The `CXXFLAGS_NOERROR` variable sets the list of warnings which are not treated
as errors.

### Optimisation level

    ]$ make CXXFLAGS_OPTIMISATION=...

### Debug symbols

    ]$ make CXXFLAGS_DEBUG=...
