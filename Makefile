all:
	gcc bf.c -O3 -o bf
	gcc bfc.c -O3 -o bfc
	g++ bfaj.cpp -O3 -lasmjit -o bfaj