.block: handler
    print (pull 2)
    leave
.end

.block: throws_derived
    throw (new 1 Combined)
    leave
.end

.function: typesystem_setup/0
    register (class 0 SharedBase)
    register (class 1 SharedDerived)

    register (derive (class 2 BaseA) SharedBase)
    register (derive (class 3 DerivedA) BaseA)
    register (derive (derive (class 4 MoreDerivedA) DerivedA) SharedDerived)

    register (derive (class 5 BaseB) SharedBase)
    register (derive (class 6 DerivedB) BaseB)
    register (derive (derive (class 7 MoreDerivedB) DerivedB) SharedDerived)

    class 8 Combined
    derive 8 MoreDerivedA
    derive 8 MoreDerivedB
    register 8

    return
.end

.function: main/1
    call (frame 0) typesystem_setup/0

    try
    catch "BaseA" handler
    enter throws_derived

    izero 0
    return
.end
