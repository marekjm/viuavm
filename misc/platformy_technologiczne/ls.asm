.import: [[dynamic]] std::os

.signature: std::os::lsdir/1
.signature: std::os::system/1
.signature: std::os::fs::path::lexically_normal/1
.signature: std::os::fs::path::lexically_relative/2

.function: print_entry/2
    allocate_registers %7 local

    .name: iota entry
    .name: iota path
    .name: iota status
    .name: iota key
    .name: iota control_sequence
    .name: iota highlight

    move %entry local %0 parameters
    move %highlight local %1 parameters

    atom %key local 'path'
    structat %path local %entry local %key local

    atom %key local 'is_directory'
    structat %status local %entry local %key local
    if *status local +1 is_a_regular_file
    text %status local "⇛ "
    jump print_path_part

    .mark: is_a_regular_file
    text %status local "  "
    
    .mark: print_path_part
    echo %status local

    if %highlight local +1 print_the_path
    string %control_sequence "\033[1m"
    echo %control_sequence local
    string %control_sequence "\033[4m"
    echo %control_sequence local

    .mark: print_the_path
    print *path local
    string %control_sequence "\033[0m\r"
    echo %control_sequence local

    return
.end
.function: print_entries/2
    allocate_registers %7 local

    .name: 5 highlight
    .name: 6 tmp

    move %1 local %0 parameters
    move %highlight local %1 parameters

    integer %2 local 0
    vlen %3 local %1 local

    .mark: entry_printing_loop
    lt %4 local %2 local %3 local
    if %4 local +1 the_end

    vat %4 local %1 local %2 local
    frame %2
    copy %0 arguments *4 local
    eq %tmp local %2 local %highlight local
    copy %1 arguments %tmp local
    call void print_entry/2

    iinc %2 local
    jump entry_printing_loop

    .mark: the_end
    return
.end

.function: max/2
    allocate_registers %3 local

    .name: 0 r0
    .name: iota lhs
    .name: iota rhs

    move %lhs local %0 parameters
    move %rhs local %1 parameters

    lt %r0 local %lhs local %rhs local
    if %r0 local +1 +3
    move %r0 local %rhs local
    jump the_end
    move %r0 local %lhs local

    .mark: the_end
    return
.end
.function: min/2
    allocate_registers %3 local

    .name: 0 r0
    .name: iota lhs
    .name: iota rhs

    move %lhs local %0 parameters
    move %rhs local %1 parameters

    gt %r0 local %lhs local %rhs local
    if %r0 local +1 +3
    move %r0 local %rhs local
    jump the_end
    move %r0 local %lhs local

    .mark: the_end
    return
.end
.function: lower_bound_to_zero/1
    allocate_registers %2 local

    .name: 0 r0
    .name: iota n

    move %n local %0 parameters
    izero %r0 local

    frame %2
    move %0 arguments %r0 local
    move %1 arguments %n local
    call %r0 local max/2

    return
.end

.function: exec_with/2
    allocate_registers %5 local

    .name: 0 r0
    .name: iota executable
    .name: iota file
    .name: iota path
    .name: iota tmp

    move %executable local %0 parameters
    move %file local %1 parameters

    text %executable local %executable local
    textlength %tmp local %executable
    idec %tmp local
    izero %r0 local
    textsub %executable local %executable local %r0 local %tmp local

    atom %path local 'path'
    structat %path local %file local %path local
    text %path local *path local

    text %tmp local " "

    textconcat %tmp local %executable local %tmp local
    textconcat %tmp local %tmp local %path local

    frame %1
    move %0 arguments %tmp local
    call void std::os::system/1

    return
.end
.function: refresh_view/1
    allocate_registers %3 local

    .name: 0 r0
    .name: iota state
    .name: iota tmp

    move %state local %0 parameters

    frame %1
    atom %tmp 'cwd'
    structat %tmp local %state local %tmp local
    copy %0 arguments *tmp local
    call %tmp local std::os::lsdir/1

    frame %1
    move %0 arguments %tmp local
    call %tmp local make_data_message/1
    self %r0 local
    send %r0 local %tmp local

    return
