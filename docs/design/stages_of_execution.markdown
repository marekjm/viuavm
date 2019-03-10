# Stages of execution

Viua VM programs are composed of many processes running concurrently. Each of
these processes is internally sequential, and at the lowest level runs as a
linear stream of simple instructions.

When a physical CPU executes an instruction it goes through several stages. For
example:

- read: a word containing the instruction is read from memory
- decode: the instruction is decoded
- fetch: the CPU fetches the operands for the instruction
- execute: the CPU executes the instruction
- retire: the CPU stores the result of the instruction

This is repeated ad infinitum until the CPU is shut down, either by power loss
or as a planned poweroff. Somewhere in this sequence there are moments when
interrupts are delievered, when exceptions are reported, etc. so the apparent
simplicty of the simple read-execute-store loop becomes lost when you look at
what is *actually* happening.

Viua VM needs a similar model describing the "lifecycle" of a single instruction
execution.

--------------------------------------------------------------------------------

# Stages of execution in Viua VM

This is the sequence of instruction execution phases used by Viua VM:

- read: the instruction's opcode is read from memory
- fetch: the instruction's operands are loaded from registers (or from immediate
  fields)
- execute: the instruction's logic is executed by the VM
- retire: the instruction's result is stored in a register (if the instruction
  produces a value)

Interrupts are delievered in-between instruction execution (after the "retire"
phase ends, and before the "read" phase of the next instruction begins).
Interrupts are events or conditions that the process is forced to handle and
which originate "from the outside" (i.e. when the process is sent a signal by
another process). They are somewhat similar to exceptions, but exceptions
originate "from the inside" of the process (i.e. when the process tries to
execute a division by zero).

After each phase there is a hidden phase that checks for errors that may have
occured during the previous phase, so the full list is:

- read
- read-verify: throw an exception if an error occured during "read" phase
- fetch
- fetch-verify: throw an exception if an error occured during "fetch" phase
- execute
- execute-verify: throw an exception if an error occured during "execute" phase
- retire
- retire-verify: throw an exception if an error occured during "retire" phase

Most instructions try hard to be atomic, so that if an error occurs during any
phase of an execution the state of registers is not modified. This is not always
possible. The guarantees provided by each instruction are listed in that
instruction's specification.
