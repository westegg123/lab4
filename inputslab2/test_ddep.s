.text
movz x1, 1
movz x3, 2
add x2, x1, 1
add x2, x2, 1
add x2, x2, 1
movz x4, 10
add x2, x2, x4

cmp x2, x3
bne foo
hlt 0

foo:
movz x30, 1
sub x3, x2, x1
orr x6, x2, x1
sub x3, x30, x2
add x7, x3, x6
hlt 0

