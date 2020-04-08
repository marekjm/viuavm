.import: [[dynamic]] viuapq

.signature: viuapq::connect/1
.signature: viuapq::finish/1
.signature: viuapq::get/2

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

.function: main/0
    allocate_registers %3 local

    .name: 0 r0
    .name: iota connection
    .name: iota query

    text %connection local "dbname = pt_lab2"

    frame %1
    move %0 arguments %connection local
    call %connection local viuapq::connect/1

    frame %1
    ptr %r0 local %connection local
    move %0 arguments %r0 local
    call %query local employees_all/1
    print %query local

    frame %1
    ptr %r0 local %connection local
    move %0 arguments %r0 local
    call %query local orders_all/1
    print %query local

    frame %1
    move %0 arguments %connection local
    call void viuapq::finish/1

    izero %0 local
    return
.end
