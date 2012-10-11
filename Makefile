all:
	gcc -g -W -Wall zp.c dictionary.c fortune-set.c -o zp -lm
