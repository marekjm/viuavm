; Example file showing more involved interfacing with C++
;
; Compilation:
;
;   g++ -std=c++11 -Wall -pedantic -fPIC -c -o registerset.o ../src/cpu/registerset.cpp
;   g++ -std=c++11 -Wall -pedantic -fPIC -shared -o io.so io.cpp registerset.o
;   ../bin/vm/asm readwrite.asm
;
; After libraries and the program are compiled result can be tested with following command
; which will ask for a path to read, read it, ask for a path write and write the string to it.
;
;   ../bin/vm/cpu a.out
;

.function: main
    eximport "io"

    strstore 1 "Path to read: "
    echo 1

    frame 0
    excall io.getline 1

    print 1

    frame 1
    param 0 1
    excall io.read 2
    echo 2

    strstore 3 "Path to write: "
    echo 3
    frame 0
    excall io.getline 3

    frame 2
    param 0 3
    param 1 2
    excall io.write 0

    izero 0
    end
.end
