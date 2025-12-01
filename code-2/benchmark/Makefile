
all : test
test: ../my_vm.h
	gcc -g test.c -L../ -lmy_vm -o test
	gcc -g multi_test.c -L../ -lmy_vm -lpthread -o mtest

clean:
	rm -rf test mtest
