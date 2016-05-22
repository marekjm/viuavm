.block: handle_integer
    ; pull caught object into 2 register
    pull 2
    print 2
    print 4
    leave
.end

.function: tertiary/1
    arg 3 0
    istore 4 300
    throw 3
    return
.end

.function: secondary/1
    arg 2 0
    istore 4 200

    frame ^[(param 0 2)] 5
    istore 4 250
    call tertiary/1

    istore 4 225
    return
.end

.block: main_block
    istore 1 42
    istore 4 100

    frame ^[(param 0 1)] 5
    call secondary/1

    istore 2 41
    istore 4 125
    leave
.end


.function: main
    istore 4 50

    try
    catch "Integer" handle_integer
    enter main_block
    ; leave instructions lead here
    print 2
    print 4

    izero 0
    return
.end


; catch "<type>" <block>    - registers a catcher <block> for given <type>
; enter <block>             - tries executing given block after registered catchers have been registered
; leave                     - leaves active block (if any) and resumes execution on instruction after the one that caused
;                             the block to be entered
