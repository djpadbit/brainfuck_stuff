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
		printf("Gib brainfuck like ./bf helloworld.b\n");
		return 1;
	}
	FILE *file;
	if (!(file=fopen(argv[1], "r"))) {
		printf("Coudln't open '%s'\n", argv[1]);
		return 1;
	}
	fseek (file, 0, SEEK_END);
	size_t len = ftell(file);
	rewind(file);
	char *file_data = (char*)calloc(1,len);
	if (!file_data) {
		printf("Coudln't allocated memory to read file\n");
		return 1;
	}
	fread(file_data,1,len,file);
	fclose(file);

	bf_ptr = &bf_buff[0];
	bf_loop_stack_ptr = &bf_loop_stack[0];
	printf("len : %i\n", len);
	for (unsigned int ptr=0;ptr<len;ptr++) {
		switch (file_data[ptr]) {
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
						if (file_data[ptr] == '[') loopc++;
						else if (file_data[ptr] == ']') {
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