.end
.function: enter_item/1
    allocate_registers %7 local

    .name: 0 r0
    .name: iota state
    .name: iota key
    .name: iota item
    .name: iota is_directory
    .name: iota new_cwd
    .name: iota tmp

    move %state local %0 parameters

    atom %key local 'data'
    structat %tmp local *state local %key local

    atom %key local 'pointer'
    structat %item local *state local %key local

    vat %item local *tmp local *item local

    atom %key local 'is_directory'
    structat %is_directory local *item local %key local
    ; dereference the pointer and copy the boolean so we can use it directly
    copy %is_directory local *is_directory local

    if %is_directory local +1 the_end

    atom %key local 'path'
    structat %new_cwd local *item local %key local

    atom %key local 'cwd'
    structinsert *state local %key local *new_cwd local

    frame %0
    call %tmp local make_refresh_message/0
    self %r0 local
    send %r0 local %tmp local

    .mark: the_end
    return
.end
.function: enter_parent_dir/1
    allocate_registers %6 local

    .name: 0 r0
    .name: iota state
    .name: iota key
    .name: iota cwd
    .name: iota up
    .name: iota tmp

    move %state local %0 parameters

    atom %key local 'cwd'
    structat %cwd local *state local %key local

    text %cwd local *cwd local
    text %up "/.."
    textconcat %cwd local %cwd local %up local

    frame %1
    move %0 arguments %cwd local
    call %cwd local std::os::fs::path::lexically_normal/1

    structinsert *state local %key local %cwd local

    frame %0
    call %tmp local make_refresh_message/0
    self %r0 local
    send %r0 local %tmp local

    .mark: the_end
    return
.end
.function: remove_base_from_entries_impl/3
    allocate_registers %9 local

    .name: 0 r0
    .name: iota counter
    .name: iota entries
    .name: iota base
    .name: iota limit
    .name: iota each
    .name: iota key
    .name: iota path
    .name: iota tmp

    move %counter local %0 parameters
    move %entries local %1 parameters
    move %base local %2 parameters
    vlen %limit local *entries local

    lt %tmp local %counter local %limit local
    if %tmp local +1 the_end

    vat %each local *entries local %counter local

    atom %key local 'path'
    structat %path local *each local %key local

    frame %2
    copy %0 arguments *path local
    copy %1 arguments %base local
    call %path local std::os::fs::path::lexically_relative/2

    structinsert *each local %key local %path local

    iinc %counter local

    frame %3
    move %0 arguments %counter local
    move %1 arguments %entries local
    move %2 arguments %base local
    tailcall remove_base_from_entries_impl/3

    .mark: the_end
    return
.end
.function: remove_base_from_entries/2
    allocate_registers %3 local

    .name: 0 r0
    .name: iota entries
    .name: iota base

    move %entries local %0 parameters
    move %base local %1 parameters

    izero %r0 local

    frame %3
    move %0 arguments %r0 local
    ptr %r0 local %entries local
    move %1 arguments %r0 local
    move %2 arguments %base local
    call remove_base_from_entries_impl/3

    move %r0 local %entries local
    return
.end
.function: print_top_line/1
    allocate_registers %5 local

    .name: iota state
    .name: iota key
    .name: iota cwd
    .name: iota tmp

    move %state local %0 parameters

    atom %key local 'cwd'
    structat %cwd local *state local %key local
    text %cwd local *cwd local

    text %tmp local "["
    textconcat %cwd local %tmp local %cwd local
    text %tmp local "]\r"
    textconcat %cwd local %cwd local %tmp local

    print %cwd local

    return
