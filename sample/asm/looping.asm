; This script is just a simple loop.
; It displays numbers from 0 to 10.

; TODO: this could cause errors if scopes are implemented for functions as
; these two variables would not go into local function scope
; solution: `scope` instruction for scope switching (global/local) and
; possibly up-scoping - if register at given index is null in local scope,
; look for it in global registers
istore 1 0
istore 2 10

.def: main 0
    ; mark the beginning of the loop
    .mark: loop
    ilt 1 2 3
    ; invert value to use short form of branch instruction, i.e.: branch <cond> <true>
    ; and expliot the fact that it will default false to "next instruction"
    not 3
    branch 3 :final_print
    print 1
    iinc 1
    jump :loop
    .mark: final_print
    print 1
    end
.end

frame 0
call main
halt
