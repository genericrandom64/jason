#!/bin/sh
cc -c cpu.c -ggdb3; printf "$(grep TODO *.c *.h | wc -l) todos\n$(cat cpu.c | grep 'void op' | wc -l)/$(cat cpu.c|grep '#ifndef UOP'|wc -l) opcodes done of 256\n" 
