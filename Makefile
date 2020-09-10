pipeline: main.c -lpng -lm -lpthread
	gcc -o pipeline main.c -l pthread -lpng -lm -I.

