# Viua VM coding style

----

## Naming

Longer, more descriptive variable names are an accepted tradeoff for shorter
lines. It's better to give variables hilariously descriptive names than to
wonder "WTF does this code do" some time later.

Function names
--------------

    auto does_something(int const) -> int;

Always use trailing return type.
Always use all-lowercase `snake_case` for names of functions.

Class, struct, and enum names
----------------------

    struct A_type;
    class TCP_IP_packet;
    enum class An_enum;

    struct scope_guard;
    class big_endian;

Always use `Snake_case` with capitalised first word; this avoids unreadable
messes such as `TCPIPPacket`. For utility classes (i.e. those that could be used
outside of the VM's codebase as they are general purpose) can follow the style
used by the C++ standard library.

Namespace names
---------------

    namespace network_stack {
        namespace tcp_ip {
        }
    }

Always use all-lowercase `snake_case`.

Variable and parameter names
----------------------------

    auto tcp_ip_packet = TCP_IP_packet{};
    auto const listening_port = int{4242};

Always use all-lowercase `snake_case`. Follow the "Almost Always Auto" rule.
Make as many variables constant as possible.

Constant names
--------------

    auto const AVOGADRO_NUMBER = 6.02;  // constant in global, namespace, or class scope
    auto const other_constant = 3.14;   // constant in local scope

Always use `UPPER_SNAKE_CASE` for global-ish constants.
Apply rules for variables for local constants.

----

## Initialisation, declarations, types

(Almost) Always use auto (if possible).

    auto answer = int{42};  // good
    int answer = 42;        // bad

(Almost) Always use brace-initialisation.

    auto x = Foo{};     // good
    auto x = Foo();     // bad

Use consistent const (a.k.a. "const on the right").

    auto const n = int{42};     // good
    const auto n = int{42};     // bad

    auto f(int const x) -> int; // good
    auto f(const int x) -> int; // bad

Pointer character (`*`) is associated with the *type*, not the *name*.

    /*
     * Use viua::util::memory::dumb_ptr<T> to create "dumb pointer" types.
     */
    auto ptr = viua::util::memory::dumb_ptr<int>{}; // good
    using viua::util::memory::dumb_ptr;
    auto ptr = dumb_ptr<int>{};                     // good
    int* ptr = nullptr;                             // bad

    auto f(int const* x) -> int; // good
    auto f(const int* x) -> int; // bad

Use one declaration per line, unless using destructuring bindings.

    // good
    auto x = int{42};
    auto y = int{43};

    // also good
    auto [ x, y ] = some_fn();


Copyright information
---------------------

Always add a copyright note at the top of any file.

List years in which a change was made to the file. If there are only two years in
which the file has been changed - list them explicitly:

    Copyright (C) 2019, 2020 John Doe <john.doe@example.com>

Longer periods can be abbreviated; instead of "2017, 2018, 2019, 2020" you can
write the copyright note like this:

    Copyright (C) 2017-2020 John Doe <john.doe@example.com>

You can mix and match:

    Copyright (C) 2015, 2017-2020 John Doe <john.doe@example.com>
    Copyright (C) 2016 Jane Doe <jane.doe@example.com>

Or write it in several lines:

    Copyright (C) 2017-2020 John Doe <john.doe@example.com>
    Copyright (C) 2016 Jane Doe <jane.doe@example.com>
    Copyright (C) 2015 John Doe <john.doe@example.com>

Most recent change on top. In case several people made a change in the same
year, use alphabetic ordering.

----

## Automatic formatting

An appropriate `.clang-format` file is included in the repository. Use it to
format your code before commiting; it will enforce correct indentation, brace
placement, etc.

Documentation for `clang-format`: https://clang.llvm.org/docs/ClangFormatStyleOptions.html
