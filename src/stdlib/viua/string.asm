; This module contains various string utility functions.
; It is a part of standard Viua runtime library.
;
; Signatures for copying are available below the documentation block.

;
; Signatures
;
.signature: std::string::stringify
.signature: std::string::represent

;
; Code
;
.function: std::string::stringify
    frame ^[(param 0 (ptr 2 (strstore 1 ""))) (param 1 (arg 3 0))]
    msg 0 stringify
    move 0 1
    end
.end

.function: std::string::represent
    frame ^[(param 0 (ptr 2 (strstore 1 ""))) (param 1 (arg 3 0))]
    msg 0 represent
    move 0 1
    end
.end
