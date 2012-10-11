all:
	gcc -g -W -Wall parrot.c index.c fortune-set.c tokenizer.c -o parrot -lm

clean:
	rm parrot -f
