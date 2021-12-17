.function: [[entry_point]] main
    float $1, 3.14
    li $2, 3u

    ; Take a reference to the float in register $1.l to force the VM to box the
    ; float. We do not need the reference so it can be immediately deleted.
    g.ref $4, $1
    delete $4

    cmp $3, $1, $2

    ebreak
    return
.end
