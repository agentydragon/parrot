all:
	gcc -g -W -Wall parrot.c dictionary.c fortune-set.c tokenizer.c -o parrot -lm
