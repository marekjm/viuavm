; This module contains various string utility functions.
; It is a part of standard Viua runtime library.
;
; Signatures for copying are available below the documentation block.

;
; Signatures
;
.signature: std::string::stringify/1
.signature: std::string::represent/1

;
; Code
;
.function: std::string::stringify/1
    frame ^[(param 0 (ptr 2 (strstore 1 ""))) (param 1 (arg 3 0))]
    msg 0 stringify/2
    move 0 1
    return
.end

.function: std::string::represent/1
    frame ^[(param 0 (ptr 2 (strstore 1 ""))) (param 1 (arg 3 0))]
    msg 0 represent/2
    move 0 1
    return
.end
