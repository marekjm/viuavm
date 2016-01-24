; Example file showing more involved interfacing with C++
;
; Compilation:
;
;   g++ -std=c++11 -Wall -pedantic -fPIC -c -I../include -o registerset.o ../src/cpu/registerset.cpp
;   g++ -std=c++11 -Wall -pedantic -fPIC -shared -I../include -o io.so io.cpp registerset.o
;   ../build/bin/vm/asm readwrite.asm
;
; After libraries and the program are compiled result can be tested with following command
; which will ask for a path to read, read it, ask for a path write and write the string to it.
;
;   ../build/bin/vm/cpu a.out
;

.signature: io::getline
.signature: io::read
.signature: io::write

.function: main
    import "./io"

    echo (strstore 1 "Path to read: ")
    frame 0
    print (call 1 io::getline)

    frame ^[(param 0 1)]
    echo (call 2 io::read)

    echo (strstore 3 "Path to write: ")
    frame 0
    call 3 io::getline

    frame ^[(param 0 3) (param 1 2)]
    call 0 io::write

    izero 0
    return
.end
