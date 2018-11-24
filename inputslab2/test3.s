.text
mov X1, 0x1000

mov X10, 10

stur X10, [X1, 0x0]

mov X12, 2

mov X2, 0x1000

lsl x2, x2, 28

stur W12, [X1, 0x10]

stur X12, [X1, 0x20]

ldur X13, [X1, 0x0]

ldur X14, [X1, 0x10]

stur x2, [x1, 0x30]

stur W2, [x1, 0x40]

mov x3, 0xabcd

lsl x3, x3, 46

stur x3, [x1, 0x50]

stur w3, [x1, 0x60]

mov x6, 0xabcd

sturb w6, [x1,#0x4C]

 

ldur x4, [x1, 0x50]

ldur w5, [x1, 0x50]

ldur w20, [x1, 0x54]

 

HLT 0
