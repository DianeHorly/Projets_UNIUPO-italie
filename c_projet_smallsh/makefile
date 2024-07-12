smallsh: smallsh.o input.o
	gcc -Wall -o smallsh input.o smallsh.o 
smallsh.o: smallsh.c smallsh.h
	gcc -Wall -c smallsh.c
input.o: input.c smallsh.h
	gcc -Wall -c input.c
clean:
	rm smallsh input.o smallsh.o
