all:
	gcc -g -W -Wall parrot.c index.c fortune-set.c build-index.c hash.c tokenizer.c lcs.c -o parrot -lm

clean:
	rm parrot -f
