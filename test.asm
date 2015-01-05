istore 1 16
istore 2 1
ilt 2 1 3
branch 3 5 4
print 3
jump foo
.mark: foo
istore 4 @1
istore 5 4
iinc @5
iinc 4
print @5
halt
