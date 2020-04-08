.import: [[dynamic]] viuapq
.import: [[dynamic]] std::os

.signature: viuapq::connect/1
.signature: viuapq::finish/1
.signature: viuapq::get/2
.signature: std::os::system/1


; Funkcje pomocznicze ogólnego przeznaczenia.
.function: map/4
    allocate_registers %6 local

    .name: 0 r0
    .name: iota vec
    .name: iota fn
    .name: iota n
    .name: iota limit
    .name: iota item

    move %vec local %0 parameters
    move %fn local %1 parameters
    move %n local %2 parameters
    move %limit local %3 parameters

    vpop %item local %vec local %n local
    frame %1
    move %0 arguments %item local
    call %item local %fn local
    vinsert %vec local %item local %n local

    iinc %n local
    lt %r0 local %n local %limit local
    if %r0 local next_iteration the_end

    .mark: next_iteration
    frame %4
    move %0 arguments %vec local
    move %1 arguments %fn local
    move %2 arguments %n local
    move %3 arguments %limit local
    tailcall map/4

    .mark: the_end
    move %r0 local %vec local
    return
.end
.function: map/2
    allocate_registers %4 local

    .name: 0 r0
    .name: iota vec
    .name: iota n
    .name: iota limit

    move %vec local %0 parameters
    izero %n local
    vlen %limit local %vec local

    frame %4
    move %0 arguments %vec local
    move %1 arguments %1 parameters
    move %2 arguments %n local
    move %3 arguments %limit local
    tailcall map/4
.end


; Funkcje pomocnicze do zarządzania stanem konsoli.
.function: make_tty_raw/0
    allocate_registers %2 local

    .name: iota tmp

    string %tmp local "stty raw"
    frame %1
    move %0 arguments %tmp local
    call void std::os::system/1

    string %tmp local "stty -echo"
    frame %1
    move %0 arguments %tmp local
    call void std::os::system/1

    return
.end
.function: return_tty_to_sanity/0
    allocate_registers %2 local

    .name: iota command

    string %command local "stty sane"
    frame %1
    move %0 arguments %command local
    call void std::os::system/1

    string %command local "stty cooked"
    frame %1
    move %0 arguments %command local
    call void std::os::system/1

    return
.end


; Funkcje some_of_pq/1 konwertują wartości zwrócone z tabeli "some" na użyteczne
; struktury danych.
; Funkcje some_all/1 pobierają wszystkie rekordy z tabeli "some".
.function: employee_of_pq/1
    allocate_registers %4 local

    .name: 0 r0
    .name: iota employee
    .name: iota key
    .name: iota value

    move %employee local %0 parameters

    atom %key local 'id'
    structremove %value local %employee local %key local
    stoi %value local %value local
    structinsert %employee local %key local %value local

    move %r0 local %employee local
    return
.end
.function: employees_all/1
    allocate_registers %2 local

    .name: 0 r0
    .name: iota query

    text %query local "select * from employees"
    frame %2
    move %0 arguments %0 parameters
    move %1 arguments %query local
    call %query local viuapq::get/2

    frame %2
    move %0 arguments %query local
    function %r0 local employee_of_pq/1
    move %1 arguments %r0 local
    call %r0 local map/2

    return
.end

.function: order_of_pq/1
    allocate_registers %4 local

    .name: 0 r0
    .name: iota order
    .name: iota key
    .name: iota value

    move %order local %0 parameters

    atom %key local 'id'
    structremove %value local %order local %key local
    stoi %value local %value local
    structinsert %order local %key local %value local

    atom %key local 'employee'
    structremove %value local %order local %key local
    stoi %value local %value local
    structinsert %order local %key local %value local

    move %r0 local %order local
    return
.end
.function: orders_all/1
    allocate_registers %2 local

    .name: 0 r0
    .name: iota query

    text %query local "select * from orders"
    frame %2
    move %0 arguments %0 parameters
    move %1 arguments %query local
    call %query local viuapq::get/2

    frame %2
    move %0 arguments %query local
    function %r0 local order_of_pq/1
    move %1 arguments %r0 local
    call %r0 local map/2

    return
.end


