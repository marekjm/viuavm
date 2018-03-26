.signature: std::io::stdout::write/1
.signature: std::io::stderr::write/1
.signature: std::typesystem::typeof/1
.signature: std::os::system/1
.signature: std::random::device::random/0
.signature: std::random::device::urandom/0
.signature: std::random::random/0
.signature: std::random::randint/2

.function: print_newline/1
    arg %1 local %0

    frame ^[(pamv %0 %1 local)]
    call void std::io::stdout::write/1
    frame ^[(pamv %0 (text %2 local "\n") local)]
    call void std::io::stdout::write/1

    return
.end

.function: main/0
    import "std/io"
    import "std/typesystem"
    import "std/os"
    import "std/random"

    text %1 local "Hello, World (stdout)!\n"
    frame ^[(pamv %0 %1 local)]
    call void std::io::stdout::write/1

    text %1 local "Hello, World (stderr)!\n"
    frame ^[(pamv %0 %1 local)]
    call void std::io::stderr::write/1

    integer %1 local 42
    ptr %2 local %1 local
    frame ^[(pamv %0 %2 local)]
    call %2 local std::typesystem::typeof/1

    frame ^[(pamv %0 %2 local)]
    call void print_newline/1
    ;call void std::io::stdout::write/1
    ;frame ^[(pamv %0 (text %2 local "\n") local)]
    ;call void std::io::stdout::write/1

    frame ^[(pamv %0 (string %2 local "ls -lh") local)]
    call void std::os::system/1

    frame %0
    call %2 local std::random::device::random/0
    frame ^[(pamv %0 %2 local)]
    call void print_newline/1

    frame %0
    call %2 local std::random::device::urandom/0
    frame ^[(pamv %0 %2 local)]
    call void print_newline/1

    frame %0
    call %2 local std::random::random/0
    frame ^[(pamv %0 %2 local)]
    call void print_newline/1

    integer %1 local 42
    integer %2 local 666
    frame ^[(pamv %0 %1 local) (pamv %1 %2 local)]
    call %2 local std::random::randint/2
    frame ^[(pamv %0 %2 local)]
    call void print_newline/1

    izero %0 local
    return
.end
