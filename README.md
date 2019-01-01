# Brainfuck Stuff
I made a simple interpreter, a test with some nasty x86 asm botching and a JIT brainfuck compiler with simple optimizations using asmjit

### Files :
```
bf.c - Simple interpreter, it seems to work on everything i tested yet
bfc.c - Nasty JIT botched together, ehhhh, you shouldn't use that
bfaj.cpp - Prettier JIT using asmjit, It seems to work on most things, but on some it doesn't work idk why
```

### Compiling
bfaj.cpp requires [asmjit](https://github.com/asmjit/asmjit)
```
make
```