; Implementacja aktora prezentującego interfejs.
.function: print_top_line/1
    allocate_registers %3 local

    .name: 0 r0
    .name: iota state
    .name: iota tmp

    move %state local %0 parameters

    text %tmp local "["
    text %state local *state local
    textconcat %state local %tmp local %state local
    text %tmp local "]\r"
    textconcat %state local %state local %tmp local

    print %state local

    return
.end
.function: print_bottom_line/1
    allocate_registers %3 local

    .name: 0 r0
    .name: iota state
    .name: iota tmp

    move %state local %0 parameters

    text %tmp local "["
    text %state local *state local
    textconcat %state local %tmp local %state local
    text %tmp local "]\r"
    textconcat %state local %state local %tmp local

    print %state local

    return
.end
.function: view_actor_impl/1
    allocate_registers %11 local

    .name: 0 r0
    .name: iota state
    .name: iota message
    .name: iota control_sequence
    .name: iota tmp
    .name: iota key
    .name: iota got_tag
    .name: iota tag_shutdown
    .name: iota tag_data
    .name: iota tag_ptr_down
    .name: iota tag_ptr_up

    move %state local %0 parameters

    receive %message local infinity

    atom %tag_shutdown local 'shutdown'
    atom %tag_data local 'data'
    atom %tag_ptr_down local 'pointer_down'
    atom %tag_ptr_up local 'pointer_up'
    atom %key local 'tag'
    structat %got_tag local %message local %key local

    atomeq %tag_shutdown local *got_tag local %tag_shutdown local
    atomeq %tag_data local *got_tag local %tag_data local
    atomeq %tag_ptr_down local *got_tag local %tag_ptr_down local
    atomeq %tag_ptr_up local *got_tag local %tag_ptr_up local

    if %tag_shutdown local stage_shutdown +1
    if %tag_data local stage_data +1
    if %tag_ptr_down local stage_ptr_down +1
    if %tag_ptr_up local stage_ptr_up +1
    jump the_end

    .mark: stage_shutdown
    text %tmp "view actor shutting down\r"
    print %tmp local
    return

    .mark: stage_data
    .mark: stage_ptr_down
    .mark: stage_ptr_up

    .mark: printing_sequence
    string %control_sequence "\033[2J"
    echo %control_sequence local
    string %control_sequence "\033[1;1H"
    echo %control_sequence local

    frame %1 local
    ptr %tmp local %state local
    move %0 arguments %tmp local
    call void print_top_line/1

    frame %1 local
    ptr %tmp local %state local
    move %0 arguments %tmp local
    call void print_bottom_line/1

    .mark: the_end
    frame %1
    move %0 arguments %state local
    tailcall view_actor_impl/1
.end
.function: view_actor/1
    allocate_registers %5 local

    .name: iota input_actor
    .name: iota state
    .name: iota key
    .name: iota value

    text %0 local "view actor starting up... (awaiting PID of input actor)"
    print %0 local

    receive %input_actor local 60s

    text %0 local "view actor starting up..."
    print %0 local

    struct %state local

    atom %key local 'pointer'
    izero %value local 0
    structinsert %state local %key local %value local

    atom %key local 'data'
    vector %value local
    structinsert %state local %key local %value local

    atom %key local 'input_actor'
    structinsert %state local %key local %input_actor local

    frame %1
    move %0 arguments %state local
    tailcall view_actor_impl/1
.end



; Funkcje pomocnicze do tworzenia różnych wiadomości.
.function: make_tagged_message_impl/1
    allocate_registers %4 local

    .name: iota message
    .name: iota tag
    .name: iota key

    move %tag local %0 parameters

    struct %message local
    atom %key local 'tag'
    structinsert %message local %key local %tag local

    move %0 local %message local
    return
.end
.function: make_data_message/1
    allocate_registers %5 local

    .name: iota message
    .name: iota tag
    .name: iota data
    .name: iota key

    atom %tag local 'data'
    frame %1
    move %0 arguments %tag local
    call %message local make_tagged_message_impl/1

    ; insert the data field: { tag: 'data', data: ... }
    atom %key local 'data'
    move %data local %0 parameters
    structinsert %message local %key local %data local

    move %0 local %message local
    return
.end
.function: make_shutdown_message/0
    allocate_registers %2 local

    .name: iota tag
    atom %tag local 'shutdown'

    frame %1
    move %0 arguments %tag local
    call %0 local make_tagged_message_impl/1
    return
