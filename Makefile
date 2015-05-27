all: 
	gcc -c threadPool.c
	gcc -c osqueue.c
	ar rcs libthreadPool.a threadPool.o osqueue.o
	gcc -L. test.c -lthreadPool -lpthread  -o a.out

