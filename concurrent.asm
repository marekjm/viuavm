.function: main/2
    allocate_registers %3 local

    ; Make arguments accessible.
    .name: iota argv
    move %argv local %1 parameters

    ; Determine the number of processes to grow.
    .name: iota no_of_processes
    izero %no_of_processes local
    vat %no_of_processes local %argv local %no_of_processes local
    stoi %no_of_processes local *no_of_processes local

    print %no_of_processes local

    izero %0 local
    return
.end
