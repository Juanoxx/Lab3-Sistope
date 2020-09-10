# pragma once

/*Estructura que guarda cada uno de los resultados de las funciones de pooling, clasificacion, etc.*/
struct funciones{
	struct listmf *buffer;
	struct listmf *photothread;
	struct matrixf *filter;
	int *datos;
	char *imagenSalida;
	int mostrar;
};
typedef struct funciones funciones;