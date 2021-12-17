.function: [[entry_point]] main
    string $1, "Hello, World!"
    string $2, "Hello, World!"

    cmp $3, $1, $2
    eq $4, $1, $2
    lt $5, $1, $2
    gt $6, $1, $2

    ebreak
    return
.end
