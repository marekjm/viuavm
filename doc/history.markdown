# History of Wudoo VM

This file aims to be a kind of development diary, for things I'd like to save for later or
things I'd like to remember - because I know I will forget - or just to write down what I have learned
during development of the VM.


----

## Versions and name changes

- `0.1.0`: **Wudoo VM**
- `0.0.1`: **Tatanka VM**


----

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


----

### Names, aliases or whatever. And structs.

*When*: 2015.01.07

So, I thought it would be nice to be able to give registers names.
Just to be able to write `print result` instead of `print 6` which involves remembering that I am storing
result in register 6 - but, well, that's no longer true since register 6 is now used to store result of
some boolean operation that was needed during some steps of the program.

And thus two ideas have been born.
First one - just implement `name <number> <name>` CPU instruction.
Second one - implement `.name: <number> <name>` assembler instruction.

Looking at it now, I don't know why would anybody implement such a thing as a VM's CPU instruction.
Wudoo has `branch <cond> <true> (<false>)` where `<true>` and `<false>` are instruction indexes resolved
into bytecode addresses *by the assembler* and not inside the CPU.
Then it has `.mark: <mark>` assembler instruction and, e.g. `jump` CPU instruction which may be written as
`jump :<mark>` but the non-numerical marker will be resolved into instruction index and
further into bytecode address *by the assembler* and not inside the CPU.

But must have been sleepy and exhausted already that day (which is probably true, since I think it has
been around 1am) and the name-as-CPU-intruction thought crossed my mind.
Luckily, I got to my senses and argued myself into implementing it names as assembler instructions.


Here comes the explanation for the second part of the title - *And structs*.
When thinking about implementing `struct` like objects for Wudoo I encountered an obstacle - how to access
their fields?

I came up with the idea (well, not *I* per se, because I am sure somebody already had this idea before) that
CPU could be equipped with a set of access-instructions.
Say, `accr` for *access-read* and `accw` for *access-wite*.
Mental draft for this was this:

- `accr`: access-read would access a field in a register and store it in another register,
- `accw`: access-write would store a value held in a register in a field in another register,

As simple as that.

But how to create structs?
With yet another CPU instruction: `struct <register> <number-of-fields>`.
Say, we have a following C struct:

```
typedef struct {
    int x, y;
} Point2D;
```

In Wudoo assembly language, we would have then to write:

```
struct 1 2
.name: 1 point2d

istore 2 0
accw point2d 0 2
accw point2d 1 2
```

Now, to explain each instruction:

0.  `struct 1 2`: create a struct in register 1, and make the struct have two fields,
0.  `.name: 1 point2d`: name register 1 `point2d`
0.  `istore 2 0`: store 0 in register 2
0.  `accw point2d 0 2`: access field in register `point2d` and write to its 0-th field value which is
    in register 2,
0.  `accw point2d 1 2`: access field in register `point2d` and write to its 0-th field value which is
    in register 2,


The code above would initialise our `Point2D` struct's fields with integer zeroes.
It is fragile and weak but it is also a first iteration of the idea.
