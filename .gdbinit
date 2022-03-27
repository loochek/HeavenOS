target remote :1234
file kernel.bin
symbol-file kernel.sym
b _early_entry_point
c
layout src
