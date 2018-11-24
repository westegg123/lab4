.text
cmp X3, X4
beq bar
add X2, X0, 5
add X2, X0, 6
add X2, X0, 10

foo:
add X8, X9, 11
b bar

end:
HLT 0

bar:
add X10, X2, X8
cmp X1, X1
beq end
HLT 0

