all: 
	gcc -c -g threadPool.c
	gcc -c -g osqueue.c
	ar rcs libthreadPool.a threadPool.o osqueue.o
	gcc -g -L. test.c -lthreadPool -lpthread  -o a.out
