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
    class XML_HTTP_request;
    enum class An_enum;

Always use `Snake_case` with capitalised first word; this avoids unreadable
messes such as `TCPIPPacket`, or inconsistencies such as `XMLHttpRequest`.

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

Always use all-lowercase `snake_case`.

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

----

## Automatic formatting

An appropriate `.clang-format` file is included in the repository. Use it to
format your code before commiting; it will enforce correct indentation, brace
placement, etc.

Documentation for `clang-format`: https://clang.llvm.org/docs/ClangFormatStyleOptions.html
