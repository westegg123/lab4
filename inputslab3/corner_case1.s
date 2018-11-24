.text
mov x1, 2
mov x2, 2
cmp x1, x2
beq foo
foo:
movz x3, 3
cbnz x2, bar
bar:
movz x4, 4
cbz x5, cow
cow:
movz x5, 5
hlt 0

