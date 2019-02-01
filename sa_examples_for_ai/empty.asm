; This file is empty. The static analyser gives enough clues to fill this file
; with a valid main function.

.function: main/0
    allocate_registers %1 local
    text %0 local "Hello World!"
    return
.end
