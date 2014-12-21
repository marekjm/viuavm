# Tatanka VM bytecodes

Here are described all bytecodes of Tatanka VM.

----

## `istore`

**syntax**: `istore <register> <integer>`

Store integer at given register.


----

## `iadd`

**syntax**: `add <reg_a> <reg_b> <reg_c>`

Add integer at register `a` to integer in register `b`.
Store the result at register `c`.


----

## `print`

**syntax**: `print <register>`

Print value at given register.


----

## `branch`

**syntax**: `branch <addr>`

Branch to instruction at bytecode `<addr>`.


----

## `branchif`

**syntax**: `branchif <reg_cond> <addr_if_true> <addr_if_false>`

Check `Boolean` value at condition register.
If it is true - branch to `addr_if_true`, otherwise branch to `addr_if_false`.


----

## `ret`

**syntax**: `ret <register>`

Copy value of given register to register 0, to make this value return value of the function or
a program.


----

## `call`

**syntax**: `call <func_addr> <param_no> <first_param> <ret_value_register>`

Call function located at bytecode `func_addr`.
The number of parameters if specified by `param_no` operand.
First parameter is located at register `first_param`.
Return value (if any) will be stored under register `ret_value_register`, which can be set to 0 to
drop return value.


----

## `end`

**syntax**: `end`

This instruction finishes function execution.
If used in global context (i.e. when only main function is called) it is equivallent to the `HALT` instruction.


----

## `halt`

**syntax**: `halt`

This instruction causes the CPU to halt, exiting the program in effect.