.end
.function: tree_view_display_actor_impl/1
    allocate_registers %17 local

    .name: 0 r0
    .name: iota state
    .name: iota message
    .name: iota key
    .name: iota tag_shutdown
    .name: iota tag_refresh
    .name: iota tag_data
    .name: iota tag_ptr_down
    .name: iota tag_ptr_up
    .name: iota tag_esc
    .name: iota tag_exec
    .name: iota tag_enter
    .name: iota tag_go_up
    .name: iota got_tag
    .name: iota entries
    .name: iota tmp
    .name: iota control_sequence

    move %state local %0 parameters
    print %state local

    receive %message local infinity
    print %message local
    
    atom %tag_shutdown local 'shutdown'
    atom %tag_refresh local 'refresh'
    atom %tag_data local 'data'
    atom %tag_ptr_down local 'pointer_down'
    atom %tag_ptr_up local 'pointer_up'
    atom %tag_esc local 'esc'
    atom %tag_exec local 'exec'
    atom %tag_enter local 'enter_dir'
    atom %tag_go_up local 'go_up'
    atom %key local 'tag'
    structat %got_tag local %message local %key local

    atomeq %tag_shutdown local *got_tag local %tag_shutdown local
    atomeq %tag_refresh local *got_tag local %tag_refresh local
    atomeq %tag_data local *got_tag local %tag_data local
    atomeq %tag_ptr_down local *got_tag local %tag_ptr_down local
    atomeq %tag_ptr_up local *got_tag local %tag_ptr_up local
    atomeq %tag_esc local *got_tag local %tag_esc local
    atomeq %tag_exec local *got_tag local %tag_exec local
    atomeq %tag_enter local *got_tag local %tag_enter local
    atomeq %tag_go_up local *got_tag local %tag_go_up local

    if %tag_shutdown local stage_shutdown +1
    if %tag_refresh local stage_refresh +1
    if %tag_data local stage_data +1
    if %tag_ptr_down local stage_ptr_down +1
    if %tag_ptr_up local stage_ptr_up +1
    if %tag_esc local stage_esc +1
    if %tag_exec local stage_exec +1
    if %tag_enter local stage_enter +1
    if %tag_go_up local stage_go_up +1
    jump the_end

    .mark: stage_shutdown
    .mark: stage_esc
    text %tmp "tree_view_display_actor_impl shutting down"
    print %tmp local
    return

    .mark: stage_data
    atom %key local 'data'
    structremove %entries local %message local %key local
    structinsert %state local %key local %entries local
    jump pointer_bounds_check

    .mark: stage_refresh
    frame %1
    copy %0 arguments %state local
    call void refresh_view/1

    ; We can't just jump to the 'the_end' label or the static analyser will
    ; complain about unused registers... If we put the two non-using branches
    ; one after the other. If we put a branch that uses the register between the
    ; two non-using ones then all is good.
    ;
    ; Nice code you have there - sooo reliable.
    jump the_end

    .mark: stage_exec
    atom %key local 'executable'
    structremove %tmp local %message local %key local

    frame %2
    move %0 arguments %tmp local
    atom %key local 'pointer'
    structat %tmp local %state local %key local
    atom %key local 'data'
    structat %entries local %state local %key local
    vat %tmp local *entries local *tmp local
    copy %1 arguments *tmp local
    call void exec_with/2

    jump printing_sequence

    .mark: stage_enter
    frame %1
    ptr %tmp local %state local
    copy %0 arguments %tmp local
    call void enter_item/1

    jump printing_sequence

    .mark: stage_go_up
    frame %1
    ptr %tmp local %state local
    move %0 arguments %tmp local
    call void enter_parent_dir/1

    jump printing_sequence

    .mark: stage_ptr_down
    atom %tmp local 'pointer'
    structat %tmp local %state local %tmp local
    iinc *tmp local
    jump pointer_bounds_check

    .mark: stage_ptr_up
    atom %tmp local 'pointer'
    structat %tmp local %state local %tmp local
    idec *tmp local
    ; FIXME this jump breaks the bytecode and is apprently compiled as a
    ; self-jump
    ; jump printing_sequence

    .mark: pointer_bounds_check
    ;
    ; make sure pointer does not exceed bounds
    ;
    atom %key local 'data'
    structat %entries local %state local %key local

    atom %key local 'pointer'
    structat %tmp local %state local %key local
    frame %1
    copy %0 arguments *tmp local
    call %tmp local lower_bound_to_zero/1

    frame %2 local
    move %0 arguments %tmp local
    vlen %tmp local *entries local
    idec %tmp local
    move %1 arguments %tmp local
    call %tmp local min/2

    structinsert %state local %key local %tmp local

    .mark: printing_sequence
    string %control_sequence "\033[2J"
    echo %control_sequence local
    string %control_sequence "\033[1;1H"
    echo %control_sequence local

    ;
    ; normalise paths used in entries
    ;
    frame %2 local

    atom %key local 'data'
    structat %entries local %state local %key local
    copy %0 arguments *entries local

    atom %key local 'cwd'
    structat %tmp local %state local %key local
    copy %1 arguments *tmp local
    delete %tmp local

    call %entries local remove_base_from_entries/2

    ;
    ; present the data on the screen
    ;
    frame %1 local
    ptr %tmp local %state local
    move %0 arguments %tmp local
    call void print_top_line/1

    frame %2
    move %0 arguments %entries local
    atom %key local 'pointer'
    structat %tmp local %state local %key local
    copy %1 arguments *tmp local
    call void print_entries/2

    .mark: the_end
    frame %1
    move %0 arguments %state local
    tailcall tree_view_display_actor_impl/1
