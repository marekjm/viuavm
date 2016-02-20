; this example is the Viua assembly version of the following JS:
;
;   function closure_maker(x) {
;       function closure_level_1(a) {
;           function closure_level_2(b) {
;               function closure_level_3(c) {
;                   return (x + a + b + c);
;               }
;               return closure_level_3;
;           }
;           return closure_level_2;
;       }
;       return closure_level_1;
;   }
;
;   closure_maker(1)(2)(3)(4) == 10
;

.function: closure_level_3
    ; expects 1, 2 and 3 to be enclosed integers
    .name: 5 accumulator
    move 0 (iadd accumulator (arg 4 0) (iadd accumulator 3 (iadd accumulator 1 2)))
    return
.end

.function: closure_level_2
    closure 0 closure_level_3
    ; registers 1 and 2 are occupied by enclosed integers
    ; but they must be enclosed by the "closure_level_3"
    clbind 0 1 1
    clbind 0 2 2
    clbind 0 3 (arg 3 0)
    return
.end

.function: closure_level_1
    closure 0 closure_level_2
    ; register 1 is occupied by enclosed integer
    ; but it must be enclosed by the "closure_level_2"
    clbind 0 1 1
    clbind 0 2 (arg 2 0)
    return
.end

.function: closure_maker
    ; create the outermost closure
    closure 0 closure_level_1
    clbind 0 1 (arg 1 0)
    return
.end

.function: main
    frame ^[(param 0 (istore 1 1))]
    call 2 closure_maker

    frame ^[(param 0 (istore 1 2))]
    fcall 3 2

    frame ^[(param 0 (istore 1 3))]
    fcall 4 3

    frame ^[(param 0 (istore 1 4))]
    print (fcall 5 4)

    izero 0
    return
.end
