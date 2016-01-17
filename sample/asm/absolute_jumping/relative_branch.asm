.function: main
    .name: 1 text_to_print
    .name: 2 condition

    strstore text_to_print "Hello World"
    istore condition 42

    branch condition +2 +1
    strstore text_to_print "Goodby World, you fail this test"

    print text_to_print

    izero 0
    return
.end
