.import: [[dynamic]] viuapq
.import: [[dynamic]] std::os

.signature: viuapq::connect/1
.signature: viuapq::finish/1
.signature: viuapq::get/2
.signature: viuapq::get_one/2
.signature: std::os::system/1


; Funkcje pomocznicze ogólnego przeznaczenia.
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
.function: for_each/4
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

    vat %item local %vec local %n local
    frame %1
    move %0 arguments %item local
    call void %fn local

    iinc %n local
    lt %r0 local %n local %limit local
    if %r0 local next_iteration the_end

    .mark: next_iteration
    frame %4
    move %0 arguments %vec local
    move %1 arguments %fn local
    move %2 arguments %n local
    move %3 arguments %limit local
    tailcall for_each/4

    .mark: the_end
    move %r0 local %vec local
    return
.end
.function: for_each/2
    allocate_registers %4 local

    .name: 0 r0
    .name: iota vec
    .name: iota n
    .name: iota limit

    move %vec local %0 parameters
    izero %n local
    vlen %limit local %vec local

    eq %r0 local %n local %limit local
    if %r0 local empty_vector vector_ok

    .mark: vector_ok
    frame %4
    move %0 arguments %vec local
    move %1 arguments %1 parameters
    move %2 arguments %n local
    move %3 arguments %limit local
    tailcall for_each/4

    .mark: empty_vector
    return
.end
.function: for_each_nx/5
    allocate_registers %6 local

    .name: 0 r0
    .name: iota vec
    .name: iota fn
    .name: iota data
    .name: iota n
    .name: iota limit

    move %vec local %0 parameters
    move %fn local %1 parameters
    move %data local %2 parameters
    move %n local %3 parameters
    move %limit local %4 parameters

    frame %3
    copy %0 arguments %n local
    ptr %r0 local %vec local
    move %1 arguments %r0 local
    copy %2 arguments %data local
    call void %fn local

    iinc %n local
    lt %r0 local %n local %limit local
    if %r0 local next_iteration the_end

    .mark: next_iteration
    frame %5
    move %0 arguments %vec local
    move %1 arguments %fn local
    move %2 arguments %data local
    move %3 arguments %n local
    move %4 arguments %limit local
    tailcall for_each_nx/5

    .mark: the_end
    move %r0 local %vec local
    return
.end
.function: for_each_nx/3
    allocate_registers %6 local

    .name: 0 r0
    .name: iota vec
    .name: iota fn
    .name: iota data
    .name: iota n
    .name: iota limit

    move %vec local %0 parameters
    move %fn local %1 parameters
    move %data local %2 parameters

    izero %n local
    vlen %limit local %vec local

    eq %r0 local %n local %limit local
    if %r0 local empty_vector vector_ok

    .mark: vector_ok
    frame %5
    move %0 arguments %vec local
    move %1 arguments %fn local
    move %2 arguments %data local
    move %3 arguments %n local
    move %4 arguments %limit local
    tailcall for_each_nx/5

    .mark: empty_vector
    return
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

    string %tmp local "\033[?25l"
    print %tmp local

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

    string %command local "\033[?25h"
    print %command local

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

    atom %key local 'name'
    structremove %value local %order local %key local
    text %value local %value local
    structinsert %order local %key local %value local

    move %r0 local %order local
    return
.end
.function: orders_by_id/2
    allocate_registers %4 local

    .name: 0 r0
    .name: iota query
    .name: iota connection
    .name: iota id

    move %connection local %0 parameters
    move %id local %1 parameters

    ; Wiem, że sklejanie zapytań ręcznie, bez jakiejkolwiek weryfikacji to
    ; kiepski pomysł, ale 1/ tutaj robię demo 2/ nie mam porządnej biblioteki do
    ; interakcji z PostreSQL.
    text %query local "select * from orders, customers where orders.id = "
    text %id local %id local
    textconcat %query local %query local %id local

    print %query local
    print %connection local

    frame %2
    move %0 arguments %connection local
    move %1 arguments %query local
    call %query local viuapq::get/2

    izero %r0 local
    vpop %query local %query local %r0 local
    print %query local

    frame %1
    move %0 arguments %query local
    call %r0 local order_of_pq/1

    print %r0 local

    return
