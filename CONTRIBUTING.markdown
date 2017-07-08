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
Appropriate interface for reporting is the Github issues page.

Rememeber to provide appropriate test cases with code patches you submit.
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


### Vulnerabilities

Vulnerabilities are considered bugs, and the standard way of submission applies.
A GitHub issue should be opened with a description of how to exploit the VM, and
a possible fix preventing the exploit.


### Coding standard

When submitting a patch make sure that it is formatted according to the coding standard:

- long, descriptive variable names are an accepted tradeoff for shorter lines (it's better to give variables hilariously descriptive names than
  to wonder "WTF does this code do" some time later)
- opening braces on the same line as their ifs, whiles, class and function signatures, etc.
- indent by four spaces

An appropriate `.clang-format` file is included in the repository.
Use it to format your code before commiting.
Documentation for `clang-format`: https://clang.llvm.org/docs/ClangFormat.html


----

## Suggested contributions


**Documentation**

As always, the documentation is lacking.
If you're interested in writing documentation have a go at [docs repository](https://github.com/marekjm/docs.viuavm.org).


**Code samples**

> Talk is cheap, show me the code.

The VM will be no good, if no code runs on it.
If you have written something that runs on the VM (a short program solving your problem, or
a Rosetta Code task) submit a pull request.
Your sample should contain appropriate copyright notice pointing to you, and
be published under GNU GPL v3 or any later version of the GPL.


**Data structures**

Viua uses only the C++ standard library data structures.
However, if you are capable of writing efficient, reliable, concurrent data structures feel welcome
to share your knowlegde and experience and enhance the VM.

Most sought-after would be:

- multiple-producer single-consumer queue (for message queues),
- data structure for mapping types to their ancestors (e.g. map from string to vector of strings),
- multi-version map (for storing multiple versions of loaded bytecode modules for code hot-swapping)

If you have other ideas, feel free to suggest them.

Note that *all* data structures used by the VM kernel *must* support parallel access so either use locks, or
atomic operations but ensure that your data structure is safe to use in parallel environment.


**Bytecode format**

Currently, debugging information embedded in bytecode is limited.
If you have an idea how to embed debugging symbols in bytecode, and
want to have a go at implementing such a functionality feel free to.


**Overall hardening**

If you can set-up Coverity (I seem unable to), it would be a great help.
Otherwise, if you can spot memory access bugs, multithreading bugs, use ASan, TSan, or
other sanitisers - don't hesitate and attack the VM as hard as you can.
After all, the more bugs you find - the more bugs will be fixed.
