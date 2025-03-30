test:
	@cp xml.h xml.c
	gcc -DXML_H_IMPLEMENTATION -DXML_H_TEST -DXML_H_DEBUG -o test xml.c
	@rm xml.c
	./test
	@rm test
