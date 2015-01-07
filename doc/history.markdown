# History of Wudoo VM

This file aims to be a kind of development diary, for things I'd like to save for later or
things I'd like to remember - because I know I will forget - or just to write down what I have learned
during development of the VM.


----

## Versions and name changes

- `0.1.0`: **Wudoo VM**
- `0.0.1`: **Tatanka VM**


----

## History

### Virtual Destructors

*When*: 2014.01.05

When creating polymorphic classes remember to make the destructor virtual!
This is one freakin' good idea because otherwise you can have memory leaks and
they are not a good thing to have, in any way.

Also, destorying polymorphic objects with non-virtual destructors leads to undefined behaviour and
software behaving in an undefined manner is not something we like (unless it is written to behave in
an undefined manner).

So, lesson for today: use `virtual` on your destructors when tinkering with polymorphism.
And, as a reminder, compile with `-Wall -pedantic` once in a while to catch errors you missed earlier.