.end
.function: tree_view_display_actor/1
    allocate_registers %5 local

    .name: iota directory
    .name: iota state
    .name: iota key
    .name: iota value

    move %directory local %0 parameters

    struct %state local

    ; pointer is the index of item that should be highlighted
    atom %key local 'pointer'
    izero %value local 0
    structinsert %state local %key local %value local

    ; data is the list of items to display
    atom %key local 'data'
    vector %value local
    structinsert %state local %key local %value local

    ; the current directory to list
    atom %key local 'cwd'
    structinsert %state local %key local %directory local

    frame %1
    move %0 arguments %state local
    tailcall tree_view_display_actor_impl/1
.end

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
.function: make_exec_message/1
    allocate_registers %5 local

    .name: iota message
    .name: iota tag
    .name: iota executable
    .name: iota key

    atom %tag local 'exec'
    frame %1
    move %0 arguments %tag local
    call %message local make_tagged_message_impl/1

    ; insert the data field: { tag: 'exec', executable: ... }
    atom %key local 'executable'
    move %executable local %0 parameters
    structinsert %message local %key local %executable local

    move %0 local %message local
    return
.end
.function: make_enter_dir_message/0
    allocate_registers %2 local

    .name: iota tag
    atom %tag local 'enter_dir'

    frame %1
    move %0 arguments %tag local
    call %0 local make_tagged_message_impl/1
    return
.end
.function: make_refresh_message/0
    allocate_registers %2 local

    .name: iota tag
    atom %tag local 'refresh'

    frame %1
    move %0 arguments %tag local
    call %0 local make_tagged_message_impl/1
    return
.end
.function: make_go_up_message/0
    allocate_registers %2 local

    .name: iota tag
    atom %tag local 'go_up'

    frame %1
    move %0 arguments %tag local
    call %0 local make_tagged_message_impl/1
    return
.end

.function: time_to_shut_down/0
    allocate_registers %1 local

    integer %0 local 0
    not %0 local

    try
    catch "Exception" .block: catch_me
        draw void
        not %0 local
        leave
    .end
    enter .block: try_receiving
        receive void 0ms
        leave
    .end

    return
.end
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
.function: prepare_and_send_exec_message/1
    allocate_registers %6 local

    .name: iota stdin
    .name: iota buf
    .name: iota req
    .name: iota message
    .name: iota dst

    integer %stdin local 0
    integer %buf local 128
    io_read %req local %stdin local %buf local

    io_wait %buf local %req local infinity

    frame %1
    move %0 arguments %buf local
    call %message local make_exec_message/1

    move %dst local %0 parameters
    send %dst local %message local

    return
