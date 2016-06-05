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


----

## 0.8.5

- the `viua-vm` frontend exposes `--info` option reporting VM process scheduler and
  FFI worker thread statistics (how many there are, if the number is fixed or dynamic, etc.)
- standard VM library includes `std::vm::info/0` function that provides the same information
  to programs as `viua-vm --info` provides to users,


----

# 0.9.0

- stackful coroutines, with coroutines being implemented as processes;
  coroutines would provide a "synchronised concurrency" model


----

# 0.9.1

- process and coroutine related enhancements to standard VM library,
- improvements to error handling (stress-testing the VM),
