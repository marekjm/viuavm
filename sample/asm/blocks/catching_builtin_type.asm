.block: handle_integer
    ; pull caught object into 2 register
    pull 2
    print 2
    leave
.end

.block: main_block
    istore 1 42
    throw 1
    leave
.end

.function: main
    tryframe
    catch "Integer" handle_integer
    enter main_block
    ; leave instructions lead here

    izero 0
    end
.end


; catch "<type>" <block>    - registers a catcher <block> for given <type>
; enter <block>             - tries executing given block after registered catchers have been registered
; leave                     - leaves active block (if any) and resumes execution on instruction after the one that caused
;                             the block to be entered
