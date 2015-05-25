all: 
	gcc -c -g threadPool.c
	gcc -c -g osqueue.c
	ar rcs libthreadPool.a threadPool.o osqueue.o
	gcc -g -L. test.c -lthreadPool -lpthread  -o a.out

sadaka:
	gcc -c -g threadPool.c
	gcc -c -g osqueue.c
	ar rcs libthreadPool.a threadPool.o osqueue.o
	gcc -g -L. testSadaka.c -lthreadPool -lpthread  -o a.out

dori:
	gcc -c -g threadPool.c
	gcc -c -g osqueue.c
	ar rcs libthreadPool.a threadPool.o osqueue.o
	gcc -g -L. testDori.c -lthreadPool -lpthread  -o a.out

valgrind:
	gcc -c -g threadPool.c
	gcc -c -g osqueue.c
	ar rcs libthreadPool.a threadPool.o osqueue.o
	gcc -g -L. testValgrind.c -lthreadPool -lpthread  -o a.out

