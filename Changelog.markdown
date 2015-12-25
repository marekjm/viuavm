# Viua VM changelog

This file documents version-to-version changes in the machine; made to
both internal and external APIs, and to the assembly language closest to
machine's core.

Changes documented here are not the fullest representation of all changes.
For that you have to reference Git commit log.


----

### Change catrgories

There are several categories of change:

- **fix**: reserved for bugfixes
- **enhancement**: reserved for backward-compatible enhancements to machine's
  core (e.g. speed improvement, better memory management etc.)
- **bic**: an acronym for *Backwards Incomparible Change*, these changes (possibly
  also enhancements) are backwards incompatible and
  may break software written in machine's assembly or using it's public APIs,
  BICs in internal APIs may not be announced here
- **feature**: resereved for new features implemented in the VM, e.g. a new
  instruction, or a new feature in a standard library
- **misc**: various other changes not really fitting into any other category,


----


# From 0.6.1 to 0.6.2

- bic: `throw` instruction no longer leaves thrown object in its source register;
  thrown object is put in special *throw-register* and the source register in
  currently used instruction set is made empty
- bic: `free` instruction was renamed to `delete` to better match the naming convections
  of the other object-lifespan controlling instructions (i.e. the `new` instruction)
- enhancement: change `Thread::instruction_counter`'s type from `unsigned` to `uint64_t`
- fix: stack traces displayed after uncaught exceptions are generated for the thread that
  the exception originated from
- bic: machine prevents overwriting registers that were passed by value; this is due to the
  fact that calls employ lazy parameter passing - the value is only copied when requested by
  the callee so if during the time between putting an object in parameter slot and
  actually calling the function a register has been overwritten (causing the old object residing
  in the register to be deleted) called function would try to copy an object that does not
  exist anymore resulting in segmentation fault,
