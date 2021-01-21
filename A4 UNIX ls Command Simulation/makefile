all: build

build:
	gcc -o UnixLs  UnixLs.c  list.o list.h 


valgrind: build
	valgrind --leak-check=full --show-leak-kinds=all ./UnixLs
 
clean:
	rm -f UnixLs