target remote :1234
symbol-file kernel.bin
b _start
c
layout src
