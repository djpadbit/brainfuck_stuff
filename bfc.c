#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <signal.h>

#define BUFF_SIZE 30000
#define MAX_LOOP_REC 1024*1024
#define MAX_INSTS 1024*1024

#define DEFAULT_TO_HELLO

#ifdef DEFAULT_TO_HELLO
const char hello_world[] = "++++++++++[>+++++++>++++++++++>+++>+<<<<-]>++.>+.+++++++..+++.>++.<<+++++++++++++++.>.+++.------.--------.>+.>.";
#endif

static unsigned char bf_buff[BUFF_SIZE];
static unsigned int bf_loop_stack[MAX_LOOP_REC];
static unsigned int *bf_loop_stack_ptr;
static unsigned char *bf_ptr;

int change_page_permissions_of_address(void *addr,int len,int flags) {
	// Move the pointer to the page boundary
	int page_size = getpagesize();
	void* addra = addr-((unsigned long)addr % page_size);

	for (;addra < addr+len; addra+=page_size) {
		if(mprotect(addra, page_size, flags) == -1) {
			return 1;
		}
	}

	return 0;
}

static unsigned char instbuffer[MAX_INSTS+4+3];
static void (*instbuffer_func)() = (void (*)())instbuffer;
static unsigned int instptr = 0;

void change_addr(void *src, void *dst)
{
	int call_off = dst-(src+4);
	unsigned char *jmp = (unsigned char*)src;
	//memset(src,'A',4);
	jmp[3] = (call_off>>24)&0xFF;
	jmp[2] = (call_off>>16)&0xFF;
	jmp[1] = (call_off>>8)&0xFF;
	jmp[0] = call_off&0xFF;
}
void change_addr_extra(void *src, void *dst,int extra)
{
	int call_off = dst-(src+4+extra);
	unsigned char *jmp = (unsigned char*)src;
	//memset(src,'A',4);
	jmp[3] = (call_off>>24)&0xFF;
	jmp[2] = (call_off>>16)&0xFF;
	jmp[1] = (call_off>>8)&0xFF;
	jmp[0] = call_off&0xFF;
}
/*
	11c0:	48 83 05 98 2e 00 00 	addq   $0x1,0x2e98(%rip)        # 4060 <buff_ptr>
	11c7:	01 
*/
int addptrfor()
{
	if (instptr+8 > MAX_INSTS) {
		printf("Not enough space left in instruction buffer\n");
		return 0;
	}
	unsigned char insts[] = 
		"\x48\x83\x05\x00\x00\x00\x00"		// addq   $0x1,(to be filled)(%rip)
		"\x01";								// ?
	memcpy(instbuffer+4+instptr,insts,8);
	change_addr_extra(instbuffer+4+instptr+3,&bf_ptr,1);
	instptr+=8;
	return 1;
}
/*
	11d0:	48 83 2d 88 2e 00 00 	subq   $0x1,0x2e88(%rip)        # 4060 <buff_ptr>
	11d7:	01 
*/
int addptrbac()
{
	if (instptr+8 > MAX_INSTS) {
		printf("Not enough space left in instruction buffer\n");
		return 0;
	}
	unsigned char insts[] = 
		"\x48\x83\x2d\x00\x00\x00\x00"		// subq   $0x1,(to be filled)(%rip)
		"\x01";								// ?
	memcpy(instbuffer+4+instptr,insts,8);
	change_addr_extra(instbuffer+4+instptr+3,&bf_ptr,1);
	instptr+=8;
	return 1;
}
/*
	11e0:	48 8b 05 79 2e 00 00 	mov    0x2e79(%rip),%rax        # 4060 <buff_ptr>
	11e7:	80 00 01             	addb   $0x1,(%rax)
*/
int addcelladd()
{
	if (instptr+10 > MAX_INSTS) {
		printf("Not enough space left in instruction buffer\n");
		return 0;
	}
	unsigned char insts[] = 
		"\x48\x8b\x05\x00\x00\x00\x00"		// mov    (to be filled)(%rip),%rax
		"\x80\x00\x01";						// addb   $0x1,(%rax)
	memcpy(instbuffer+4+instptr,insts,10);
	change_addr(instbuffer+4+instptr+3,&bf_ptr);
	instptr+=10;
	return 1;
}
/*
	11f0:	48 8b 05 69 2e 00 00 	mov    0x2e69(%rip),%rax        # 4060 <buff_ptr>
	11f7:	80 28 01             	subb   $0x1,(%rax)
*/
int addcellsub()
{
	if (instptr+10 > MAX_INSTS) {
		printf("Not enough space left in instruction buffer\n");
		return 0;
	}
	unsigned char insts[] = 
		"\x48\x8b\x05\x00\x00\x00\x00"		// mov    (to be filled)(%rip),%rax
		"\x80\x28\x01";						// subb   $0x1,(%rax)
	memcpy(instbuffer+4+instptr,insts,10);
	change_addr(instbuffer+4+instptr+3,&bf_ptr);
	instptr+=10;
	return 1;
}
/*
	1190:	48 8b 05 c9 2e 00 00 	mov    0x2ec9(%rip),%rax        # 4060 <buff_ptr>
	
	1130:	48 8b 35 c9 2e 00 00 	mov    0x2ec9(%rip),%rsi        # 4060 <buff_ptr>
	1137:	48 c7 c0 01 00 00 00 	mov    $0x1,%rax
	113e:	48 89 c7             	mov    %rax,%rdi
	1141:	48 89 c2             	mov    %rax,%rdx
	1148:	0f 05                	syscall 
*/
int addputchar()
{
	if (instptr+23 > MAX_INSTS) {
		printf("Not enough space left in instruction buffer\n");
		return 0;
	}
	unsigned char insts[] = 
		"\x48\x8b\x35\x00\x00\x00\x00"		// mov    (to be filled)(%rip),%rsi
		"\x48\xc7\xc0\x01\x00\x00\x00"		// mov    $0x1,%rax
		"\x48\x89\xc7"						// mov    %rax,%rdi
		"\x48\x89\xc2"						// mov    %rax,%rdx
		"\x0f\x05"							// syscall
		"\x90";								// nop

	memcpy(instbuffer+4+instptr,insts,23);
	change_addr(instbuffer+4+instptr+3,&bf_ptr);
	instptr+=23;
	return 1;
}
/*
	11a0:	48 83 ec 08          	sub    $0x8,%rsp
	11a4:	31 c0                	xor    %eax,%eax
	11a6:	e8 95 fe ff ff       	callq  1040 <getchar@plt>
	11ab:	48 8b 15 ae 2e 00 00 	mov    0x2eae(%rip),%rdx        # 4060 <buff_ptr>
	11b2:	88 02                	mov    %al,(%rdx)
	11b4:	48 83 c4 08          	add    $0x8,%rsp
*/
int addgetchar()
{
	if (instptr+24 > MAX_INSTS) {
		printf("Not enough space left in instruction buffer\n");
		return 0;
	}
	unsigned char insts[] = 
		"\x48\x83\xec\x08"					// sub    $0x8,%rsp
		"\x31\xc0"							// xor    %eax,%eax
		"\xe8\x00\x00\x00\x00"				// callq  (to be filled)<getchar@plt>
		"\x48\x8b\x15\x00\x00\x00\x00"		// mov    (to be filled)(%rip),%rdx
		"\x88\x02"							// mov    %al,(%rdx)
		"\x48\x83\xc4\x08";					// add    $0x8,%rsp

	memcpy(instbuffer+4+instptr,insts,24);
	change_addr(instbuffer+4+instptr+7,getchar);
	change_addr(instbuffer+4+instptr+14,&bf_ptr);
	instptr+=24;
	return 1;
}

