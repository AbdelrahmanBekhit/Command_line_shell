.PHONY: all
all: shell.out test.out

shell.out: shell.c parser.c pipe_handler.c
	gcc -o $@ $^

test.out: test.c parser.c pipe_handler.c
	gcc -o $@ $^

.PHONY: clean
clean: 
	rm -f *.out

.PHONY: test
test: test.out
	./test.out