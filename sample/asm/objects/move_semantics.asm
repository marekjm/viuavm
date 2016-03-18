.function: main
    new 1 Object
    new 2 Object

    ; print object to be used as attribute value
    ; before it is inserted
    print 2

    .name: 3 key
    strstore key "foo"

    ; insert and remove
    ; just to test move semantics
    remove 4 (insert 1 key 2) key

    ; print object used as attribute value
    ; after it was removed 
    print 4

    izero 0
    return
.end
