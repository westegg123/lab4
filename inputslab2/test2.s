.text
movz X9, 10
movz X10, 10
add X0, X10, 4
cbnz X10, cow 

foo:
HLT 0

cow:
cbnz X22, 8 
add X21, X1, X2
cbz X23, 8
add X21, X1, X2
add X30, X1, 0
b foo
HLT 0
