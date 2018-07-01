; Main function of the program, automatically invoked by the VM.
; It must be present in every executable file.
; Valid main functions are main/0, main/1 and main/2.
;
.function: main/0
    ; Allocate local register set.
    ; This is done explicitly by the callee because it is an
    ; implementation detail - if a newer version of the function
    ; requires fewer registers the code using it should not have
    ; to be compiled.
    ; Also, when a module is reloaded the code using it can stay
    ; the same as long as the number and types of parameters of
    ; the function stay the same.
    ;
    ; In this function, two local registers are allocated.
    allocate_registers %2 local

    ; Store text value in register with index 1.
    ; The text instruction shares operand order with majority of
    ; other instructions:
    ;
    ;   <mnemonic> <target> <sources>...
    ;
    text %1 local "Hello World!"

    ; Print contents of a register to standard output.
    ; This is the most primitive form of output the VM supports, and
    ; should be used only in the simplest programs and
    ; for quick-and-dirty debugging.
    print %1 local

    ; The finishing sequence of every program running on Viua.
    ;
    ; Register 0 is used to store return value of a function, and
    ; the assembler will enforce that it is set to integer 0 for main
    ; function.
    izero %0 local
    return
.end
