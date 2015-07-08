.signature: linked::with_jump
.signature: linked::with_branch
.signature: linked::dis_a_closure
.signature: linked::with_block
.signature: linked::catches_own_exception
.signature: linked::needs_catching

.signature: linked::undefined_function


.block: local_block
    frame 0
    call 0 linked::needs_catching
    leave
.end

.block: local_thrower
    istore 1 69
    throw 1
    leave
.end

.block: local_catcher
    pull 8
    strstore 7 "caught something: "
    echo 7
    print 8
    leave
.end

.function: main
    link linked

    frame 0
    call linked::with_jump

    frame 0
    call 1 linked::with_branch

    jump nearly_end

    print 1
    print 1
    print 1

    .mark: nearly_end

    function 2 linked::with_jump
    frame 0
    fcall 0 2

    strstore 3 "printed from closure"
    clbind 3
    closure 4 linked::dis_a_closure
    frame 0
    fcall 0 4

    ; this will throw if uncommented
    ;function 5 linked::undefined_function
    ;frame 0
    ;fcall 0 5

    strstore 7 "Be ready, pirates, errors lie ahead!"
    print 7

    frame 0
    call 0 linked::with_block

    frame 0
    call 0 linked::catches_own_exception

    ;tryframe
    ;catch "Integer" local_catcher
    ;try local_thrower

    tryframe
    catch "Integer" local_catcher
    try local_block

    strstore 10 "It's OK now."
    print 10

    izero 0
    end
.end