int addloopstart()
{
	if (instptr+5 > MAX_INSTS) {
		printf("Not enough space left in instruction buffer\n");
		return 0;
	}
	unsigned char jmp[5] = "\xe9\x00\x00\x00\x00";
	memcpy(instbuffer+4+instptr,jmp,5);
	instptr+=5;
	return 1;
}
/*
	124b:	48 8b 05 0e 2f 00 00 	mov    0x2f0e(%rip),%rax        # 4160 <buff_ptr>
	1252:	0f b6 00             	movzbl (%rax),%eax
	1255:	84 c0                	test   %al,%al
	1257:	75 ac                	jne    1205 <kek+0x6>
*/
int addloopend(void *src)
{
	if (instptr+18 > MAX_INSTS) {
		printf("Not enough space left in instruction buffer\n");
		return 0;
	}
	unsigned char insts[] = 
		"\x48\x8b\x05\x00\x00\x00\x00"		// mov    (to be filled)(%rip),%rax
		"\x0f\xb6\x00"						// movzbl (%rax),%eax
		"\x84\xc0"							// test   %al,%al
		"\x0f\x85\x00\x00\x00\x00";			// jne    (to be filled)

	memcpy(instbuffer+4+instptr,insts,18);
	change_addr(instbuffer+4+instptr+3,&bf_ptr);
	change_addr(instbuffer+4+instptr+14,src+5);
	change_addr(src+1,instbuffer+4+instptr);
	instptr+=18;
	return 1;
}

