.def: foo 0
    istore 1 16
    print 1
    end
.end

istore 1 0
iinc 1

ref 1 2

jump :foo
istore 3 41
iadd 2 3

.mark: foo
print 2

call foo

halt
