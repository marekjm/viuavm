.import: [[dynamic]] std::posix::network
.signature: std::posix::network::socket/0
.signature: std::posix::network::bind/3
.signature: std::posix::network::listen/2
.signature: std::posix::network::accept/1

.function: [[no_sa]] reader_of_messages/1
    allocate_registers %4 local

    .name: iota client_sock
    move %client_sock local %0 parameters

    .name: iota buffer_size
    .name: iota req
    integer %buffer_size 1024
    io_read %req local %client_sock local %buffer_size local
    print %req local

    io_wait %0 local %req local 10s
    print %0 local

    return
.end

.function: main/1
    allocate_registers %7 local

    .name: iota sock
    .name: iota server_addr
    .name: iota server_port
    .name: iota tmp

    ; 1/ Create the socket.
    frame %0
    call %sock local std::posix::network::socket/0
    print %sock local

    ; 2/ Store the address.
    string %server_addr local "127.0.0.1"
    print %server_addr local

    ; 3/ Store the port.
    move %tmp local %0 parameters
    integer %server_port local 1
    vat %server_port local %tmp local %server_port local
    stoi %server_port local *server_port local
    print %server_port local

    ; 4/ Bind the socket and mark it as listening for connections.
    frame %3
    move %0 arguments %sock local
    move %1 arguments %server_addr local
    move %2 arguments %server_port local
    call %sock local std::posix::network::bind/3

    .name: iota backlog
    integer %backlog local 1
    frame %2
    move %0 arguments %sock local
    move %1 arguments %backlog local
    call %sock local std::posix::network::listen/2

    ; 5/ Accept a connection and read a message.
    .name: iota client_sock
    frame %1
    ptr %tmp local %sock local
    print %tmp local
    move %0 arguments %tmp local
    call %client_sock local std::posix::network::accept/1
    print %client_sock local

    frame %1
    move %0 arguments %client_sock local
    process void reader_of_messages/1

    izero %0 local
    return
.end
