# Building instructions

The compilation process is automated using GNU Make, and in its simples form
looks like this:

    ]$ make -j

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
