.text
adds X8, X1, 0x1
adds X19, X2, 0x2
adds X11, X8, 0xff
adds X8, X1, 0x1
adds X19, X8, 0x2
mov X1, 0x1000
lsl X1, X1, 16
ldur X6, [X1, 0x0]
stur X6, [X1, 0x8]
cmp x8, x19
b foo
mov X1, 0x1000
lsl X1, X1, 16
ldur X6, [X1, 0x0]
ldur X7, [x6, 0x0]
adds X8, X6, 0x1
hlt 0 

foo:
movz x23, 1
hlt 0
