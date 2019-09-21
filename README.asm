; Main function of the program, automatically invoked by the VM.
; It must be present in every executable file.
; Valid main functions are main/0, main/1 and main/2.
;
.function: main/0
    ; Allocate local register set.
    ; This is done explicitly by the callee because it is an implementation
    ; detail - if a newer version of the function requires fewer (or more)
    ; registers the code using it should not have to be recompiled.
    ; Also, when a module is reloaded the code using it can stay the same as
    ; long as the number and types of parameters of the function stay the same.
    allocate_registers %4 local

    ; Store text value in local register with index 1.
    ; The text instruction shares operand order with majority of
    ; other instructions:
    ;
    ;   <mnemonic> <target> <sources>...
    ;
    text %1 local "Hello World!\n"

    ; Store integer value in local register 2.
    ; Use a 1 because it will be used to represent the standard output stream on
    ; UNIX-like systems.
    integer %2 local 1

    ; Write "Hello World!" on standard output. The io_write instruction will
    ; create an I/O request that will be carried out asynchronously and
    ; immediately return control to the process.
    io_write %3 local %2 local %1 local

    ; Wait for the completion of the I/O request. In this case, the timeout
    ; given is 1 second.
    ; Void register (signified by the use of 'void') is used to tell the
    ; instruction that the result of the I/O operation is not needed and may be
    ; discarded.
    io_wait void %3 local 1s

    ; The finishing sequence of every program running on Viua.
    ;
    ; Register 0 is used to store return value of a function, and
    ; the assembler will enforce that it is set to integer 0 for main
    ; function.
    izero %0 local
    return
.end
