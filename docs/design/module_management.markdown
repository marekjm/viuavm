# Module management under Viua VM

Viua VM uses a simple environment variable-based module management scheme. The
scheme is described by this document.

----

## Module search path

Viua looks for libraries in these directories (in the given order):

- `~/.local/lib/viuavm{version}`
- `/usr/local/lib/viuavm{version}`
- `/usr/lib/viuavm{version}`

To suppress or replace system directories use `VIUA_SYSTEM_LIBRARY_PATH`
environment variable. When it is set Viua VM will use directories specified in
this environment variable instead of the default system directories.

Viua VM also looks for modules in directories specified in the
`VIUA_LIBRARY_PATH` environment variable. Directories specified by this
environment variable take precedence over system directories.

----

## Importing modules from specific files

If a module needs to be imported from a specific location it should be specified
inside `VIUA_MODULE_PATH` environment variable like this:

```
VIUA_MODULE_PATH=some::module=/opt/lib/viuavm/some/module.so,other::module=/tmp/foo.module
```

The semi-abstract syntax for `VIUA_MODULE_PATH` is:

```
( module-name "=" module-path ( "," module-name "=" module-path )* )?
```

----

## Preloading modules

If a module needs to be preloaded before the program starts use the
`VIUA_MODULE_PRELOAD` environment variable. It is a command-separated list of
modules that will be preloaded.
