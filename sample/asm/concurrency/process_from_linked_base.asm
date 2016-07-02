.signature: test_module::test_function/0

.function: main/0
    link test_module

    frame 0
    process 1 test_module::test_function/0

    join 0 1

    izero 0
    return
.end
