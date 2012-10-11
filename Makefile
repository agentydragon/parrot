all:
	gcc -g -W -Wall parrot.c index.c fortune-set.c tokenizer.c -o parrot -lm
