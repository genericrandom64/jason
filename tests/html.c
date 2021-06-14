#include "../cpu/cpu.h"
#include "../cpu/func_proto.h"
#include <assert.h>

extern function_pointer_array opcodes[];

int main() {
	printf("<table>");
	for (int row=0; row<16; row++) {
		printf("<tr>");			
		for(int column=0; column<16; column++) {
			int opcode = (row * 16) + column;
			if (opcodes[opcode] != NULL) {
				printf("<td style='color:yellow'><strong>%02x</strong></td>\n", opcode);
			} else {
				printf("<td style='color:red'>%02x</td>\n", opcode);
			}
		}
		printf("</tr>");			
	}
	printf("</table>");			
}
