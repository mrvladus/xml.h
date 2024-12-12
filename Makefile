build:
	gcc -o test test.c

run: build
	./test

clean:
	rm -rf test
