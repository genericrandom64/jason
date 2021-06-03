#!/bin/sh
cc -c cpu.c -o /dev/null -w; printf "$(grep TODO *.c *.h | wc -l) todos\n$(cat cpu.c | grep 'void op' | wc -l) opcodes done of 152 official/255 total\n" 
