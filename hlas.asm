; higher-level assembly syntax
;
; suggestions

.function: main                     | INPUT INSTRUCTION INDEX
    vec 1 [0, 1, 2, 3]              | 1

    copy 3 $(vat 2 1 0)             | 2
    istore 3 42                     | 3

    print 1 ; [0, 1, 2, 3]          | 4

    istore $(vat 4 1 0) 69          | 5
    print 1 ; [69, 1, 2, 3]         | 6

    izero 0                         | 7
    end                             | 8
.end


; generated output
;
; there are many FREE instructions in generated output because
; VPUSH copies object into the vector
; use following code to push a reference:
;
;       vec 1
;       istore 0 666
;       ref 0 0         ; change object in 0 to reference to itself
;       vpush 1 0       ; pushed a reference
;
;
; the $(INSTRUCTION operand...) syntax behaves as described below:
;
;       * extaracts the instruction from between '$(' and ')'
;       * extracts target register (first operand)
;       * puts the instruction into source code
;       * then, in the original line, substitutes '$(...)' part
;         for the target register of extracted instruction
;       * pushes augmented line to source code
;
;
; the @(INSTRUCTION operand...) syntax behaves exactly as $(...) with one exception.
; it precedes target operand of nested instruction with @ character which.
; this is usefull in situations like this:
;
;       istore 2 @(imul 4 3 6)      ->  imul 4 3 6
;                                       istore 2 @4
;
;
; the "vec <register> [element (, element)...]" syntax behaves as described below:
;
;       * extracts <register> index
;       * generates code that will push each element between [ and ] to the vector
;         the code is generated using 0 register which SHOULD NOT be used
;         for purposes other than returning values and
;         as such it is assumed to be empty and available to the assembler
;
;
; nesting of $(...) or [...] is not currently implemented so code as following examples will not work:
;
;       vec 1 [1, 2, [3, 4], [5, [6, 7]]]
;
;       param 0 $(iadd 1 4 $(imul 2 7 7))
;
;
.function: main                     | INPUT INSTRUCTION INDEX FROM WHICH THE LINE HAS BEEN GENERATED
    vec 1                           | 1

    istore 0 0                      | 1
    vpush 1 0                       | 1
    free 0                          | 1

    istore 0 1                      | 1
    vpush 1 0                       | 1
    free 0                          | 1

    istore 0 2                      | 1
    vpush 1 0                       | 1
    free 0                          | 1

    istore 0 3                      | 1
    vpush 1 0                       | 1
    free 0                          | 1

    vat 2 1 0                       | 2
    copy 3 2                        | 2

    istore 3 42                     | 3

    print 1                         | 4

    vat 4 1 0                       | 5
    istore 4 69                     | 5

    print 1                         | 6

    izero 0                         | 7
    end                             | 8
.end
