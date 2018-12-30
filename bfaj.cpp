#include <iostream>
#include <cstring>
#include <asmjit/asmjit.h>

#define BUFF_SIZE 30000
#define MAX_LOOP_REC 1024*1024
#define MAX_INSTS 1024*1024

#define DEFAULT_TO_HELLO

#ifdef DEFAULT_TO_HELLO
const char hello_world[] = "++++++++++[>+++++++>++++++++++>+++>+<<<<-]>++.>+.+++++++..+++.>++.<<+++++++++++++++.>.+++.------.--------.>+.>.";
#endif

using namespace asmjit;

static unsigned char bf_buff[BUFF_SIZE];
static Label bf_loop_stack[MAX_LOOP_REC][2];
static unsigned int bf_loop_stack_ptr = 0;
static unsigned char *bf_ptr;

typedef void (*func_type)(void);

class PrintErrorHandler : public asmjit::ErrorHandler {
public:
	// Return `true` to set last error to `err`, return `false` to do nothing.
	bool handleError(asmjit::Error err, const char* message, asmjit::CodeEmitter* origin) override {
		std::cerr << "ERROR: " << message << std::endl;
		return false;
	}
};

int main(int argc, char const *argv[])
{
	if (argc < 2) {
#ifndef DEFAULT_TO_HELLO
		std::cerr << "Gib brainfuck like ./bfc \"++++++++++[>+++++++>++++++++++>+++>+<<<<-]>++.>+.+++++++..+++.>++.<<+++++++++++++++.>.+++.------.--------.>+.>.\"" << std::endl;
		return 1;
#else
		std::cout << "Did not receive brainfuck, defaulting to the hello world one" << std::endl;
		argv[1] = (const char*)&hello_world;
#endif
	}
	JitRuntime rt;
	CodeHolder code;
	FileLogger logger(stdout);
	PrintErrorHandler eh;

	code.init(rt.getCodeInfo());
	code.setLogger(&logger);
	code.setErrorHandler(&eh);

	X86Assembler a(&code);

	bf_ptr = &bf_buff[0];
	unsigned int len = std::strlen(argv[1]);
	unsigned int ptr_tmp;
	Label tmpd,tmps;
	std::cout << "Compiling brainfuck" << std::endl;
	std::cout << "len : " << len << std::endl;
	a.mov(x86::rsi,(uint64_t)bf_ptr);
	for (unsigned int ptr=0;ptr<len;ptr++) {
		switch (argv[1][ptr]) {
			case '>':
				ptr_tmp = ptr;
				for (;ptr<len&&argv[1][ptr]=='>';ptr++);
				ptr--;
				a.add(x86::rsi,(ptr+1)-ptr_tmp);
				break;
			case '<':
				ptr_tmp = ptr;
				for (;ptr<len&&argv[1][ptr]=='<';ptr++);
				ptr--;
				a.sub(x86::rsi,(ptr+1)-ptr_tmp);
				break;
			case '+':
				ptr_tmp = ptr;
				for (;ptr<len&&argv[1][ptr]=='+';ptr++);
				ptr--;
				a.add(x86::dword_ptr(x86::rsi),(ptr+1)-ptr_tmp);
				break;
			case '-':
				ptr_tmp = ptr;
				for (;ptr<len&&argv[1][ptr]=='-';ptr++);
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