.end
.function: orders_all/1
    allocate_registers %2 local

    .name: 0 r0
    .name: iota query

    text %query local "select orders.id, customer, employee, order_date, customers.name from orders where orders.customer = customers.id"
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
.function: orders_all_count/1
    allocate_registers %2 local

    .name: 0 r0
    .name: iota query

    text %query local "select count(*) as x from orders"
    frame %2
    move %0 arguments %0 parameters
    move %1 arguments %query local
    call %query local viuapq::get_one/2

    atom %r0 local 'x'
    structat %r0 local %query local %r0 local
    stoi %r0 local *r0 local

    return
.end
.function: orders_products_of_order_id/2
    allocate_registers %4 local

    .name: 0 r0
    .name: iota connection
    .name: iota id
    .name: iota query
    .name: iota tmp

    move %connection local %0 parameters
    move %id local %1 parameters

    text %query local "select entries.id, products.name, unit_price, discount, quantity from entries, products where entries.product_id = products.id and order_id = "
    text %id local %id local
    textconcat %query local %query local %id local

    frame %2
    move %0 arguments %connection local
    move %1 arguments %query local
    call %query local viuapq::get/2

    move %r0 local %query local
    return
.end


; Implementacja aktora prezentującego interfejs.
.function: print_top_line/1
    allocate_registers %5 local

    .name: 0 r0
    .name: iota state
    .name: iota connection
    .name: iota orders_no
    .name: iota fmt

    move %state local %0 parameters

    atom %connection local 'connection'
    structat %connection local *state local %connection local

    frame %1
    move %0 arguments %connection local
    call %orders_no local orders_all_count/1

    string %r0 local "\r┌──── General information ──────────────────────────────┐"
    print %r0 local

    text %fmt local "\r│ Total orders: "
    text %orders_no local %orders_no local
    textconcat %fmt local %fmt local %orders_no local
    print %fmt local

    string %r0 local "\r├───────────────────────────────────────────────────────┤"
    print %r0 local

    return
.end
.function: print_bottom_line/1
    allocate_registers %1 local

    .name: 0 r0
    string %r0 local "\r└───────────────────────────────────────────────────────┘\r"
    print %r0 local

    return
.end
.function: format_order_list_entry/2
    allocate_registers %6 local

    .name: 0 r0
    .name: iota item
    .name: iota highlighted
    .name: iota tmp
    .name: iota key
    .name: iota value

    move %item local %0 parameters
    move %highlighted local %1 parameters

    text %r0 local "\r│ ["

    atom %key local 'id'
    structat %value local *item local %key local

    integer %tmp local 10
    lt %tmp local *value local %tmp local
    if %tmp local needs_leading_space no_leading_space

    .mark: needs_leading_space
    text %tmp local " "
    textconcat %r0 local %r0 local %tmp local

    .mark: no_leading_space
    text %value local *value local
    text %tmp local "] "
    textconcat %r0 local %r0 local %value local
    textconcat %r0 local %r0 local %tmp local

    if %highlighted local enable_highlighting no_highlighting

    .mark: enable_highlighting
    text %tmp "\033[1m"
    textconcat %r0 local %r0 local %tmp local
    text %tmp "\033[4m"
    textconcat %r0 local %r0 local %tmp local

    .mark: no_highlighting
    atom %key local 'customer'
    structat %value local *item local %key local
    text %value local *value local
    text %tmp local " / "
    textconcat %r0 local %r0 local %value local
    textconcat %r0 local %r0 local %tmp local

    atom %key local 'order_date'
    structat %value local *item local %key local
    text %value local *value local
    textconcat %r0 local %r0 local %value local

    return
