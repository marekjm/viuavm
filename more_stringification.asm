.signature: viua::core::string::stringify
.signature: viua::core::string::represent

.function: main
    ; this program requires "string" module from standard runtime library
    ; it needs functions from "viua::core::string" namespace
    link string

    strstore 1 "Hello World!"

    frame ^[(paref 0 1)]
    call 1 viua::core::string::stringify

    frame ^[(paref 0 1)]
    call 2 viua::core::string::represent

    print 1
    print 2

    izero 0
    end
.end
