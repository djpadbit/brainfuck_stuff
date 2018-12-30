# Brainfuck Stuff
I made a simple interpreter, a test with some nasty x86 asm botching and a JIT brainfuck compiler with simple optimizations using asmjit

### Files :
```
bf.c - Simple interpreter
bfc.c - Nasty JIT botched together
bfaj.cpp - Prettier JIT using asmjit
```

### Compiling
bfaj.cpp requires [asmjit](https://github.com/asmjit/asmjit)
```
make
```