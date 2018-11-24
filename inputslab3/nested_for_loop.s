.text
mov X1, 5
mov X2, 0

foo:
mov X3, 1
mov X3, 1
mov X3, 1

mov X5, 5
mov X6, 0
bar:
mov X3, 1
mov X3, 1
add X6, X6, 1
cmp X5, X6
bgt bar

add X2, X2, 1
cmp X1, X2
bgt foo


mov X1, 4
mov X2, 0


HLT 0

