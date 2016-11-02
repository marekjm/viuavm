> I’m strugling a bit to see why you would want to do that in the first place.
> Do you have a more real-world use case in your VM-code somewhere?

Let me explain the "why", then, with the real-world use case from Viua's code.
The introduction may be a bit lengthy but I will eventually get to the point, I promise.

As you may remember, the machine is register based, meaning I store instruction operands not on a stack, but
in a set of general purpose registers.
Also, the is not one but many register sets (a minimum of 3 for each run of the machine: a global register set, and
local sets for `__entry` and `main` functions).
Each register in a set is capable of storing every object the machine can possibly create.

Now, how can pointers to various types be stored in a single container (register sets are treated as simple containers)?
C++ has been chosen as the implementation language for the machine so the answer is "polymorphism", a technique that
allows storing pointers to derived classes in pointers to their base class (this is **the** feature of polymorphism in
context of storage).
What is important, the real type of an object is not lost once it is stored in a register, under a pointer to a
base class - it can be retrieved by a call to a type-returning method (say, `std::string type() const`) so the type
information can be accessed by the machine at runtime.
Also, the dynamic dispatch of `virtual` member functions in C++ means that the correct version (i.e. one defined in the
derived class) of the `std::string type() const` method will be called, so even if the `std::string type() const` method
is called via a `Base*` the code that will run will be one defined in the `Derived` class.

Polymorphism is a powerful construct and can be creatively exploited, should a need arise.
And, in case of Viua, it did.

Machine's runtime provides a few basic types:

- numeric `Integer` and `Float`,
- `Boolean` used in conditional instructions,
- `String` for storing text,
- `Vector` for storing sequences of objects,
- `Object` representing objects,
- `Function` and `Closure` representing callables.

They all inherit from common base abstract type `Type` which defines a common interface all types must support.
Register sets store pointers to `Type`.

Nearly all instructions operate on objects, notable exceptions being `halt`, `end` and `nop`;
overwhelming majority manipulates them (some instructions, e.g. `move`, `copy`, `free`, `null`, `param` and `arg` only
in context of register-oriented tasks, though) and
many map directly to a method implemented in the underlying C++ classes, e.g. `iinc` instruction only calls
`increment()` on its operand, and `vpush` only calls `push()` on the `Vector` object.
In every instruction implementing member function in machine's CPU a cast from `Type*` to the desired type is performed,
e.g. `static_cast<Integer*>(object)` for `iinc` instruction.
There is no need for runtime-checked `dynamic_cast<>` because compiler will ensure type safety.

There is one obvious downside to this architecture - *every* method needs it's own instruction, or
if not a dedicated instruction then a wrapper function in standard runtime library.
You can probably imagine the amount of repeated, boilerplate code that would be needed to implements this.
Some features are indeed exposed by standard runtime library functions written in C++ that are called by code written
in machine's asm language (e.g. the `typesystem` module).
These functions are registered inside the machine as *foreign* but their use is transparent to the user code (machine
only internally distinguishes between a *foreign* and a *native* function).
Modules exposing completely external libraries (`libnotify` and `libgearman`) have been written, and
successfully used from code written in Viua's assembly, so the foreign function model is proved to be viable.

However, it makes little sense with relation to objects.
Why would a separate module be needed and functions called to access functionality of an *object*?
After all, the whole thing about object-orientation is the encapsulation of data *and* logic in a single unit - a
message is passed to the object, instead of the object being passed to a function.

```
; non object-oriented way of concatenating strings
; passing two strings to a function
strstore 1 "Hello "
strstore 2 "World!"

frame ^[(param 0 1) (param 1 2)]
call 3 concatenate

; will print "Hello World!"
print 3


; object-oriented way
strstore 1 "Hello "
strstore 2 "World!"

; the frame still must be created, first parameter is `this`
frame ^[(param 0 1) (param 1 2)]

; send a message "concatenate" to object in r1, and store the response in r3
msg 3 1 concatenate

; this will print "Hello World!"
print 3
```

In the code above, first two objects are passed to a function, then an object is sent a message.
Machine stores prototypes for each class.
A prototype is a type description, specifying for example what messages it accepts and to what functions they are
dispatched.

The key issue is - **how to register and expose prototypes written in machine's implementation language in a way
transparent to user code**?

One way is to provide functions for creating and manipulating the objects.
It is simple and good enough.

There is another way, though.
Prototypes are used by machine's runtime to determine whether the user code sends valid messages to objects and
that's it, they are like maps of what functions messages resolve to.
Below is a brief description of how method dispatch works:

1. an object is being sent a message,
2. if the message is valid for given type, determine to what function it resolves and go to 6,
3. if the message is invalid for given type, linearize the inheritance chain and check if a method is valid for any
   base type; if it is, resolve its name and go to 6; otherwise go to 5,
5. throw an exception about message not being accepted by a type,
6. if the function is undefined, throw an exception,
7. call a function specified by method resolution with frame given for the message,

After a message is validated, the function it resolved to must be called.
A function can be of three distinct types: it can be a *native function* (i.e. written in machine's assembly), a
foreign function (i.e. written in C++ and registered via foreign function interface), or
a *foreign method*.
This third type is of interest to us now.

Foreign methods are pointers to member functions of foreign prototypes, which are all classes written in C++ that can
be used by code running inside the VM (by this definition, the basic types machine provides are *also* considered
foreign).
This is where the real-world use case for *storing a pointer to member function of derived type in a pointer to member
of base class* appears.
Foreign methods have special signature and must conform to

```
typedef Type* (Type::*ForeignMethodMemberPointer)(Frame*, viua::kernel::RegisterSet*, viua::kernel::RegisterSet*)
```

Apart from this, they **must** also be declared `virtual` to make compiler dispatch them at runtime via a `vtable`.
The dynamic dispatch part is **crucial**.

Then, pointers to such members are stored in a `std::map<std::string, ForeignMethodMemberPointer>` so a
string-to-member-pointer mapping is available to the machine (so it can both determine whether a function is a foreign
method, and know how to call it).
Then, following code is used to apply the method to the object:

```
// foreign_methods is the map with pointers-to-members and call_name is the name of a function to call
(object->*foreign_methods.at(call_name))(frame, static_registers, global_registers);
```

After a refactoring to use `std::function` the calling code looks like this (it is more readable than the old version,
but the idea is the same):

```
// foreign_methods.at(...) returns a std::function
foreign_methods.at(call_name)(object, frame, static_registers, global_registers);
```

This is technique exploits two aspects of C++:

- dynamic dispatch to apply the method at runtime,
- polymorphism to store pointers to members of derived class in pointers to members of base class,

----

This looks like dark voodoo-magic but works and can be explained, even if it raises your eyebrows.
In fact, you are not the first person to ask me *Why would you do that?* or *Does this even work?*.

---

#### Copyright (C) 2015, 2016 Marek Marecki

This documentation file is part of Viua VM, and
published under GNU GPL v3 or later.

You should have received a copy of the GNU General Public License
along with Viua VM.  If not, see <http://www.gnu.org/licenses/>
