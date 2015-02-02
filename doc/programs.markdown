# Wudoo VM programs

Every program is composed of functions.
Every file must contain at least one function.

Entry point if `main` function unless assembler is told to set first instruction pointer to another function.
Main function must be defined.
Main function must not be empty.
Code is generated to automaticaly create a frame for main function and call it at startup.

If any code exists outside of functions, special function is created which contains that code.
If this special function is created, assembler will set it as entry point.
If this special function is created, `main` (or any other function set as main) is not automaticaly called.
It is programmers responsibility (*note*: careful, this document was last updated for version 0.2.4) to call
main function manually.
