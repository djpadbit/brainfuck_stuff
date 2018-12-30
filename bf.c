#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define BUFF_SIZE 30000
#define MAX_LOOP_REC 1024*1024

static unsigned char bf_buff[BUFF_SIZE];
static unsigned int bf_loop_stack[MAX_LOOP_REC];
static unsigned int *bf_loop_stack_ptr;
static unsigned char *bf_ptr;

int main(int argc, char const *argv[])
{
	if (argc < 2) {
		printf("Gib brainfuck like ./bf \"++++++++++[>+++++++>++++++++++>+++>+<<<<-]>++.>+.+++++++..+++.>++.<<+++++++++++++++.>.+++.------.--------.>+.>.\"\n");
		return 1;
	}

	bf_ptr = &bf_buff[0];
	bf_loop_stack_ptr = &bf_loop_stack[0];
	unsigned int len = strlen(argv[1]);
	printf("len : %i\n", len);
	for (unsigned int ptr=0;ptr<len;ptr++) {
		switch (argv[1][ptr]) {
			case '>':
				bf_ptr++;
				break;
			case '<':
				if (bf_ptr > bf_buff) bf_ptr--;
				break;
			case '+':
				(*bf_ptr)++;
				break;
			case '-':
				(*bf_ptr)--;
				break;
			case '.':
				putchar(*bf_ptr);
				break;
			case ',':
				(*bf_ptr) = getchar();
				break;
			case '[':
				if (!(*bf_ptr)) {
					int loopc = 0;
					for (ptr++;ptr<len;ptr++) {
						if (argv[1][ptr] == '[') loopc++;
						else if (argv[1][ptr] == ']') {
							if (!loopc) break;
							else loopc--;
						}
					}
				} else {
					(*(bf_loop_stack_ptr++)) = ptr;
				}
				break;
			case ']':
				if (*bf_ptr) ptr = *(bf_loop_stack_ptr-1);
				else bf_loop_stack_ptr--;
				break;
		}
	}
	return 0;
}
/*
Calculate Pi : >+++++++++++++++[<+>>>>>>>>++++++++++<<<<<<<-]>+++++[<+++++++++>-]+>>>>>>+[<<+++[>>[-<]<[>]<-]>>[>+>]<[<]>]>[[->>>>+<<<<]>>>+++>-]<[<<<<]<<<<<<<<+[->>>>>>>>>>>>[<+[->>>>+<<<<]>>>>>]<<<<[>>>>>[<<<<+>>>>-]<<<<<-[<<++++++++++>>-]>>>[<<[<+<<+>>>-]<[>+<-]<++<<+>>>>>>-]<<[-]<<-<[->>+<-[>>>]>[[<+>-]>+>>]<<<<<]>[-]>+<<<-[>>+<<-]<]<<<<+>>>>>>>>[-]>[<<<+>>>-]<<++++++++++<[->>+<-[>>>]>[[<+>-]>+>>]<<<<<]>[-]>+>[<<+<+>>>-]<<<<+<+>>[-[-[-[-[-[-[-[-[-<->[-<+<->>]]]]]]]]]]<[+++++[<<<++++++++<++++++++>>>>-]<<<<+<->>>>[>+<<<+++++++++<->>>-]<<<<<[>>+<<-]+<[->-<]>[>>.<<<<[+.[-]]>>-]>[>>.<<-]>[-]>[-]>>>[>>[<<<<<<<<+>>>>>>>>-]<<-]]>>[-]<<<[-]<<<<<<<<]++++++++++.

triforce ? : >++++[<++++++++>-]>++++++++[>++++<-]>>++>>>+>>>+<<<<<<<<<<[-[->+<]>[-<+>>>.<<]>>>[[->++++++++[>++++<-]>.<<[->+<]+>[->++++++++++<<+>]>.[-]>]]+<<<[-[->+<]+>[-<+>>>-[->+<]++>[-<->]<<<]<<<<]++++++++++.+++.[-]<]+++++
*/