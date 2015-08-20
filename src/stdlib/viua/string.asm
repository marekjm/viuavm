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
    frame ^[(paref 0 (strstore 1 "")) (paref 1 (arg 2 0))]
    msg 0 stringify
    move 0 1
    end
.end

.function: std::string::represent
    frame ^[(paref 0 (strstore 1 "")) (paref 1 (arg 2 0))]
    msg 0 represent
    move 0 1
    end
.end
