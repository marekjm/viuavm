.import: [[dynamic]] std::os

.signature: std::os::lsdir/1

.function: print_entry/1
    allocate_registers %5 local

    .name: iota entry
    .name: iota path
    .name: iota status
    .name: iota key

    move %entry local %0 parameters

    atom %key local 'path'
    structat %path local %entry local %key local

    atom %key local 'is_directory'
    structat %status local %entry local %key local
    if *status local +1 is_a_regular_file
    text %status local "⇛ "
    jump print_the_path

    .mark: is_a_regular_file
    text %status local "  "
    
    .mark: print_the_path
    echo %status local
    print *path local

    return
.end

.function: main/2
    allocate_registers %5 local

    move %1 local %1 parameters
    if %1 local +1 use_default_directory
    vpop %1 local %1 local void
    jump +2
    .mark: use_default_directory
    string %1 local "."

    print %1 local

    frame %1
    move %0 arguments %1 local
    call %1 local std::os::lsdir/1

    integer %2 local 0
    vlen %3 local %1 local

    .mark: entry_printing_loop
    lt %4 local %2 local %3 local
    if %4 local +1 the_end

    vat %4 local %1 local %2 local
    frame %1
    copy %0 arguments *4 local
    call void print_entry/1

    iinc %2 local
    jump entry_printing_loop

    .mark: the_end

    izero %0 local
    return
.end
