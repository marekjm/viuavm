# Viua VM contributor guidelines

So, you want to help out with development, or just see what the VM can do.
Either way - that's great.
This document will get you started.

----

## Development setup

Here you can find information about development setup required to compile, test, and
develop the VM.


### Tools

* `g++`: GNU Compiler Collection's C++ compiler version 6.0 or later
* `clang++`: clang C++ compiler version 3.9.1 or later,
* `python`: Python programming language 3.x for test suite (optional),
* `valgrind`: for memory leak testing (optional; by default enabled, disabling required setting `MEMORY_LEAK_CHECKS_ENABLE` variable in `tests/tests.py` to `False`),

Other C++14 capable compilers may work but testing is only performed for G++ and Clang++.

The development environment is also assumed to have the usual \*NIX tools (awk, sed, xargs, etc.) available.


### Operating system

Testing is regularly performed only on Linux.
Compilation on on various BSD distributions should also work.
Compilation on other operating systems is not tested, but contributors with access to
Microsoft, Apple, or other exotic operating systems are welcome to try and compile the code, to see if it runs.


### Typical "from clone to first run"

```
$ git clone --branch devel --recursive git@github.com:marekjm/viuavm.git
$ cd viuavm/
$ make -j 4
$ make test     # this assumes that you have Python 3.x and Valgrind installed
```

If the above sequence of commands can be run without problems on your system then
you're all set.


### Development scripts

In the `scripts/` directory, you can find scripts that are used during development of Viua.
The shell installed in dev environment is ZSH but the scripts should be compatible with BASH as well.

----


## Git workflow

Each feature and fix is developed in a separate branch.
Bugs which are discovered during development of a certain feature,
may be fixed in the same branch as their parent issue.
This is also true for small features.

**Branch structure:**

- `master`: master branch - contains stable, working version of VM code,
- `devel`: development branch - all fixes and features are first merged here,
- `issue/<number>/<slug>` or `issue/<slug>`: for issues (both enhancement and bug fixes),

----


## Patch submissions

Patch submissions and issue reports are both welcome.

Rememeber, though, to provide appropriate test cases with code patches you submit.
It will be appreciated and will make the merge process faster.
If you're fixing a bug:

- write a test that will catch the bug should it resurface
- put comments in the code sample your test uses

If you're implementing a feature:

- provide a positive test (i.e. feature working as intended and performing its function)
- provide a negative test (i.e. feature generating an error)
- put comments in the code samples the tests use

In both cases your code must pass the tests on both x86\_64 and 64 bit ARM (if you don't have an ARM CPU around
mention it when submitting a pull request and I'll test on ARM myself).
Your code must also run clean under Valgrind, which implies:

- no memory leaks
- no hangs due to multithreading
- no unprotected reads/writes to memory

Remember - if your code normally runs OK, but behaves strangely under Valgrind it's most
probably a sign that there's a problem with your code.

When submitting a patch make sure that it is formatted according to the coding standard:

- long, descriptive variable names are an accepted tradeoff for shorter lines (it's better to give variables hilariously descriptive names than
  to wonder "WTF does this code do" some time later)
- opening braces on the same line as their ifs, whiles, class and function signatures, etc.
- indent by four spaces

An appropriate `.clang-format` file is included in the repository.
Use it to format your code before commiting.
Documentation for `clang-format`: https://clang.llvm.org/docs/ClangFormat.html
