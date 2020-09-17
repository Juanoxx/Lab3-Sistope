padre: main.c -ljpeg -lm -lpthread
	gcc -o pipeline main.c -l pthread -ljpeg -lm -I.