.end
.function: view_actor_list_orders_print_entry/3
    allocate_registers %7 local

    .name: 0 r0
    .name: iota each
    .name: iota data
    .name: iota state
    .name: iota pointer
    .name: iota key
    .name: iota control_sequence

    move %r0 local %0 parameters
    move %data local %1 parameters
    move %state local %2 parameters

    atom %key local 'pointer'
    structat %pointer local *state local %key local
    eq %pointer local %r0 local *pointer local

    vat %each local *data local %r0 local

    frame %2
    move %0 arguments %each local
    move %1 arguments %pointer local
    call %each local format_order_list_entry/2
    print %each local

    string %control_sequence "\033[0m\r"
    echo %control_sequence local

    ;move %each local %0 parameters
    ;text %each local *each local

    ;text %tmp local "\r⇝ "
    ;textconcat %each local %tmp local %each local
    ;text %tmp local "\r"
    ;textconcat %each local %each local %tmp local
    ;print %each local

    return
.end
.function: view_actor_list_orders/1
    allocate_registers %5 local

    .name: 0 r0
    .name: iota state
    .name: iota key
    .name: iota data
    .name: iota fn

    move %state local %0 parameters

    atom %key local 'data'
    structat %data local *state local %key local

    frame %3
    copy %0 arguments *data local
    function %fn local view_actor_list_orders_print_entry/3
    move %1 arguments %fn local
    move %2 arguments %state local
    call void for_each_nx/3

    return
.end
.function: print_order_entry/1
    allocate_registers %6 local

    .name: 0 r0
    .name: iota entry
    .name: iota key
    .name: iota value
    .name: iota fmt
    .name: iota tmp

    move %entry local %0 parameters

    text %fmt local "\r│ ["

    atom %key local 'id'
    structat %value local *entry local %key local
    text %value local *value local
    textconcat %fmt local %fmt local %value local

    text %tmp local "] \t"
    textconcat %fmt local %fmt local %tmp local

    atom %key local 'name'
    structat %value local *entry local %key local
    text %value local *value local
    textconcat %fmt local %fmt local %value local

    text %tmp local ": "
    textconcat %fmt local %fmt local %tmp local

    atom %key local 'unit_price'
    structat %value local *entry local %key local
    text %value local *value local
    textconcat %fmt local %fmt local %value local

    text %tmp local " (discount: "
    textconcat %fmt local %fmt local %tmp local

    atom %key local 'discount'
    structat %value local *entry local %key local
    text %value local *value local
    textconcat %fmt local %fmt local %value local

    text %tmp local ") x "
    textconcat %fmt local %fmt local %tmp local

    atom %key local 'quantity'
    structat %value local *entry local %key local
    text %value local *value local
    textconcat %fmt local %fmt local %value local

    print %fmt local

    return
