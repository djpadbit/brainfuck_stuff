#include <iostream>
#include <fstream>
#include <cstring>
#include <asmjit/asmjit.h>

#define BUFF_SIZE 30000
#define MAX_LOOP_REC 1024*1024
#define MAX_INSTS 1024*1024

#define DEFAULT_TO_HELLO
//#define ENABLE_ASMJIT_LOGGING

#ifdef DEFAULT_TO_HELLO
const char hello_world[] = "++++++++++[>+++++++>++++++++++>+++>+<<<<-]>++.>+.+++++++..+++.>++.<<+++++++++++++++.>.+++.------.--------.>+.>.";
#endif
const char bf_charset[] = "<>+-.,[]";

using namespace asmjit;

static unsigned char bf_buff[BUFF_SIZE];
static Label bf_loop_stack[MAX_LOOP_REC][2];
static unsigned int bf_loop_stack_ptr = 0;

typedef void (*func_type)(void);

class PrintErrorHandler : public asmjit::ErrorHandler {
public:
	// Return `true` to set last error to `err`, return `false` to do nothing.
	bool handleError(asmjit::Error err, const char* message, asmjit::CodeEmitter* origin) override {
		std::cerr << "ERROR: " << message << std::endl;
		return false;
	}
};

void clean_bf(char **fd, size_t *l) // Needs work, doesn't handle comments and stuff
{
	size_t ptr = 0;
	char *fdp = *fd;
	for (size_t i=0;i<(*l);i++) {
		if (std::strchr(bf_charset,fdp[i])) {
			fdp[ptr++] = fdp[i];
		}
	}
	(*fd) = (char*)std::realloc(fdp,ptr);
	(*l) = ptr;
}

int main(int argc, char const *argv[])
{
	char *file_data;
	size_t len = 0;
	if (argc < 2) {
#ifndef DEFAULT_TO_HELLO
		std::cerr << "Gib brainfuck like ./bfc helloworld.b" << std::endl;
		return 1;
#else
		std::cout << "Did not receive brainfuck, defaulting to the hello world one" << std::endl;
		file_data = (char*)&hello_world;
		len = std::strlen(file_data);
#endif
	} else {
		std::ifstream file(argv[1]);
		if (!file.is_open()) {
			std::cerr << "Coudln't open file" << std::endl;
			return 1;
		}
		file.seekg(0, std::ios::end);
		len = file.tellg();
		file.seekg(0, std::ios::beg);
		file_data = (char*)malloc(len);
		if (!file_data) {
			std::cerr << "Coudln't allocated memory" << std::endl;
			return 1;
		}
		file.read(file_data, len);
		file.close();
		clean_bf(&file_data,&len);
	}

	JitRuntime rt;
	CodeHolder code;
#ifdef ENABLE_ASMJIT_LOGGING
	FileLogger logger(stdout);
#endif
	PrintErrorHandler eh;

	code.init(rt.getCodeInfo());
#ifdef ENABLE_ASMJIT_LOGGING
	code.setLogger(&logger);
#endif
	code.setErrorHandler(&eh);

	X86Assembler a(&code);

	size_t ptr_tmp;
	Label tmpd,tmps;
	std::cout << "Compiling brainfuck" << std::endl;
	std::cout << "len : " << len << std::endl;
	a.mov(x86::rsi,(uint64_t)&bf_buff); // RSI is used as the pointer to the buffer

	for (size_t ptr=0;ptr<len;ptr++) {
		switch (file_data[ptr]) {
			case '>':
				for (ptr_tmp = ptr;ptr<len&&file_data[ptr]=='>';ptr++);
				ptr--;
				a.add(x86::rsi,(ptr+1)-ptr_tmp);
				break;
			case '<':
				for (ptr_tmp = ptr;ptr<len&&file_data[ptr]=='<';ptr++);
				ptr--;
				a.sub(x86::rsi,(ptr+1)-ptr_tmp);
				break;
			case '+':
				for (ptr_tmp = ptr;ptr<len&&file_data[ptr]=='+';ptr++);
				ptr--;
				a.add(x86::dword_ptr(x86::rsi),(ptr+1)-ptr_tmp);
				break;
			case '-':
				for (ptr_tmp = ptr;ptr<len&&file_data[ptr]=='-';ptr++);
				ptr--;
				a.sub(x86::dword_ptr(x86::rsi),(ptr+1)-ptr_tmp);
				break;
			case '.':
				a.mov(x86::rax,1);
				a.mov(x86::rdi,x86::rax);
				a.mov(x86::rdx,x86::rax);
				a.syscall();
				break;
			case ',':
				a.mov(x86::rax,0);
				a.mov(x86::rdi,x86::rax);
				a.mov(x86::rdx,1);
				a.syscall();
				break;
			case '[':
				tmpd = a.newLabel();
				tmps = a.newLabel();
				a.jmp(tmpd);
				a.bind(tmps);
				bf_loop_stack[bf_loop_stack_ptr][0] = tmpd;
				bf_loop_stack[bf_loop_stack_ptr][1] = tmps;
				bf_loop_stack_ptr++;
				break;
			case ']':
				bf_loop_stack_ptr--;
				tmpd = bf_loop_stack[bf_loop_stack_ptr][0];
				tmps = bf_loop_stack[bf_loop_stack_ptr][1];
				a.bind(tmpd);
				a.mov(x86::eax,x86::dword_ptr(x86::rsi));
				a.test(x86::al,x86::al);
				a.jne(tmps);
				break;
		}
	}
	a.ret();
	func_type fn;
	Error err = rt.add(&fn, &code);
	if (err) {
		std::cerr << "ASMJIT Error" << std::endl;
		std::cerr << DebugUtils::errorAsString(err) << std::endl;
		return 1;
	}
	std::cout << "Running brainfuck" << std::endl;
	fn();
	rt.release(fn);
	return 0;
}