.end
.function: make_pointer_up_message/0
    allocate_registers %2 local

    .name: iota tag
    atom %tag local 'pointer_up'

    frame %1
    move %0 arguments %tag local
    call %0 local make_tagged_message_impl/1
    return
.end
.function: make_pointer_down_message/0
    allocate_registers %2 local

    .name: iota tag
    atom %tag local 'pointer_down'

    frame %1
    move %0 arguments %tag local
    call %0 local make_tagged_message_impl/1
    return
.end


; Implementacja aktora pobierającego dane od użutkownika.
.function: input_actor_await_io_handle_cancellation/1
    allocate_registers %2 local

    .name: 0 r0
    .name: iota req

    draw void

    move %req local %0 parameters

    io_cancel %req local

    try
    catch "IO_cancel" .block: catch_cancelled
        draw void
        leave
    .end
    enter .block: wait_for_cancelled
        io_wait void %req local infinity
        leave
    .end

    return
.end
.function: input_actor_await_io/0
    allocate_registers %4 local

    .name: 0 r0
    .name: iota stdin
    .name: iota buf
    .name: iota req

    integer %stdin local 0
    integer %buf local 1
    io_read %req local %stdin local %buf local

    try
    catch "Exception" .block: no_input_available
        frame %1
        move %0 arguments %req local
        call void input_actor_await_io_handle_cancellation/1

        string %buf local "_"
        leave
    .end
    enter .block: wait_for_some_input
        io_wait %buf local %req local 1s
        leave
    .end

    move %r0 local %buf local
    return
.end
.function: input_actor_impl/1
    allocate_registers %13 local

    .name: iota tree_view_actor
    .name: iota tmp
    .name: iota buf

    .name: iota c_quit
    .name: iota c_pointer_down
    .name: iota c_pointer_up

    move %tree_view_actor local %0 parameters

    .mark: await_input_stage
    frame %0
    call %buf local input_actor_await_io/0

    string %c_quit local "q"
    streq %c_quit local %buf local %c_quit local
    string %c_pointer_down local "j"
    streq %c_pointer_down local %buf local %c_pointer_down local
    string %c_pointer_up local "k"
    streq %c_pointer_up local %buf local %c_pointer_up local

    if %c_quit local the_end +1
    if %c_pointer_down local move_pointer_down +1
    if %c_pointer_up local move_pointer_up +1

    jump happy_loopin

    .mark: move_pointer_down

    frame %0
    call %tmp make_pointer_down_message/0
    send %tree_view_actor local %tmp local

    jump happy_loopin

    .mark: move_pointer_up

    frame %0
    call %tmp make_pointer_up_message/0
    send %tree_view_actor local %tmp local

    jump happy_loopin

    .mark: the_end
    frame %0
    call %tmp local make_shutdown_message/0
    send %tree_view_actor local %tmp local

    text %tmp "input actor shutting down\r"
    print %tmp local
    return

    .mark: happy_loopin
    frame %1
    move %0 arguments %tree_view_actor local
    tailcall input_actor_impl/1
.end
.function: input_actor/1
    allocate_registers %1 local

    frame %0
    call void make_tty_raw/0

    text %0 local "input actor starting up..."
    print %0 local

    frame %1
    move %0 arguments %0 parameters
    tailcall input_actor_impl/1
.end


.function: main/0
    allocate_registers %4 local

    .name: 0 r0
    .name: iota connection
    .name: iota view_actor
    .name: iota input_actor

    frame %0
    defer return_tty_to_sanity/0

    text %connection local "dbname = pt_lab2"

    frame %1
    move %0 arguments %connection local
    call %connection local viuapq::connect/1

    frame %1
    ptr %r0 local %connection local
    move %0 arguments %r0 local
    process %view_actor local view_actor/1

    frame %1
    copy %0 arguments %view_actor local
    process %input_actor local input_actor/1

    copy %r0 local %input_actor local
    send %view_actor local %r0 local

    ;frame %1
    ;ptr %r0 local %connection local
    ;move %0 arguments %r0 local
    ;call %query local employees_all/1
    ;print %query local

    ;frame %1
    ;ptr %r0 local %connection local
    ;move %0 arguments %r0 local
    ;call %query local orders_all/1
    ;print %query local

    join void %input_actor local
    join void %view_actor local

    frame %1
    move %0 arguments %connection local
    call void viuapq::finish/1

    izero %0 local
    return
.end