static inline void instbuffclean()
{
	memset(instbuffer+4,'\x90',MAX_INSTS);
	instptr = 0;
}

static inline void instbuffsetup()
{
	unsigned char strt[4] = "\x55\x48\x89\xe5";
	unsigned char end[3] = "\x90\x5d\xc3";
	memcpy(instbuffer,&strt,4);
	memcpy(instbuffer+4+MAX_INSTS,&end,3);
	change_page_permissions_of_address(&instbuffer,MAX_INSTS,PROT_READ | PROT_WRITE | PROT_EXEC);
	instbuffclean();
}

int main(int argc, char const *argv[])
{
	if (argc < 2) {
#ifndef DEFAULT_TO_HELLO
		printf("Gib brainfuck like ./bfc \"++++++++++[>+++++++>++++++++++>+++>+<<<<-]>++.>+.+++++++..+++.>++.<<+++++++++++++++.>.+++.------.--------.>+.>.\"\n");
		return 1;
#else
		printf("Did not receive brainfuck, defaulting to the hello world one\n");
		argv[1] = (const char*)&hello_world;
#endif
	}
	instbuffsetup();
	bf_ptr = &bf_buff[0];
	bf_loop_stack_ptr = &bf_loop_stack[0];
	unsigned int len = strlen(argv[1]);
	unsigned int tmp = 0;
	printf("Compiling brainfuck\n");
	printf("len : %i\n", len);
	for (unsigned int ptr=0;ptr<len;ptr++) {
		switch (argv[1][ptr]) {
			case '>':
				addptrfor();
				break;
			case '<':
				addptrbac();
				break;
			case '+':
				addcelladd();
				break;
			case '-':
				addcellsub();
				break;
			case '.':
				addputchar();
				break;
			case ',':
				printf("GETCHAR IS BROKEN\n");
				addgetchar();
				break;
			case '[':
				(*(bf_loop_stack_ptr++)) = instptr;
				addloopstart();
				break;
			case ']':
				tmp = *(bf_loop_stack_ptr-1);
				bf_loop_stack_ptr--;
				addloopend(&(instbuffer[tmp+4]));
				break;
		}
	}
	printf("Nb instructions : %i\n", instptr);
	printf("Running brainfuck\n");
	instbuffer_func();
	//fflush(stdout);
	return 0;
}
/*
Calculate Pi : >+++++++++++++++[<+>>>>>>>>++++++++++<<<<<<<-]>+++++[<+++++++++>-]+>>>>>>+[<<+++[>>[-<]<[>]<-]>>[>+>]<[<]>]>[[->>>>+<<<<]>>>+++>-]<[<<<<]<<<<<<<<+[->>>>>>>>>>>>[<+[->>>>+<<<<]>>>>>]<<<<[>>>>>[<<<<+>>>>-]<<<<<-[<<++++++++++>>-]>>>[<<[<+<<+>>>-]<[>+<-]<++<<+>>>>>>-]<<[-]<<-<[->>+<-[>>>]>[[<+>-]>+>>]<<<<<]>[-]>+<<<-[>>+<<-]<]<<<<+>>>>>>>>[-]>[<<<+>>>-]<<++++++++++<[->>+<-[>>>]>[[<+>-]>+>>]<<<<<]>[-]>+>[<<+<+>>>-]<<<<+<+>>[-[-[-[-[-[-[-[-[-<->[-<+<->>]]]]]]]]]]<[+++++[<<<++++++++<++++++++>>>>-]<<<<+<->>>>[>+<<<+++++++++<->>>-]<<<<<[>>+<<-]+<[->-<]>[>>.<<<<[+.[-]]>>-]>[>>.<<-]>[-]>[-]>>>[>>[<<<<<<<<+>>>>>>>>-]<<-]]>>[-]<<<[-]<<<<<<<<]++++++++++.

triforce ? : >++++[<++++++++>-]>++++++++[>++++<-]>>++>>>+>>>+<<<<<<<<<<[-[->+<]>[-<+>>>.<<]>>>[[->++++++++[>++++<-]>.<<[->+<]+>[->++++++++++<<+>]>.[-]>]]+<<<[-[->+<]+>[-<+>>>-[->+<]++>[-<->]<<<]<<<<]++++++++++.+++.[-]<]+++++
*/