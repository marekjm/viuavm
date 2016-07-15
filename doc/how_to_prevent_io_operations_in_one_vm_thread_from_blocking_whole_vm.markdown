# How to prevent I/O operations in one vm-thread from blocking whole VM?

Since Viua runs in a single process, and threads visible by Viua programs are
simulated blocking I/O operations performed by one vm-thread would block any
other threads.

*Figure 1. "One thread running all operations"*

Character `*` means that thread is running.

```
CPU thread #0 **************                                                      ******
CPU thread #1              |**                     *******                        |     
CPU thread #2                 \                   /      |***********             |     
CPU thread #3                  \                 /                  |*************|     
                                \               /
I/O                              ~~~~~~~~~~~~~~~
```

How to prevent that?

In real machines, when a CPU sees that a process is about to perform a blocking
operation they put it to sleep until the operation finishes, and immediately
switch to another process if there are any processes waiting for CPU.
Such behaviour prevents wasting CPU time waiting for I/O to complete.

It would be beneficial performance-wise if Viua employed a similar scheme.

One solution would be to put I/O operations in a different thread than CPU
operations.

*Figure 2. "One thread running I/O operations, and one running VMs CPU"*

Character `*` means that thread is running.

```
CPU thread #0 **************                                     ***********
CPU thread #1              |** -- blocked by I/O -- *******      |         |************
CPU thread #2                 \***********         /      |      |                     |**
CPU thread #3                  \         |********/       |******|
                                \                /
I/O                              ~~~~~~~~~~~~~~~~
```

As show on Fig. 2 the CPU blocked a thread waiting on I/O and switched
to another one waiting to be executed.

The mechanism could be implemented as such:

- CPU executes vmthread t0,
- vmthread t0 wants to perform blocking I/O operation,
- VM puts the I/O request of t0 in the queue on I/O thread, marks t0 as suspended, and
  puts it to sleep,
- CPU switches to the next vmthread awaiting execution,
- CPU does not continue execution of t0 until the I/O thread does not finish the request t0
  wanted performed,
- when the I/O thread finishes operation requested by t0, it notifies t0 about the result of
  performed operation and unblocks t0 so scheduler can pick t0 up for execution,
- I/O threads processes further requests,
- t0 processes the result of I/O operation (possibly handling an exception) and continues execution,

---

#### Copyright (C) 2015, 2016 Marek Marecki

This documentation file is part of Viua VM, and
published under GNU GPL v3 or later.

You should have received a copy of the GNU General Public License
along with Viua VM.  If not, see <http://www.gnu.org/licenses/>.
