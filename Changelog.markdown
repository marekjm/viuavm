# Viua VM changelog

This file documents version-to-version changes in the machine; made to
both internal and external APIs, and to the assembly language closest to
machine's core.

Changes documented here are not the fullest representation of all changes.
For that you have to reference Git commit log.


----

### Change categories

There are several categories of change:

- **fix**: reserved for bugfixes
- **enhancement**: reserved for backward-compatible enhancements to machine's
  core (e.g. speed improvement, better memory management etc.)
- **bic**: an acronym for *Backwards Incompatible Change*, these changes (possibly
  also enhancements) are backwards incompatible and
  may break software written in machine's assembly or using it's public APIs,
  BICs in internal APIs may not be announced here
- **feature**: resereved for new features implemented in the VM, e.g. a new
  instruction, or a new feature in a standard library
- **misc**: various other changes not really fitting into any other category,


----


# From 0.7.0 to 0.7.1

Better support for closures: users can control what registers to enclose and
how to enclose values in them.
Vector and object modifications have move semantics now (copy semantics must be
implemented in user code).

- feature: `ENCLOSECOPY` instruction for enclosing objects in closures by copying them,
- feature: `ENCLOSEMOVE` instruction for enclosing objects in closures by moving them
  inside the closure,
- bic: renamed `CLBIND` to `ENCLOSE`,
- bic: changed the way closures are created, i.e. now, the closure is created first and
  only then objects are enclosed (either by copy, by move or by reference),
- bic: renamed `THREAD` to `PROCESS`,
- bic: vector instructions `VINSERT`, `VPUSH` and `VPOP` have move semantics,
- bic: removed `Object::set` and `Object::get` methods from `Object`'s prototype (object
  modification instructions `INSERT` and `REMOVE` must be used instead),
- feature: `INSERT` and `REMOVE` instructions with move semantics for object modification,
- fix: `VPOP` allows popping all indexes, not only the last one,
- fix: `std::functional::apply` works correctly for functions that do not return anything,
- bic: renamed `THJOIN` to `JOIN` to reflect the name change from *threads* to *processes*,
- bic: renamed `THRECEIVE` to `RECEIVE` to reflect the name change from *threads* to *processes*,


----


# From 0.6.1 to 0.7.0

- bic: `throw` instruction no longer leaves thrown object in its source register;
  thrown object is put in special *throw-register* and the source register in
  currently used instruction set is made empty
- bic: `free` instruction was renamed to `delete` to better match the naming convections
  of the other object-lifespan controlling instructions (i.e. the `new` instruction)
- enhancement: change `Thread::instruction_counter`'s type from `unsigned` to `uint64_t`
- fix: stack traces displayed after uncaught exceptions are generated for the thread that
  the exception originated from
- bic: machine reports the function that started a thread as orphaning thread's children,
  this means that old "main/1 orphaning threads" changes to "__entry/0 orphaning trhreads"
- misc: if stack is not available "<unavailable> (stack empty)" will be used when
  machine reports that threads were orphaned,
- enhancement: returning objects from functions has "move semantics" - there is no copy
  operation and the object is just moved to the caller frame,
- bic: `String::stringify()` foreign method takes pointer to an object as its second argument,
- bic: `String::represent()` foreign method takes pointer to an object as its second argument,
- bic: `std::string::stringify` takes pointer to object to stringify instead of a reference,
  this makes the objects stay/1 within VM's scope-based memory management system,
- bic: `std::string::represent/1` takes pointer to object to stringify instead of a reference,
- bic: `end` instruction renamed to `return`,
- feature: VM provides a mechanism to spawn an immortal watchdog thread to deal with deaths of
  other threads (the syntax is `watchdog <function-name>`, `watchdog` instruction requires a frame),
- enhancement: VM provides a mechanism to extract return values from functions running in threads,
- bic: syntax of `thjoin` instruction changed from `thjoin <thread-handle>`
  to `thjoin <target-register> <thread-handle>`,
- enhancement: slightly better messages for some exceptions,
- feature: threads can be suspended and woken-up,
- feature: `pamv` instruction added to instruction set, supports pass-by-move,
