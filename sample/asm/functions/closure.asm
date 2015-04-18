.def: foo 0
    ; one is bound from 'returns_closure' function
    ;print 1
    end
.end

.def: returns_closure 1
    istore 1 42
    closure foo 2
    move 2 0
    end
.end

.def: main 0
    .name: 1 bar
    ; call function that returns the closure
    frame 0
    call returns_closure bar

    ; create frame for our closure and
    ; call it
    clframe 0
    clcall bar 0

    izero 0
    end
.end


; // equivallent JavaScript code
;
; function returns_closure() {
;     var x = 42;
;     function foo() {
;         print(x);
;     }
;     return foo;
; }
;
; function main() {
;     var bar = returns_closure();
;     bar();
;     return 0;
; }
