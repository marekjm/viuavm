.function: prefixed_printer/1
    try
    catch "Exception" .block: xno_more_messages
        return
    .end
    enter .block: xreceiver
        receive %1 500ms
        print (strstore %2 "")
        leave
    .end

    echo %1

    frame %0
    tailcall prefixed_printer/1

    return
.end
.function: printer/1
    try
    catch "Exception" .block: no_more_messages
        return
    .end
    enter .block: receiver
        receive %1 500ms
        leave
    .end

    echo %1

    frame %0
    tailcall prefixed_printer/1

    return
.end

.function: get_format_for_number_of_bottles/1
    arg (.name: %iota how_many_bottles) 0

    .name: %iota bottles_format_string
    if (eq int64 %iota %how_many_bottles (istore %iota 1)) +1 +3
    strstore %bottles_format_string "#{0} bottles on the wall,\nTake one down, pass it around,\nNo more bottles on the wall.\n"
    jump +2
    strstore %bottles_format_string "#{0} bottles on the wall,\nTake one down, pass it around,\n#{1} bottles on the wall.\n"

    move %0 %bottles_format_string
    return
.end
.function: format_bottle_string/1
    arg (.name: %iota how_many_bottles) 0

    frame ^[(param %iota %how_many_bottles)]
    call (.name: %iota bottles_format_string) get_format_for_number_of_bottles/1

    copy (.name: %iota current_number_of_bottles) how_many_bottles
    idec (copy (.name: %iota one_less) current_number_of_bottles)
    vec (.name: %iota format_params) current_number_of_bottles 2

    frame ^[(param %iota %bottles_format_string) (param %iota %format_params)]
    move %0 (msg %bottles_format_string format/)

    .mark: epilogue
    return
.end
.function: bottles_on_the_wall/2
    arg (.name: %iota how_many_bottles) 0
    arg (.name: %iota printer_pid) 1

    frame ^[(pamv %iota %how_many_bottles)]
    send %printer_pid (call (.name: %iota formatted_bottles) format_bottle_string/1)

    return
.end

.function: spawner/2
    arg (.name: %iota how_many_bottles) 0
    arg (.name: %iota printer_pid) 1

    if %how_many_bottles +1 no_more_bottles
    frame ^[(param %0 %how_many_bottles) (param %1 %printer_pid)]
    process void bottles_on_the_wall/2

    frame ^[(pamv %iota (idec %how_many_bottles)) (pamv %iota %printer_pid)]
    tailcall spawner/2

    .mark: no_more_bottles
    return
.end

.function: main/2
    arg (.name: %iota argv) 1

    stoi (vpop (.name: %iota how_many_bottles) argv)

    frame ^[(param %0 %how_many_bottles)]
    process (.name: %iota printer_pid) printer/1

    frame ^[(param %0 %printer_pid)]
    msg void detach/1

    frame ^[(param %iota %how_many_bottles) (param %iota %printer_pid)]
    call void spawner/2

    izero %0 local
    return
.end
