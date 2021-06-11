#!/bin/sh
cc -c cpu.c -ggdb3; printf "$(grep TODO *.c *.h | wc -l) todos\n$(cat cpu.c | grep 'void op' | wc -l)/$(cat cpu.c|grep '#ifndef UOP'|wc -l) opcodes done of 256\n" 

# update the site
[ "$(whoami)" = "generic" ] && printf $(cat cpu.c | grep 'void op' | wc -l) > /tmp/pb && doas chown http:http /tmp/pb && doas -u http mv /tmp/pb /srv/http/computer/opcodes/ && cat cpu.c | grep -A17 function_pointer_array | grep -v func > /tmp/pba && doas chown http:http /tmp/pba && doas -u http mv /tmp/pba /srv/http/computer/opcodes/
