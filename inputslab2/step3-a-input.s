.text
mov X3, 0x1
mov X4, 0x2
b foo
add X13, X0, 10
add X14, X9, 11
add X15, X9, 11
add X16, X9, 11

foo:
add X14, X9, 11

b bar
add X16, X2, X8
add X17, X2, X8

bar:
add X15, X2, X8
HLT 0
