# Registers in Viua VM

Conceptually, values are stored in registers. From the point of view of a
programmer interacting with the VM using its ISA all values are stored in
registers.

Technically, that is not true as registers are only wide enough to hold values
of some primitive types eg, integers or floats. Values of other, "bigger" types
are stored on the heap and only pointers to them are stored in the registers.
Viua presents value semantics at all times, so even if complex values are
represented as heap-allocated pointers they are visible on ISA level as simple
values - not references.

--------------------------------------------------------------------------------

## High-level overview

Instructions access registers for different reasons:

- to read a value
- to mutate a value
- to store a value

### Access to read

The first case, reading a value, is trivial. It does not matter whether the
access is direct or through a reference - the value can be easily obtained.

This access mode is only considered for input operands.

### Access to mutate

The second case, mutating a value, is equally easy. A reference to a value can
easily be produced whether by directly accessing the register, or through a
reference.

This access mode is considered for output and input-output operands.

Mutation can be tricky. For example, the following code may be ambiguous:

    addi $1, 42
    addi $2, 42
    add $1, $1, $2

Does the ADD instruction *overwrite* (ie, access the register) or *mutate* (ie,
access the cell) the value present in register $1 just prior to the
instruction's result being retired?

In Viua the answer is that the above instruction *overwrites* the value if the
access is direct. It only *mutates* through references:

    addi $1, 42
    addi $2, 42
    ref $3, $1
    add *3, $1, $2

Note that the above code would not work if the reference in register $3
referenced value of type other than the type of value in register $1. Mutation
is strict about types.

### Access to store

The third case is more difficult, though. Consider the following code again:

    addi $1, 42
    addi $2, 42
    ref $3, $1
    add *3, $1, $2

This access mode is considered for output.

The out operand (ie, the `*3`) cannot be used to store a value because it is not
generic. The value behind the reference is mutated (one *could* say that
whatever is sitting behind the reference is overwritten but that would not be
entirely true; only the referenced object's value would be overwritten, not the
object itself).

Accessing to store is a generic operation by virtue of its destructiveness.
Whatever is occupying the register that is stored to is destroyed before the new
object may be stored.

### Edge case

The above rules produce a curious edge case:

    li $1, 42
    li $2, 69
    ref $3, $1
    add $1, $2, *3

What happens after the ADD instruction with the integere in register $1? Is it
overwritten? Is it mutated?

If it would be overwritten (as the "direct access means overwriting" rule would
dictate) the reference in register $3 is now dangling, and this is no good. The
static analyser should catch such cases and cause the assembler to reject this
code. However, this may not be that bad if the reference is never used
afterwards.

If the integer would be mutated to keep the reference in register $3 valid then
the rules would be broken as direct access would *mutate* instead of
*overwrite* a register. This would mean that taking a reference to a value
silently changes rules of access when a register contains such a "referenced"
value vs when it contains an "unreferenced" value.

The issue is thorny, as no matter how we look at it the rules need to be broken
or badly bent out of shape. Unless, maybe, the meaning of "mutation" and
"overwriting" is defined differently.

If we stick to the basic rules laid out earlier, then the egde case code will
produce a dangling reference. And everything seems to fine, but... if references
point to values and not locations (they are NOT pointers), then why overwriting
a register destroys a value? Because the storage occupied by the object which
represented the value was reused. This makes sense, I think.

If we wanted the edge case code to mutate the value instead of overwriting it we
would have to define the direct access in output operands the following way:

> Direct access in output operands overwrites the value, unless the value TO BE
> stored is of type compatible with the value that IS stored in the register. In
> the latter case, the value TO BE stored is converted to the type of the value
> that IS stored and is used to mutate the object stored in the register.

This definition is... unwieldy and contains the uncomfortable bit about
"compatible type". Without this compatibility clause the definition would be
simpler ie, the object would only be mutated if it was of the same type. But
then i8 would not be possible to write into i64 what is ridiculous. Or is it?

To sum up, I think it would be best to keep the rules simple ie, that DIRECT
ACCESS in output operands IS DESTRUCTIVE and ALWAYS overwrites the contents of a
register; and that MUTATION in output ONLY happens VIA REFERENCES.

Rules for mutation via input operands do not have to be changed, adjusted, or
redefined. Input operands always produce a value anyway so there is no
ambiguity.

--------------------------------------------------------------------------------

## Terminology

Some terms may be ambiguous, or may need to be explained. The following glossary
should be helpful.

- type: a type is an instruction about the correct interpretation of an object
- object: an object is a concrete "thing" (of a particular type) allocated in
  memory
- value: a value is the meaning of the bytes of which an object is composed

The above means that i64 is a *type* - it tells the VM (and programmers) how to
create integer *objects* and how to interpret *values* of these objects.

- cell: a thing that holds objects
- register: a slot for a cell

Direct register access produces *cells*. This is why it can be used to overwrite
any kind of value. Reference access produces *objects* which is why references
can only be used to mutate objects (or, overwrite values).
