; Main function of the program, automatically invoked by the VM.
; It must be present in every executable file.
; Valid main functions are main/0, main/1 and main/2.
;
.function: main/0
    ; Store string in register with index 1.
    ; The strstore instruction has two operands, and
    ; shares operand order with majority of other instructions:
    ;
    ;   <mnemonic> <target> <source>...
    ;
    strstore %1 "Hello World!"

    ; Print contents of a register to standard output.
    ; This is the most primitive form of output the VM supports, and
    ; should be used only in the simplest programs and
    ; for quick-and-dirty debugging.
    print %1

    ; The finishing sequence of every program running on Viua.
    ;
    ; Register 0 is used to store return value of a function, and
    ; the assembler will enforce that it is set to integer 0 for main
    ; function.
    izero %0 local
    return
.end