.end
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
    allocate_registers %11 local

    .name: iota tree_view_actor
    .name: iota tmp
    .name: iota buf

    .name: iota c_quit
    .name: iota c_refresh
    .name: iota c_pointer_down
    .name: iota c_pointer_up
    .name: iota c_exec
    .name: iota c_enter
    .name: iota c_go_up

    move %tree_view_actor local %0 parameters

    frame %0
    call %tmp local time_to_shut_down/0
    if %tmp local +1 await_input_stage
    text %tmp local "time to shut down"
    print %tmp local
    return

    .mark: await_input_stage
    frame %0
    call %buf local input_actor_await_io/0

    string %c_quit local "q"
    ;string %c_quit local "\0x71"
    ;print %c_quit local
    streq %c_quit local %buf local %c_quit local
    string %c_refresh local "r"
    streq %c_refresh local %buf local %c_refresh local
    string %c_pointer_down local "j"
    streq %c_pointer_down local %buf local %c_pointer_down local
    string %c_pointer_up local "k"
    streq %c_pointer_up local %buf local %c_pointer_up local
    string %c_exec local "x"
    streq %c_exec local %buf local %c_exec local
    string %c_enter local "e"
    streq %c_enter local %buf local %c_enter local
    string %c_go_up local "u"
    streq %c_go_up local %buf local %c_go_up local

    if %c_quit local the_end +1
    if %c_refresh local refresh_display +1
    if %c_pointer_down local move_pointer_down +1
    if %c_pointer_up local move_pointer_up +1
    if %c_exec local exec_item +1
    if %c_enter local enter_dir +1
    if %c_go_up local go_up +1

    jump happy_loopin

    .mark: refresh_display

    frame %0
    call %tmp make_refresh_message/0
    send %tree_view_actor local %tmp local

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

    .mark: exec_item

    frame %0
    call void return_tty_to_sanity/0

    frame %1
    copy %0 arguments %tree_view_actor local
    call void prepare_and_send_exec_message/1

    frame %0
    call void make_tty_raw/0

    jump happy_loopin

    .mark: enter_dir

    frame %0
    call %tmp make_enter_dir_message/0
    send %tree_view_actor local %tmp local

    jump happy_loopin

    .mark: go_up

    frame %0
    call %tmp make_go_up_message/0
    send %tree_view_actor local %tmp local

    jump happy_loopin

    .mark: the_end
    frame %0
    call %tmp local make_shutdown_message/0
    send %tree_view_actor local %tmp local

    text %tmp "input_actor shutting down"
    print %tmp local
    return

    .mark: happy_loopin
    frame %1
    move %0 arguments %tree_view_actor local
    tailcall input_actor_impl/1
.end
.function: input_actor/1
    allocate_registers %2 local

    .name: iota tree_view_actor

    frame %0
    call void make_tty_raw/0

    frame %1
    move %tree_view_actor local %0 parameters
    move %0 arguments %tree_view_actor local
    tailcall input_actor_impl/1
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
.function: main/2
    allocate_registers %4 local

    .name: 0 r0
    .name: iota directory
    .name: iota tree_view_actor
    .name: iota tmp

    frame %0
    defer return_tty_to_sanity/0

    move %directory local %1 parameters
    if %directory local +1 use_default_directory
    vpop %directory local %directory local void
    jump +2
    .mark: use_default_directory
    string %directory local "."

    frame %1
    move %0 arguments %directory local
    call %directory local std::os::fs::path::lexically_normal/1

    ;
    ; launch worker actors
    ;
    frame %1
    copy %0 arguments %directory local
    process %tree_view_actor local tree_view_display_actor/1

    frame %1
    copy %0 arguments %tree_view_actor local
    process void input_actor/1

    ;
    ; set up initial directory listting
    ;
    frame %1
    copy %0 arguments %directory local
    call %tmp local std::os::lsdir/1
    frame %1
    move %0 arguments %tmp local
    call %tmp local make_data_message/1
    send %tree_view_actor local %tmp local

    ;
    ; join worker actors
    ;
    join void %tree_view_actor local

    string %tmp "\033[2J"
    echo %tmp local
    string %tmp "\033[1;1H"
    echo %tmp local

    izero %0 local
    return
.end
