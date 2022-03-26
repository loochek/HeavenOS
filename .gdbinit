target remote :1234
file kernel.bin
symbol-file kernel.sym
b kmain
c
layout src
