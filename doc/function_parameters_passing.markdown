# Viua VM pointers

Viua VM supports three ways of passing an object as function's parameter;
it can pass either by value, or by reference, or by pointer.
Each way will be discussed below along with its pros and cons.

Note that special rules apply when passing parameters to new threads; for
example every parameter is immediately copied when the thread is launched, and
passing by reference or pointer is not allowed *yet* to maintain full
separation of thread memory spaces.
In theory, passing by reference or by pointer should not prove dangerous since
both pointers and references are checked (meaning you can write code that will
handle expired pointers in clean and elegant way).


----


## Passing by value

First, an example:

```
.function: example_function
    print (strstore 1 "Hello World from example function!")
    echo (strstore 1 "First parameter: ")
    echo (arg 2 0)
    end
.end

.function: main
    frame ^[(param 0 (istore 1 42)) (param 1 (istore 2 64))]
    call example_function

    izero 0
    end
.end
```

The value 42 (which is an `Integer`) is passed to the `example_function/2` by value.
Control flow can be easily explained (if the syntax is too alien):

- `main/1` function creates a frame for a call,
- `main/1` function puts `42` in first parameter slot in the frame,
- control is passed from `main/1` to `example_function/2` using the `call` instruction,
- `example_function/2` prints its message and returns,
- `main/1` exits,

Function calls are synchronous which allow for a simple optimisation - the parameter is
not copied when the function is entered with `call` instruction but only when the
`example_function/2` asks for it (on line four with `arg 2 0` instruction).

The second argument to `example_function/2` is never copied; only the pointer to it is stored in
the call frame.

This however, leaves a bug that could lead to crashes if it were not fixed.
Whan would happen if the object was deleted between time it was passed as a parameter and
the time the frame was actually used to call a function?
Operating system would report segmentation violation and shut down the VM.
However, the machine marks registers that contain objects that were passed as parameters and
will prevent their deletion (but only until the soon-to-be-called function exits).

Such behaviour means, though, that every time the function extracts the argument (the `arg`
instruction may be called many times) the VM must create new copy of the parameter.

Pass by value is the simplest form of passing arguments to functions.


----


## Passing by reference

Pass by reference provides means of passing an object to the function in such a way that
modifications of such object are visible outside the called function.

For example, passing an integer by value and assigning new value to it in the called function
will not modify the value of an integer that was "passed" to the function.
Passing an integer by reference and assigning new value to it will make the new value visible
outside the call.

The bad news is that references are pretty invasive.
They live *outside* Viua's memory management and do not honor the normal rules for allocation and
deallocation.
Instead of having frame-scope (i.e. being deleted when their frame is popped from stack) objects
managed by references are deleted when the last reference to them is deleted.
As such, they exist *outside* of normal memory management scheme in Viua.

Despite their invasive nature, references are used extensively where the function **must** operate
on *the same* object as the calling function, or when object lifetime would not fit into frame-based
model (e.g. in closures).
References themselves are normal objects (even if their use is transparent to the user) and
their lifetimes are predictable.

One object may have many references which share a single reference counter.
When last reference is deleted, the managed object is also deleted.
There is no possibility of two references to the same object having different values in the counter.
When a reference to an object is taken, the original object is taken out of its source register, wrapped
in a reference and put back - this means that the single `ref` or `paref` instruction sometimes creates
not one but *two* references.
This is done this way because an object that is managed by references must *escape* machine's supervision and
instead be fully hidden behind references.

The object managed by references is never seen *directly* by the user's code (machine's internals
modify it on the behalf of user's code); the references themselves are also invisible to user's code.

When a user sends a message to an object with `msg` instruction, machine will unpack the object from
the reference automatically.
When an integer wrapped in a reference is incremented with `iinc` instruction machine will unpack the
object from the reference automatically.

However, once an object is wrapped in a reference there is no way the user's code will ever touch
it directly.
This simplifies both machine's and user's code.


----


## Passing by pointer

Pass by pointer is the third way of passing arguments to a function.
Pointers are more lightweight than references as they do not take the object out of the normal
management scheme (i.e. frame scope) - they only provide alternative way of accessing it.

Pointers, contrary to references, can *expire*.
When a pointer is expired it means that it points to an object that no longer exists.

Using pointers is semi-transparent.
When sending a message to an object via a pointer the user does not have to unpack the object.
The pointer will carry the message to it's destination.
This is the same as with references.
However, if the object no longer exists the pointer will signal it by throwing an exception and
this small difference is what makes pointers different from exceptions - they may sometimes fail,
even if the user has means of recovering from the failure.

There is no way to unpack the pointer, i.e. get the object pointed to and put it in its own register.
This would be too bug-prone as one misused `delete` could crash the machine, consider following code:

```
.function: everythings_ok
    arg 1 0

    ; deleting local copy of the *pointer* itself
    ; the object pointed to is not modifed in any way so
    ; everything's OK
    delete 1

    end
.end

.function: oh_noes_a_bug
    arg 1 0

    ; using a hypothetical instruction extracting an object from the pointer
    unptr 2 1

    ; the frame is popped and two things happend:
    ;
    ;   1) local copy of the pointer is deleted,
    ;   2) object in register 2 is deleted,
    ;
    ; that second deletion is a bug - since the calling frame now has a pointer
    ; to an object that no longer exists
    end
.end

.function: main
    istore 1 42
    ptr 2 1

    ; here, the pass-by-value is used on the *pointer*; the object pointed to does not change
    frame ^[(param 0 2)]
    call everythings_ok

    ; prints "42"
    print 1

    ; here, the pass-by-value is used on the *pointer*; the object pointed to does not change
    frame ^[(param 0 2)]
    call oh_noes_a_bug

    ; causes segmentation fault
    print 1

    izero 0
    end
.end
```

It is currently impossible for user's code to cause a segmentation fault by using `delete` instruction.
Users can cause memory leaks by using `empty` in a way they shouldn't but they can't crash the machine even
though they are given direct access to objects' lifetime management primitives.

> Note: Viua could of course flag the *unpointered* registers and
> prevent their deletion.
> This approach will be investogited in a development branch dedicated to this issue and
> may be included in main branch of the code in the future.

Pointers will also be opaque in that the direct operation opcodes won't be usable on them, i.e. `iinc` instruction
will not increment the integer pointer to by the pointer.
Users will have to explicitly send the `increment` message to the object and let the pointer handle indirection.
Example:

```
.function: main
    istore 1 42

    ; store pointer to Integer at register 1 in register 2
    ptr 2 1

    ; first register will contain Integer(43)
    iinc 1

    ; invalid type exception thrown
    iinc 2

    izero 0
    end
.end
```


----


## Summary

This post briefly described the differences between three methods of passing arguments to functions in Viua VM;
that is, the pass-by *value*, *pointer*, and *reference*.
It also described differences between pointers and references, and how to use them in source code.
