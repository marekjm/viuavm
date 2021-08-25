# Viua VM

--------------------------------------------------------------------------------

## Transparent execution

Linux uses `binfmt_misc` loader to provide support for various executable file
formats. See https://www.kernel.org/doc/html/latest/admin-guide/binfmt-misc.html
for more information.

Viua VM binaries can be executed "transparently" (ie, without explicitly passing
them to the interpreter) using this infrastructure. For one-time testing you can
use the following command, executed as root:

    # cat ./binfmt.d/viua-exec.conf > /proc/sys/fs/binfmt_misc/register

For a more permanent solution install the file in /usr/local/lib/binfmt.d or
consult `binfmt.d(5)` for information about appropriate installation directory
on your system. Remember to check and adjust the interpreter's location if
necessary!