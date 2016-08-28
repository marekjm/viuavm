# Viua VM release schedule

Each version is assigned a set of features, fixes, enhancements etc. that should be
included in it.
When their respecitive issues are closed - the version is released.
Release schedule is feature-based instead of time-based.
Releases should be small, but rather frequent.

----

## 0.8.1

- one FFI worker thread,


----

## 0.8.2

- process scheduler is extracted from CPU code, CPU uses scheduler object to
  run VM processes,


----

## 0.8.3

- several FFI worker threads,


----

## 0.8.4

- CPU uses several VM process schedulers and each scheduler runs in its own thread,
- remove `join` operation, use only message passing

When using `process <non-zero> foo/0` parent process subscribes to the child process, and
can use `join` on it.
When using `process 0 foo/0` parent process immediately disowns the child, and
can not use `join` on it (because no PID is returned to the parent).
This is done this way as it is the only safe mechanism to avoid race conditions: process-spawn and
process-subscribe operations must be atomic, or otherwise:

- if spawned process fails faster than parent process subscribes to it, the exception may never
  be forwarded to the parent
- if spawned process finished faster than parent process subscribes to it, the return value may never
  reach parent process because when the child process finishes it sees that no one is interested in its
  return value

Message multicasting must be implemented using ordinary message passing.
This "subscribe-on-spawn" semantics must be implemented in the same version that introduces SMP support
to the VM.
The atomicity of spawn and subscribe operations means that only the parent process may listen to
the return message of any process.


----

## 0.8.5

- the `viua-vm` frontend exposes `--info` option reporting VM process scheduler and
  FFI worker thread statistics (how many there are, if the number is fixed or dynamic, etc.)
- standard VM library includes `std::vm::info/0` function that provides the same information
  to programs as `viua-vm --info` provides to users,
- `process` opcode returns only PID to the process; PID lets you send and
  receive messages, but can't be used to suspend or resume a process from user code


----

# 0.9.0

- stackful coroutines, with coroutines being implemented as processes;
  coroutines would provide a "synchronised concurrency" model


----

# 0.9.1

- process and coroutine related enhancements to standard VM library,
- improvements to error handling (stress-testing the VM),
