.block: handle_integer
    ;fly 2
    istore 2 42
    strstore 3 "thrown Object: "
    echo 3
    print 2
    leave
.end

.block: main_block
    istore 1 42
    throw 1
    leave
.end

.def: main 1
    ;catch "Integer" handle_integer
    ;try main_block
    ; leave instructions lead here

    izero 0
    end
.end


; catch "<type>" <block>    - registers a catcher <block> for given <type>
; try <block>               - tries executing given block after registered catchers have been registered
; leave                     - leaves active block (if any) and resumes execution on instruction after the one that caused
;                             the block to be entered
