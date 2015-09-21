.function: with_jump
    strstore 1 "function with jump"
    print 1

    jump nearly_end

    print 1
    print 1
    print 1
    print 1

    .mark: nearly_end

    move 0 1
    end
.end

.function: with_branch
    strstore 1 "function with branch"
    print 1

    branch 1 nearly_end

    print 1
    print 1
    print 1
    print 1

    .mark: nearly_end

    move 0 1
    end
.end

.function: dis_a_closure
    print 3
    end
.end


.block: this_warns
    strstore 2 "Oh, noes! It's teh Devil!!!"
    print 2
    leave
.end

.block: this_throws
    istore 1 666
    throw 1
    leave
.end

.block: this_catches
    pull 2
    print 2
    leave
.end

.function: with_block
    try
    enter this_warns
    end
.end

.function: catches_own_exception
    try
    catch "Integer" this_catches
    enter this_throws
    end
.end

.function: needs_catching
    istore 1 666
    throw 1
    end
.end
