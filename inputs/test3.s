.text

mov x1, 0x1000
mov x4, 0xFFFF
lsl x5, x4, 16
add x5, x4, x5

mov x6, 0x8888
mov x7, 0x7777
lsl x1, x1, 16
mov x8, 0x3333
mov x9, 0x4444

stur x5, [x1, 0x1]
# stur x5, [x1, 0x2]
# stur x6, [x1, 0x]
# stur x7, [x1, 0x11]

ldur x10, [x1, 0x1]
# ldur x11, [x1, 0x1]
# ldur x12, [x1, 0x1]
# ldur x13, [x1, 0x1]

# stur x30, [x1, 0x29]
# stur x9, [x1, 0x29]
# ldur x15, [x1, 0x29]

# stur x30, [x1, 0x30]
# stur x9, [x1, 0x30]
# ldur x16, [x1, 0x30]

# stur x30, [x1, 0x31]
# stur x9, [x1, 0x31]
# ldur x17, [x1, 0x31]


hlt 0

