.function: foo
    ; one is bound from 'returns_closure' function
    print 1
    return
.end

.function: returns_closure
    closure 2 foo
    move 0 (clbind 2 1 (istore 1 42))
    return
.end

.function: main
    .name: 1 bar
    ; call function that returns the closure
    frame 0
    call bar returns_closure

    ; create frame for our closure and
    ; call it
    frame 0 0
    fcall 0 bar

    izero 0
    return
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