.end
.function: view_actor_single_order_impl/1
    allocate_registers %13 local

    .name: 0 r0
    .name: iota state
    .name: iota message
    .name: iota key
    .name: iota got_tag
    .name: iota tag_shutdown
    .name: iota tmp
    .name: iota selected_order
    .name: iota customer
    .name: iota order_date
    .name: iota order_id
    .name: iota connection
    .name: iota order_entries

    string %tmp "\033[2J"
    echo %tmp local
    string %tmp "\033[1;1H"
    echo %tmp local

    move %state local %0 parameters

    ; Prezentacja widoku pojedynczego zamówienia.
    atom %selected_order local 'selected_order'
    structat %selected_order local *state local %selected_order local

    atom %key local 'customer'
    structat %customer local *selected_order local %key local
    text %customer local *customer local

    atom %key local 'order_date'
    structat %order_date local *selected_order local %key local
    text %order_date local *order_date local

    atom %key local 'id'
    structat %order_id local *selected_order local %key local
    text %order_id local *order_id local

    text %tmp local "\r┌──── General information ──────────────────────────────┐"
    print %tmp local

    text %tmp local "\r│ Order ID:   "
    textconcat %r0 local %tmp local %order_id local
    print %r0 local

    text %tmp local "\r│ Order date: "
    textconcat %r0 local %tmp local %order_date local
    print %r0 local

    text %tmp local "\r│ Customer:   "
    textconcat %tmp local %tmp local %customer local
    print %tmp local

    text %tmp local "\r├──── Order positions ──────────────────────────────────┤"
    print %tmp local

    atom %key local 'connection'
    structat %connection local *state local %key local

    atom %key local 'id'
    structat %order_id local *selected_order local %key local

    frame %2
    move %0 arguments %connection local
    copy %1 arguments *order_id local
    call %order_entries local orders_products_of_order_id/2

    frame %2
    move %0 arguments %order_entries local
    function %tmp local print_order_entry/1
    move %1 arguments %tmp local
    call void for_each/2

    text %tmp local "\r└───────────────────────────────────────────────────────┘"
    echo %tmp local
    string %tmp local "\033[1A\r"
    print %tmp local

    ; Po prezentacji widoku, oczekuj na akcje użytkownika.
    receive %message local infinity

    atom %tag_shutdown local 'shutdown'
    atom %key local 'tag'
    structat %got_tag local %message local %key local

    atomeq %tag_shutdown local *got_tag local %tag_shutdown local

    if %tag_shutdown local stage_shutdown swallow_any_message

    .mark: swallow_any_message
    ; "Zjedz" dowolną wiadomość, która nie jest użyteczna i wywołaj jeszcze raz
    ; siebie podmieniając ramkę na stosie. W ten sposób przejmujemy pętlę
    ; zdarzeń w aktorze implementującym konkretny widok interfejsu i nie wracamy
    ; do aktora implementującego główny widok.
    ;
    ; Jest to sztuczka, ale działa.
    frame %1
    move %0 arguments %state local
    tailcall view_actor_single_order/1

    .mark: stage_shutdown
    ; Wróć do funkcji wywołującej oddając jej kontrolę nad interfejsem.
    return
.end
.function: view_actor_single_order/1
    allocate_registers %6 local

    .name: 0 r0
    .name: iota state
    .name: iota key
    .name: iota order
    .name: iota data
    .name: iota connection

    move %state local %0 parameters

    ; Potrzebujemy wiedzieć na jakim miejscu listy znajdował się wskaźnik żeby
    ; pobrać dane właściwego zamówienia.
    atom %key local 'pointer'
    structat %order local *state local %key local

    ; Następnie potrzebujemy wyciągnąć dane...
    atom %key local 'data'
    structat %data local *state local %key local

    ; ...pobrać odpowiednią pozycję na liście...
    vat %order local *data local *order local

    ; ...i wyłuskać z niej ID.
    atom %key local 'id'
    structat %order local *order local %key local
    copy %order local *order local

    ; Kiedy mamy już ID potrzebujemy pobrać odpowiadające mu zamówienie.
    atom %key local 'connection'
    structat %connection local *state local %key local

    frame %2
    move %0 arguments %connection local
    move %1 arguments %order local
    call %order local orders_by_id/2

    atom %key local 'selected_order'
    structinsert *state local %key local %order local

    frame %1
    move %0 arguments %state local
    tailcall view_actor_single_order_impl/1
