.signature: std::io::stdout::write/1
.signature: std::io::stderr::write/1

.function: main/0
    import "io"

    text %1 local "Hello, World (stdout)!\n"
    frame ^[(pamv %0 %1 local)]
    call void std::io::stdout::write/1

    text %1 local "Hello, World (stderr)!\n"
    frame ^[(pamv %0 %1 local)]
    call void std::io::stderr::write/1

    izero %0 local
    return
.end
