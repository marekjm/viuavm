; This module contains various string utility functions.
; It is a part of standard Viua runtime library.
;
; Signatures for copying are available below the documentation block.

;
; Signatures
;
.signature: viua::core::string::stringify
.signature: viua::core::string::represent

;
; Code
;
.function: viua::core::string::stringify
    strstore 1 ""
    frame ^[(paref 0 (arg 2 0))]
    msg 1 1 stringify
    move 0 1
    end
.end

.function: viua::core::string::represent
    strstore 1 ""
    frame ^[(paref 0 (arg 2 0))]
    msg 1 1 represent
    move 0 1
    end
.end