.end
.function: view_actor_impl/1
    allocate_registers %13 local

    .name: 0 r0
    .name: iota state
    .name: iota message
    .name: iota data
    .name: iota control_sequence
    .name: iota tmp
    .name: iota key
    .name: iota got_tag
    .name: iota tag_shutdown
    .name: iota tag_order_list
    .name: iota tag_ptr_down
    .name: iota tag_ptr_up
    .name: iota tag_enter

    move %state local %0 parameters

    receive %message local infinity

    atom %tag_shutdown local 'shutdown'
    atom %tag_order_list local 'order_list'
    atom %tag_ptr_down local 'pointer_down'
    atom %tag_ptr_up local 'pointer_up'
    atom %tag_enter local 'enter'
    atom %key local 'tag'
    structat %got_tag local %message local %key local

    atomeq %tag_shutdown local *got_tag local %tag_shutdown local
    atomeq %tag_order_list local *got_tag local %tag_order_list local
    atomeq %tag_ptr_down local *got_tag local %tag_ptr_down local
    atomeq %tag_ptr_up local *got_tag local %tag_ptr_up local
    atomeq %tag_enter local *got_tag local %tag_enter local

    if %tag_shutdown local stage_shutdown +1
    if %tag_order_list local stage_order_list +1
    if %tag_ptr_down local stage_ptr_down +1
    if %tag_ptr_up local stage_ptr_up +1
    if %tag_enter local stage_enter +1

    text %tmp local "got weird message: "
    text %message local %message local
    textconcat %tmp local %tmp local %message local
    print %tmp local
    jump the_end

    .mark: stage_shutdown
    text %tmp "view actor shutting down\r"
    print %tmp local
    return

    .mark: stage_order_list
    atom %key local 'data'
    structremove %message local %message local %key local
    structinsert %state local %key local %message local
    jump pointer_bounds_check

    .mark: stage_enter
    frame %1
    ptr %r0 local %state local
    move %0 arguments %r0 local
    call void view_actor_single_order/1
    jump printing_sequence

    .mark: stage_ptr_down
    atom %key local 'pointer'
    structat %tmp local %state local %key local
    iinc *tmp local
    jump pointer_bounds_check

    .mark: stage_ptr_up
    atom %key local 'pointer'
    structat %tmp local %state local %key local
    idec *tmp local
    ; Ten skok jest niepotrzebny, ale trzymajmy go dla czytelności.
    ; jump pointer_bounds_check

    .mark: pointer_bounds_check
    atom %key local 'data'
    structat %data local %state local %key local

    atom %key local 'pointer'
    structat %tmp local %state local %key local
    frame %1
    copy %0 arguments *tmp local
    call %tmp local lower_bound_to_zero/1

    frame %2 local
    move %0 arguments %tmp local
    vlen %tmp local *data local
    idec %tmp local
    move %1 arguments %tmp local
    call %tmp local min/2

    structinsert %state local %key local %tmp local

    .mark: printing_sequence
    string %control_sequence "\033[2J"
    echo %control_sequence local
    string %control_sequence "\033[1;1H"
    echo %control_sequence local

    frame %1 local
    ptr %tmp local %state local
    move %0 arguments %tmp local
    call void print_top_line/1

    frame %1
    ptr %tmp local %state local
    move %0 arguments %tmp local
    call void view_actor_list_orders/1

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

    receive %input_actor local 60s

    struct %state local

    atom %key local 'pointer'
    izero %value local 0
    structinsert %state local %key local %value local

    atom %key local 'data'
    vector %value local
    structinsert %state local %key local %value local

    atom %key local 'connection'
    move %value local %0 parameters
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
.function: make_none_message/0
    allocate_registers %2 local

    .name: iota tag
    atom %tag local '_'

    frame %1
    move %0 arguments %tag local
    call %0 local make_tagged_message_impl/1
    return
.end
.function: make_order_list_message/1
    allocate_registers %5 local

    .name: iota message
    .name: iota tag
    .name: iota data
    .name: iota key

    atom %tag local 'order_list'
    frame %1
    move %0 arguments %tag local
    call %message local make_tagged_message_impl/1

    ; insert the data field: { tag: 'order_list', data: ... }
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
.function: make_enter_message/0
    allocate_registers %2 local

    .name: iota tag
    atom %tag local 'enter'

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
        io_wait %buf local %req local 100ms
        leave
    .end

    move %r0 local %buf local
    return
