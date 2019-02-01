.function: main/2
    allocate_registers %6 local

    ; Create a piece of text, and a vector. The vector will be used to hold the
    ; text.
    text %1 local "Hello World!"
    vector %2 local void void

    ; Push the text value to the vector. It will be moved from the register it
    ; was in into the vector without copying...
    vpush %2 local %1 local

    ; ...so this instruction would fail.
    ;print %1 local

    ; Now, access the value inside the vector (on the 0th index). The static
    ; analyser inferred that the vector is a "vector of text" so the result of
    ; the VAT instruction is a "pointer to text".
    integer %1 local 0
    vat %3 local %2 local %1 local

    ; Since we have a "pointer to text" there are several instructions that may
    ; fail...
    ; The first is incrementation without dereference - because it is a pointer,
    ; not an integer.
    ;iinc %3 local

    ; Then, incrementation with dereference - because it is a pointer to text,
    ; not to an integer.
    ;iinc *3 local

    vector %4 local void void
    vpush %4 local %3 local

    vat %5 local %4 local %1 local

    print %5 local

    izero %0 local
    return
.end
