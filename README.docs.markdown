# Viua VM opcode documentation

This will display documentation for all opcodes:

```
]$ ./view.py
```

This will display documentation for opcode `opcode`:

```
]$ ./view.py opcode
```

This will display documentation for opcodes `some` and `opcode`:

```
]$ ./view.py some opcode
```

This will display documentation for opcode group `ctors` (note the colon after "ctors"):

```
]$ ./view.py ctors:
```

----

Requires Python 3.
Requires [Colored](https://pypi.python.org/pypi/colored/) library for colorisation.
