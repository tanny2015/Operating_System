# we need this(_POSIX_C_SOURCE=200809L ) only for the sleepy function 
CFLAGS = -Wall -g -std=c99 -D _POSIX_C_SOURCE=200809L -Werror

all: build

build:
	gcc $(CFLAGS) main.c  receiver.c sender.c SC.c KBInput.c list.o -lpthread -o s-talk
  
Fred: build
	./s-talk 6060 csil-cpu3 6001
	# ./s-talk 6060 localhost 6001

Barney: build
	./s-talk 6001 csil-cpu1 6060
	# ./s-talk 6001 localhost 6060

valgrind: build
	valgrind --leak-check=full --show-leak-kinds=all ./s-talk 6060 csil-cpu3 6001
	# valgrind --leak-check=full --show-leak-kinds=all ./s-talk 6060 localhost 6001
	# valgrind   ./s-talk 6060 csil-cpu3 6001
	# valgrind   ./s-talk 6060 localhost 6001

clean:
	rm -f s-talk