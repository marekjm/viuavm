# Signals

Signals are delievered to processes asynchronously, and force the receiving
process to handle them - if the process can handle the signal a new stack is
created for the handler and the execution switches to it; otherwise the process'
stack is unwound and the process is killed (exactly what would happen on an
unhandled exception).

There are three instrutions used to handle signals:

- `send_signal`: used to send signals, with signature `(Pid, Signal, Value)`
- `trap_signal`: used to trap signals, with signature `(Signal, Fn)`
- `mask_signal`: used to set process' signal mask, e.g block and unblock
  signals, with signature `(op, [Signal]) -> [Signal]` where `op` is `block`,
  `unblock`, or `set`, and the return value is the previous value of the
  process' signal mask

For example:

    send_signal %child_pid local 'ping' void
    trap_signal 'ping' handle_ping/1
    mask_signal void 'block' %signal_set local

By convention, a process responds to a trapped signal immediately. Signals are
used to implement a channel for "important messages" (e.g. for responsiveness
checks). Nothing prevents a process that received a signal to convert it to a
normal message and handle it at a more convenient time, though.

Signal delivery is paused when a process is executing a signal handler. If any
new signal arrives during this time it is put on a signal queue.