.end
.function: is_it_time_to_shut_down_input/0
    allocate_registers %2 local

    .name: 0 r0
    .name: iota message

    try
    catch "Exception" .block: is_it_time_to_shut_down_input__nope
        draw void
        izero %message local
        leave
    .end
    enter .block: is_it_time_to_shut_down_input__maybe
        receive %message local 0ms
        leave
    .end

    move %r0 local %message local
    return
.end
.function: input_actor_impl/1
    allocate_registers %14 local

    .name: iota tree_view_actor
    .name: iota tmp
    .name: iota buf

    .name: iota c_quit
    .name: iota c_enter
    .name: iota c_pointer_down
    .name: iota c_pointer_up

    .name: iota time_to_shut_down

    move %tree_view_actor local %0 parameters

    frame %0
    call %time_to_shut_down local is_it_time_to_shut_down_input/0
    if %time_to_shut_down local the_end await_input_stage

    .mark: await_input_stage
    frame %0
    call %buf local input_actor_await_io/0

    string %c_quit local "q"
    streq %c_quit local %buf local %c_quit local
    string %c_enter local "\r"
    streq %c_enter local %buf local %c_enter local
    string %c_pointer_down local "j"
    streq %c_pointer_down local %buf local %c_pointer_down local
    string %c_pointer_up local "k"
    streq %c_pointer_up local %buf local %c_pointer_up local

    if %c_quit local quit_view +1
    if %c_enter local enter_item +1
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

    .mark: enter_item

    frame %0
    call %tmp make_enter_message/0
    send %tree_view_actor local %tmp local

    jump happy_loopin

    .mark: quit_view
    frame %0
    call %tmp local make_shutdown_message/0
    send %tree_view_actor local %tmp local

    jump happy_loopin

    .mark: the_end
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

    frame %1
    move %0 arguments %0 parameters
    tailcall input_actor_impl/1
.end


.function: finish_pq_connection/1
    allocate_registers %2 local

    .name: iota tmp

    text %tmp local "\n\rfinishing PostgreSQL connection"
    print %tmp local

    frame %1
    move %0 arguments %0 parameters
    call void viuapq::finish/1

    return
.end
.function: send_shutdown_to_input_actor/1
    allocate_registers %2 local

    .name: iota input_actor

    move %input_actor local %0 parameters

    integer %0 local 1
    send %input_actor local %0 local
    join void %input_actor local

    return
.end
.function: main/0
    allocate_registers %5 local

    .name: 0 r0
    .name: iota connection
    .name: iota view_actor
    .name: iota input_actor
    .name: iota query

    frame %0
    defer return_tty_to_sanity/0

    text %connection local "dbname = pt_lab2"

    frame %1
    move %0 arguments %connection local
    call %connection local viuapq::connect/1

    frame %1
    ptr %r0 local %connection local
    move %0 arguments %r0 local
    call %query local orders_all/1

    frame %1
    move %0 arguments %query local
    call %query local make_order_list_message/1

    frame %1
    move %0 arguments %connection local
    process %view_actor local view_actor/1

    frame %1
    copy %0 arguments %view_actor local
    process %input_actor local input_actor/1

    copy %r0 local %input_actor local
    send %view_actor local %r0 local

    frame %1
    move %0 arguments %input_actor local
    defer send_shutdown_to_input_actor/1

    send %view_actor local %query local

    ; Połączenie z bazą danych nie jest nam już dłużej potrzebne w tym miejscu
    ; więc zlećmy jego automatyczne zamknięcie.
    ;frame %1
    ;move %0 arguments %connection local
    ;defer finish_pq_connection/1

    ;frame %1
    ;ptr %r0 local %connection local
    ;move %0 arguments %r0 local
    ;call %query local orders_all/1
    ;print %query local

    join void %view_actor local

    izero %0 local
    return
.end
