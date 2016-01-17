.function: jumprint
    branch (ilt 2 (istore 2 42) (arg 1 0)) lesser
    strstore 3 ":-)"
    jump +2

    .mark: lesser
    strstore 3 ":-("

    print 1
    print 3
    return
.end
