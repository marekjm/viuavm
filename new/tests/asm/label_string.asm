.label: hello_world
.value: string "Hello, World!"

.function: [[entry_point]] main
    string $1, hello_world
    ebreak
    return
.end
