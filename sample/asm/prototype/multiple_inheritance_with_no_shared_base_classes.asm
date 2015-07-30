.block: handler
    print (pull 2)
    leave
.end

.block: throws_derived
    throw (new 1 Combined)
    leave
.end

.function: typesystem_setup
    register (class 1 BaseA)
    register (derive (class 1 DerivedA) BaseA)
    register (derive (class 1 MoreDerivedA) DerivedA)

    register (class 1 BaseB)
    register (derive (class 1 DerivedB) BaseB)
    register (derive (class 1 MoreDerivedB) DerivedB)

    class 1 Combined
    derive 1 MoreDerivedA
    derive 1 MoreDerivedB
    register 1

    end
.end

.function: main
    call (frame 0) typesystem_setup

    tryframe
    catch "BaseA" handler
    try throws_derived

    izero 0
    end
.end
