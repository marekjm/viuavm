# Viua VM assembler pipeline

## 1. Tokenisation

Tokenise the program to make it easily digestible by software.
Replace newlines with terminator tokens.
Strip comments and whitespace.
Store a file name, line, character offset in line for each token.

This stage may throw errors.


## 2. Replace `iota` tokens

Replace `iota` tokens with sequentially increasing integers, starting
from 1. As an example:

    ; base form
    .name: iota a_message
    text %a_message local "Here is a message"
    integer %iota local 42

    ; with `iota` tokens replaced
    .name: 1 a_message
    text %a_message local "Here is a message"
    integer %2 local 42

The `iota` token is a way to relieve the programmer from having to
manually assign register indexes.

This stage may throw errors. While it just blindly replaces any `iota`
token it encounters in the token stream, blocks (areas between `.block:`
and `.end` tokens) must not contain `iota` tokens.


## 3. Unwrapping of macros

Unwrap macros. Do it in the dumbest way possible, assume everything is
fine and let later stages handle errors. Replace the parenthesised part
with the target register of the inner instruction; as an example:

    ; wrapped form
    throw (text %1 local "EADDRINUSE") local

    ; unwrapped form
    text %1 local "EADDRINUSE"
    throw %1 local

*Note*: macros are unwieldy and hard to integrate with other features.
A better design should be devised for them as they make implementation
of better checks, error messages, etc. harder than it needs to be.

*Note*: Parenthesised instruction must be replaced with both register
index and register set specifier. Current macro unwrapper replaces it
with just the index.


## 4. Normalisation

Transform the sequence of unwrapped tokens into the canonical one. This
means that all the possible valid "instruction token sequences" are
replaced with their canonical form; to give a few examples:

    call foo/0                          ; allowed form
    call void foo/0                     ; canonical form

    join %a_pid local                   ; allowed form
    join void %a_pid local infinity     ; canonical form

This stage exists just to make life easier for the parser.


## 5. Parsing

Parse normalised token stream.
Group tokens of blocks, functions, closures; gather metadata; squirrel
away any debugging information required and supplied by the programmer.


## 6. Static analysis

Perform static analysis on the code. Apart from ensuring basic type
correctness, this stage may:

- perform call graph analysis
- check for access to uninitialised registers
- check for unused values being spawned
- notify about potentially problematic constructions


## 7. Bytecode size calculation

Calculate size of the executable blocks (an executable block is a block
of bytes to which a single `.block:`, `.function:`, or `.closure:` was
compiled), metadata, debugging information, headers, etc.

Inside each executable block, calculate size of every single
instruction. This will be needed for branch target calculation.


## 8. Branch target calculation

Calculate branch targets inside each executable block. These targets are
just byte offsets relative to the beginning of the executable block in
which they exist.
They will be replaced by absolute (relative to the beginning of the
first executable block in a module) offsets after the final order of
executable blocks is established.


## 9. Bytecode emission

Emit bytecode. This stage should probably use an in-memory buffer for
the bytecode before it is written to a file.

### 9.1. Headers

First, emit all headers required before the module text. These include
metadata supplied by the programmer, dependencies, debug information,
compilation timestamps and flags, checksums, etc.
See documentation of the bytecode formats for more information.

### 9.2. Bytecode

Emit bytecode for executable blocks.

#### 9.2.1. Ordering of executable blocks

Prepare final ordering of executable blocks in a module. The canonical
order is:

- blocks
- closures
- functions

There is no particular reason for this ordering; it is alphabetical in
the name of type of the executable block. The concrete ordering is not
important; what *is* important is the fact that the ordering exists as
it allows final branch targets to be calculated.

#### 9.2.2. Final branch target calculation

Perform final branch target calculation. Use ordering of blocks and
relative offsets calculated in previous step. The final target is:

    size-of-all-previous-executable-blocks + relative-offset

#### 9.2.3. Bytecode emission for executable blocks

Emit bytecode for every executable block, in order, and save them to the
module text buffer.


## 10. Output file creation

Save the output buffer to the file.
