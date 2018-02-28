# Viua VM coding style

----

## Naming

Longer, more descriptive variable names are an accepted tradeoff for shorter lines.
It's better to give variables hilariously descriptive names than to wonder "WTF does
this code do" some time later.

Function names
--------------

    auto does_something(int const) -> int;

Always use trailing return type.
Always use all-lowercase snake\_case for names of functions.

Class and struct names
----------------------

    struct A_type;
    class TCP_IP_packet;
    class XML_HTTP_request;

Always use snake\_case with Capitalised first word; this avoid unreadable messes
such as `TCPIPPacket`, or inconsistencies such as `XMLHttpRequest`.

Namespace names
---------------

    namespace network_stack {
        namespace tcp_ip {
        }
    }

Always use all-lowercase snake\_case.

Variable and parameter names
----------------------------

    auto tcp_ip_packet = TCP_IP_packet{};
    auto const listening_port = int{4242};

Always use all-lowercase snake\_case.
(Almost) Always use auto (if possible).
(Almost) Always use brace-initialisation.

Constant names
--------------

    auto const AVOGADRO_NUMBER = 6.02;  // constant in global, namespace, or class scope
    auto const other_constant = 3.14;   // constant in local scope

Always use UPPER\_SNAKE\_CASE for global-ish constants.
Apply rules for variables for local constants.

----

## Indentation

Ident by four spaces.

This is good:

    if (something) {
        // do stuff
    }

This is not:

    if (something) {
      // do stuff
    }
    if (something) {
          // do stuff
    }

----

## Braces

Put opening braces on the same line as their ifs, whiles, class and function signatures, etc.

This is good:

    auto f(int const x) {

This is not:

    auto f(int const x)
    {

----

## When in doubt...

An appropriate `.clang-format` file is included in the repository.
Use it to format your code before commiting; it will enforce correct indentation, brace placement, etc.
Documentation for `clang-format`: https://clang.llvm.org/docs/ClangFormat.html
