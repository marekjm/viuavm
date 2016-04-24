# Example foreign library

The `contrib/` subdirectory in main Viua VM source code repository is a place when
foreign libraries should be cloned.
This way they can use `-I../../include` to include Viua header files, and
find necessary object files in `../../build/platform/`.

----

## Description

This foreign library is provided as a simple bootstrap project to base other foreign
libraries on.
