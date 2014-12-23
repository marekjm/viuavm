# Regrefs - register references

Register reference is encoded by a byte coming just before the operand to the instruction.
If it is truthy - it means that this operand has changed meaning, in a way dependent on the instruction.
It will be explained by examples.


----

## Basic example

Basic form of print instruction is `print 1`, and it prints any value at register `1`.
However, consider following example:

```
istore 1 16
istore 2 1

print  1
print @2

halt
```

The above program would print the number 16 two times.
First time - because of the `print 1` instruction, which says *print register 1*, and
second time - because the `print @2` instruction says print a value at a register *with index taken from* register 2.

In short, the `@` operator means *take a number at this register as an operand*.
