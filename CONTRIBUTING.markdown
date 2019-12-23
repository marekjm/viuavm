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

Other C++17 capable compilers may work but testing is only performed for G++ and Clang++.

The development environment is also assumed to have the usual \*NIX tools (awk, sed, xargs, etc.) available.


### Operating system

Testing is regularly performed only on Linux.
Compilation on on various BSD distributions should also work.
Compilation on other operating systems is not tested, but contributors with
access to Microsoft, Apple, or other exotic operating systems are welcome to try
and compile the code to see if it runs.


### Typical "from clone to first run"

```
$ git clone --branch devel --recursive git@github.com:marekjm/viuavm.git
$ cd viuavm/
$ make -j 4
$ make test     # this assumes that you have Python 3.x and Valgrind installed
```

If the above sequence of commands can be run without problems on your system then
you're all set.

*Note:* the last command - `make test` - may run for several minutes, depending on the speed of your hardware.


### Development scripts

In the `scripts/` directory, you can find scripts that are used during development of Viua.

----


## Git workflow

Each feature and fix is developed in a separate branch.
Bugs which are discovered during development of a certain feature, may be fixed
in the same branch as their parent issue. This is also true for small features.

**Branch structure:**

- `master`: master branch - contains stable, working version of VM code
- `devel`: development branch - all fixes and features are first merged here
- `issue-{hash}-{slug}` or `issue-{slug}`: for issue branches

Issues are tracked using the `issue` tool: https://git.sr.ht/~maelkum/issue

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
- provide a negative test (i.e. feature generating an error) if applicable
- put comments in the code samples the tests use

In both cases your code must pass the tests on x86\_64 on Linux.

Your code must run clean under Valgrind, which implies:

- no memory leaks
- no hangs due to multithreading
- no unprotected reads/writes to memory

Remember - if your code normally runs OK, but behaves strangely under Valgrind
it's most probably a sign that there's a problem with your code.

Your code must compile warning-free under both GCC and Clang.


### Vulnerabilities

Vulnerabilities are considered bugs, and the standard way of submission applies.
An issue should be opened with a description of how to exploit the VM, and a
possible fix preventing the exploit.


### Coding standard

When submitting a patch make sure that it is formatted according to the coding
standard. It is described [here](CODING_STYLE.markdown).

----

## Suggested contributions


**Documentation**

As always, the documentation is lacking.
If you're interested in writing documentation have a go at it.


**Code samples**

> Talk is cheap, show me the code.

The VM will be no good, if no code runs on it. If you have written something
that runs on the VM (a short program solving your problem, or a Rosetta Code
task) submit a pull request.  Your sample should contain appropriate copyright
notice pointing to you, and be published under GNU GPL v3 or any later version
of the GPL.


**Overall hardening**

If you can set-up Coverity (I seem unable to), it would be a great help.
Otherwise, if you can spot memory access bugs, multithreading bugs, use ASan,
TSan, or other sanitisers - don't hesitate and attack the VM as hard as you can.
After all, the more bugs you find - the more bugs will be fixed.
