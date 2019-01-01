CFLAGS = -O3

all:
	gcc bf.c $(CFLAGS) -o bf
	gcc bfc.c $(CFLAGS) -o bfc
	g++ bfaj.cpp $(CFLAGS) -lasmjit -o bfaj

debug: CFLAGS += -g
debug: all

clean:
	rm bf bfc bfaj