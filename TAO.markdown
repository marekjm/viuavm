# Tao of Viua VM

Tao ("the way") of Viua VM is a set of principles that
guide the developement of, and for the machine.
They are listed here in no particular order, and
some are more of an advice than a principle or rule (not *thou shalt...* but
rather *thou shalt consider that...*).

----

### Design for failure

To err is human, but the environment does not forgive.
Provide the way to handle every error that can be corrected by the program.
Expect failure, and use provided mechanisms (watchdog process, `catch` blocks) to handle errors
instead of crashing.

----

### Do not pollute information

Separate cleanly the errors from valid return values - e.g. do not return
error codes, throw exceptions instead.
If needed, provide functions that will check if an exception would be thrown.

----

### Fail fast

Do *not* try to guess what should happen; if you're unsure, if input is invalid, if
something is missing - throw an exception crying for help.
The worst thing that can happen is a stacktrace telling you what should be fixed.
If a watchdog process is running and was set up properly, it may be possible your
process will be restarted anyway - this time with correct data.

----

### Data ownership should be obvious

That's why sharing is reduced to minimum.
It should be clear to whom each piece of data belongs.
Although it might be achieved with copying, many instructions default to move semantics.
If you need copy semantics, copy the data before the instruction moves it.
If you're passing data up the stack (to more nested calls) pass-by-pointer may also suffice.

----

### Divide tasks between processes

Viua can run many virtual processes concurrently - use this feature when applicable.

----


## Viua VM core development guidelines

A set of rules guiding development of the core VM (i.e. the actual C++ implemention).

- express ideas directly in code: use comments only when there's no way to express intent
  in code,
- write only in standard C++: do not use any platform-specific language extensions,
- prefer compile-time checks to run-time checks: what can be checked staticaly - should be,
  what cannot, must be checked at runtime,
- fail fast, and catch run-time errors early - before they propagate,
- do not leak resources: the VM must not leak any resources it manages,

These rules were extracted from Bjarne Stroustrup's talk "Writing Good C++14" from CppCon 2015.
The talk is available here: https://www.youtube.com/watch?v=1OEu9C51K2A
