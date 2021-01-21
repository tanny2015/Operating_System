all: build

build:
	gcc -o main cpusch.c  list.o

valgrind: build
	valgrind --leak-check=full --show-leak-kinds=all ./main
 
clean:
	rm -f main