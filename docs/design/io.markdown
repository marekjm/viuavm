# I/O operations of Viua VM

The I/O model should be simple. All values that facilitate I/O should subscribe
to this model to provide a uniform interface to the programmer.

The high-level overview is short. There are I/O "handles" (or "ports") through
which the I/O is performed. The actual I/O actions (e.g. file read, socket
write, etc.) are carried out as a result of an "operation" being requested from
the I/O handle (or port). Executing an I/O instruction specifying which
operation should the port perform does not mean that the operation is carried
out immediately - it is merely scheduled, and the user code receives a value
representing the I/O "request" that the I/O scheduler will eventually perform.
All I/O operations are non-blocking. User programs may then wait for the request
to be completed. For every wait a timeout is specified so that the program never
waits indefinitely for the request to finish (unless it gives a timeout value of
"inifinity"). A request may be cancelled (if it is still scheduled; if the
scheduler started executing it it is too late to cancel that I/O request).

The reason why performing I/O should be done this way is to avoid getting into a
situation when an I/O operation may block the program, causing it to hang.

The VM provides the instructions necessary to perform all I/O operations except
for opening I/O ports.

How a program will acquite an I/O port, then? Through a platform-specific
libraries. There is a myriad of possible types of ports and I see no reasonable
way of providing a uniform way of opening them. Closing is easy: just call a
`close()` function on them, so this can supplied as an instruction: `io_close`.
Performing I/O through a port can also be reduced to reads, writes, and seeks.
It may be limiting, but pretty much every I/O interaction can be reduced to
these primitives and I would like to not clutter the ISA (at least not until I
see patterns emerging that would be useful enough to be given their own
instructions).

----

The platform supplies the means of obtaining an "I/O handle". An I/O handle is
a value which may be a target of the I/O instructions, and provides I/O services
in a way that is consistent with what Viua VM presents to programs that it runs.

The basic I/O operations that a handle must present are:

- read: read a buffer from the handle
- write: write a buffer to the handle
- close: close the handle

An I/O handle may present additional I/O services as means of extension of this
basic model.

I/O operations are *always* non-blocking; the control is returned to the running
program immediately after the operation is scheduled and produces an "I/O request".

An I/O request is a representation of a requested I/O operation. It may be in
one of several different states:

- scheduled: the request has been scheduled for execution
- executing: the request is currently being executed
- commited: the request has been successfully executed
- errored: the request has *not* been successfully executed
- cancelled: the request has been cancelled

The only state in which a request can be cancelled is "scheduled". After the I/O
scheduler starts executing the request it is too late to cancel it reliably.

Different kinds of I/O handles give different guarantees with regard to
atomicity of the operations they implement. Some operations may be atomic, some
may not. All operations *strive* to be atomic, though - i.e. to always either
read or write the full amount of bytes requested. In case of failure - they will
report the amount of data read or written in details of the error message.

A basic set of I/O operations presented by Viua VM in the form of instructions
is this:

- `io_read`: read data through the I/O handle
- `io_write`: write data through the I/O handle
- `io_seek`: move the data pointer of the I/O handle
- `io_close`: closes an I/O handle

A set of supporting I/O instructions presented by Viua VM is this:

- `io_cancel`: cancel a scheduled I/O operation
- `io_await`: wait for an I/O operation to complete

There is also the generic instruction that is able to schedule any of the basic
operations, while also being able to schedule any *extension* operations:

- `io_op`: schedule an I/O operation through the I/O handle

All I/O operations allow passing an "operation specification" struct allowing to
pass additional information and data to the operation, such as:

- a preallocated buffer for result of `io_read`
- a set of flags directing how to treat some errors or conditions
- a timeout after which the operation "expires" (i.e. is automatically cancelled
  by the I/O scheduler)

The specification may also contain any other data, as deemed necessary by the
designer of that particular I/O handle type.

--------------------------------------------------------------------------------

## Basic set of I/O primitives

This section lists the basic I/O primitives that should be provided by an I/O
system to allow it to emulate POSIX I/O API.

### Input primitives

#### Read

This is similar to `read(3)`. Read at most n bytes from I/O handle, and move the
position marker forward by the amount of bytes actually read (which may be less
than n).

#### Receive

This is similar to `recv(3)`.
