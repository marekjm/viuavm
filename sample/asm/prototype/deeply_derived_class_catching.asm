.block: handler
    print (pull 2)
    leave
.end

.block: throws_derived
    throw (new 1 DeeplyDerived)
    leave
.end

.function: typesystem_setup
    register (class 1 Base)
    register (derive (class 1 Derived) Base)
    register (derive (class 1 MoreDerived) Derived)
    register (derive (class 1 EvenMoreDerived) MoreDerived)
    register (derive (class 1 DeeplyDerived) EvenMoreDerived)
    end
.end

.function: main
    call (frame 0) typesystem_setup

    tryframe
    catch "Base" handler
    enter throws_derived

    izero 0
    end
.end
