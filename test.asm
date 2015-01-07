.name: 1 sixteen
.name: 2 one
.name: 5 fifth_reg
istore sixteen 16
istore one 1
ilt 2 1 3
branch 3 5 4
print 3
jump :foo
.mark: foo
istore 5 4
istore @fifth_reg @1
iinc @5
iinc 4
print @5
halt
