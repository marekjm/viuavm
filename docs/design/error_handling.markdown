# Error handling and error categories

Exceptions are the default mode of error handling that Viua VM suggest programs
should adopt. They are the mechanism that kicks in when an error condition
occurs during program execution and it would be unwise to continue without
taking action to amend the situation.

There are two error "categories":

- soft errors: they can be caught by a process
- hard errors: they cannot be caught by a process

An example of a soft error is a division by zero, or out-of-bounds vector
access. These may happen and are easily corrected.

An example of a hard error is an invalid instruction. When the process tries to
execute an invalid instruction there's not much it can do as it just learned
that the correctness guarantees about the bytecode format are void. A manager
process may try to alleviate this by reloading the code module, and restarting
the process.

The process of error "evolution" consists of two phases: first, in which the
error is a "fault" and may be corrected early; and second, in which the error
becomes an "exception" and causes the process' stack to be unwound.

A fault is handled transparently - the executing process may not even notice
that the handler it set for a fault was used:

    .function: handle_zero_division/1
        ; on division by zero just return 0
        allocate_registers %1 local
        izero %0 local
        return
    .end

    .function: some_other_function/2
        ; ...

        service_fault 'Zero_division_error' handle_zero_division/1
        div %result local %numerator local %denominator local

        ; ...
        return
    .end

In `some_other_function/2` the division may fail but the fault will be handled
by the `handle_zero_division/1` service function. If the service function was
not registered the fault would be promoted to an exception and propagated up the
stack.
