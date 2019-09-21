# POSIX systems call implementing I/O 

Obtaining and closing file descriptors:

- open(2)
- close(2)

I/O operations on files:

- write(2)
- writev(2)
- read(2)
- readv(2)

I/O operations on sockets:

- send(2)
- sendto(2)
- sendmsg(2)
- recv(2)
- recvfrom(2)
- recvmsg(2)

Basic polling:

- poll(2)
- select(2)

Sockets:

- socket(2), socket(7)
- connect(2)
- bind(2)
- listen(2)
- accept(2)
- socketpair(2)
- getsockname(2)
- getpeername(2)
- getsockopt(2), setsockopt(2)
- shutdown(2)

Nonblocking I/O:

- fcntl(2)

Man pages:

- socket(7)
- aio(7)

--------------------------------------------------------------------------------

# I/O handles

Handle acts as a generic object that interacts in the outside world in any way.

- file
- socket
- pipe
- directory

--------------------------------------------------------------------------------

# I/O ports

An I/O port is something which can be used to read bytes from, and to write
bytes to. Some ports may also support seeking.

- file
- socket
- pipe

The abstraction is very simple: the user program interacts with the port, and
the port interacts with the outside world (acts as a sort of proxy between VM
world and the outside world). This allows specially-crafted ports to use
something else than plain byte vectors to communicate with the user program
running on Viua VM; for example, a port may perform serialisation and
deserialisation of data.

A port must specify that it is not a "dumb pipe" if it wants to perform
additional processing on data that passes through it. Also, the processing is
scheduled on a different scheduler than the regular I/O scheduler to avoid
accidental starvation of "dumb pipes" by a misbehaving port.

A pipeline for writing could look like this:

- user program opens a port
- user program writes a data structure to the port...
- the port serialises the data structure to an on-the-wire format
- the port writes the serialised form to the file descriptor
- ...user program continues

A pipeline for reading could look like this:

- user program opens a port
- user program reads a data structure from the port...
- the port reads some bytes from the file descriptor
- the port deserialises the data from on-the-wire format to a data structure
- ...user program continues

If the port is just a "dumb pipe" then the serialisation/deserialisation step is
removed and it's just bytes that go through that port.

The question now is: should 

--------------------------------------------------------------------------------

# I/O operations

Not all ports and handles must support all operations. Some ports may be
read-only (e.g. a source of random bytes) or write-only (e.g. the standard
output stream).

- read (recv): reads bytes from a port
- write (send): writes bytes to a port
- seek: sets the stream pointer position
- tell: reports the stream pointer position
- op: operation determined by the opcode, used (opcode, struct) pair to tell the
  port what needs to be done

--------------------------------------------------------------------------------

# First things first

Allow opening, reading from, and writing to files. That's a priority. Then do
the same but for sockets.

Only then think about how to include opening stuff like directories in this.
