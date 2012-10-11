all:
	gcc -g -W -Wall parrot.c dictionary.c fortune-set.c -o parrot -lm
