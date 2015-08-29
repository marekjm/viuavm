.function: main
    .name: 2 H
    .name: 3 e
    .name: 4 l
    .name: 5 o
    .name: 7 W
    .name: 8 r
    .name: 9 d

    ; exclamation mark
    .name: 1 _exmark
    bstore _exmark 33

    bstore H 72
    bstore e 101
    bstore l 108
    bstore o 111

    .name: 6 _space
    bstore _space 32

    bstore W 87
    bstore r 114
    bstore d 100

    echo H
    echo e
    echo l
    echo l
    echo o

    echo _space

    echo W
    echo o
    echo r
    echo l
    echo d

    print _exmark

    izero 0
    end
.end
