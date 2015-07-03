.function: foo
    ; one is bound from 'returns_closure' function
    print 1
    end
.end

.function: returns_closure
    istore 1 42

    clbind 1
    closure foo 2
    move 0 2
    end
.end

.function: main
    .name: 1 bar
    ; call function that returns the closure
    frame 0
    call returns_closure bar

    ; create frame for our closure and
    ; call it
    frame 0 0
    fcall bar 0

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
