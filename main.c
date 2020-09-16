/*Laboratorio número tres de Sistemas operativos - 1 - 2020*/
/*Integrantes: Hugo Arenas - Juan Arredondo*/
/*Profesor: Fernando Rannou*/


/*Se importan las librerías*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <getopt.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <pthread.h>
#include <png.h>
#include "matrixf.h"
#include "listmf.h"
#include "funciones.h"
#include <stdint.h>
#include <math.h>
#include "jpeglib.h"
#include <setjmp.h>

//Primer mutex para la lectura y escritura del buffer.
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
//Segundo mutex para la lectura y escritura del contador para detectar nearly black.
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;
//Primer barrier para que todas las hebras hagan los procesos de las imagenes en paralelo.
pthread_barrier_t barrier;
//Segundo barrier para esperar a todas las hebras que calculan nearly black.
pthread_barrier_t barrier2;
//Dato global que contiene toda la informacion necesaria para trabajar una imagen y
//almacena el buffer.
funciones args;


//Entradas: Puntero de tipo png_bytep, el largo y ancho de tipo int de la matriz entrante.
//Funcionamiento: Realiza la conversion de una matriz a escala de grises.
//Salidas: matrixf, el cual sirve como entrada para las demas funciones.

matrixf *grayScale(png_bytep *row_pointers, int height, int width) {
  matrixf *mf = createMF(height, width);
  for(int y = 0; y < height; y++) {
    png_bytep row = row_pointers[y];
    for(int x = 0; x < width; x++) {
      png_bytep px = &(row[x * 4]);
	  float prom = px[0]*0.299+px[1]*0.587+px[2]*0.114;
	  mf = setDateMF(mf, y, x, prom);
    }
  }
  return mf;
}



//Entradas: Nombre de la imagen entrante de tipo char, matriz vacia para guarda la imagen, 
//          parametros importantes del formato png.
//Funcionamiento: realiza la lectura de la imagen entrante.
//Salidas: matrixf, correspondiente a los pixeles de la imagen y el cual sirve como entrada 
//         para las demas funciones.

matrixf* readPNG(char *nombre, matrixf *mf, int width, int height, png_byte color_type,
  png_byte bit_depth, png_bytep *row_pointers) {
  FILE *archivo = fopen(nombre, "rb");
  png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  png_infop info = png_create_info_struct(png);
  png_init_io(png, archivo);
  png_read_info(png, info);
  width      = png_get_image_width(png, info);
  height     = png_get_image_height(png, info);
  color_type = png_get_color_type(png, info);
  bit_depth  = png_get_bit_depth(png, info);
  png_read_update_info(png, info);
  row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * height);
  for(int y = 0; y < height; y++) {
    row_pointers[y] = (png_byte*)malloc(png_get_rowbytes(png,info));
  }
  png_read_image(png, row_pointers);
  fclose(archivo);
  png_destroy_read_struct(&png, &info, NULL);
  mf = grayScale(row_pointers, height, width);
  return mf;
}

//Entradas: La matriz resultante de etapas anteriores, y el filtro necesario para hacer la convolution.
//Funcionamiento: Aplica la convolution, el filtro a la matriz entrante.
//Salidas: matrixf,correspondiente a la matriz filtrada y el cual sirve como entrada para las demas funciones.

matrixf *bidirectionalConvolution(matrixf *mf, matrixf *filter){
	if ((countFil(filter) == countColumn(filter))&&(countFil(filter)%2 == 1)){
		int increase = 0, initial = countFil(filter);
		while (initial != 1){
			initial = initial - 2;
			increase = increase + 1;
		}
		for (int cont = 0; cont < increase; cont++){
			mf = amplifyMF(mf);
		}
		for (int fil = 0; fil < countFil(mf) - countFil(filter); fil++){
			for (int col = 0; col < countColumn(mf) - countColumn(filter); col++){
				float sum = 0.0000;
				for (int y = 0; y < countFil(filter); y++){
					for (int x = 0; x < countColumn(filter); x++){
						float result = getDateMF(filter, y, x)*getDateMF(mf, y + fil, x + col);
						sum = sum + result;
					}
				}
				mf = setDateMF(mf, fil + (countFil(filter)/2), col + (countColumn(filter)/2), sum);
			}
		}
		for (int cont2 = 0; cont2 < increase; cont2++){
			mf = decreaseMF(mf);
		}
		return mf;
	}
	else{
		return mf;
	}
}

//Entradas: La matriz resultante de etapas anteriores.
//Funcionamiento: Aplica la rectificacion, recorre cada pixel de la matriz, verificando si es positivo 
//                (se mantiene pixel 0 es 255 si es mayor que este) o negativo (se cambia por 0).
//Salidas: matrixf, el cual sirve como entrada para las demas funciones.

matrixf *rectification(matrixf *mf){
	for (int y = 0; y < countFil(mf); y++){
		for (int x = 0; x < countColumn(mf); x++){
			if (getDateMF(mf,y,x) < 0.0000){
				mf = setDateMF(mf, y, x, 0.0000);
			}
			if (getDateMF(mf,y,x) > 255.0000){
				mf = setDateMF(mf, y, x, 255.0000);
			}
		}
	}
	return mf;
}


//Entradas: La matriz resultante de etapas anteriores.
//Funcionamiento: Realiza la etapa de pooling, en el cual obtiene de la matriz entrante el 
//                largo y ancho, para poder reducir la matriz resultante.
//Salidas: matrixf, correspondiente a la matrix resultando al aplicar el pooling y el cual 
//         sirve como entrada para las demas funciones.

matrixf *pooling(matrixf *mf){
	int heigth = 0, width = 0;
	if (countFil(mf)%2 == 0){
		heigth = countFil(mf)/2;
	}
	if (countFil(mf)%2 == 1){
		heigth = (countFil(mf)/2) + 1;
	}
	if (countColumn(mf)%2 == 0){
		width = countColumn(mf)/2;
	}
	if (countColumn(mf)%2 == 1){
		width = (countColumn(mf)/2) + 1;
	}
	matrixf *newmf = createMF(heigth, width);
	int fil = 0, col = 0, fil2 = 0, col2 = 0;
	while (fil < countFil(mf)){
		col2 = 0;
		col = 0;
		while (col < countColumn(mf)){
			float max = 0.0000;
			for (int y = 0; y < 2; y++){
				for (int x = 0; x < 2; x++){
					if (max < getDateMF(mf, y+fil, x+col)){
						max = getDateMF(mf, y+fil, x+col);
					}
				}
			}
			newmf = setDateMF(newmf, fil2, col2, max);
			col = col + 2;
			col2 = col2 + 1;
		}
		fil = fil + 2;
		fil2 = fil2 + 1;
	}
	return newmf;
}

//Entradas: La matriz resultante de etapas anteriores, como tambien el nombre de la imagen saliente (out_1.png).
//Funcionamiento: Realiza el proceso de escribir la matriz resultante, en una imagen de formato .png.
//Salidas: Void.

void escribirPNG(char *filename, matrixf *mf) {
  FILE *filepng = fopen(filename, "wb");
  png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  png_infop info = png_create_info_struct(png);
  png_init_io(png, filepng);
  png_set_IHDR(
    png,
    info,
    countColumn(mf), countFil(mf),
    8,
    PNG_COLOR_TYPE_RGB,
    PNG_INTERLACE_NONE,
    PNG_COMPRESSION_TYPE_BASE,
    PNG_FILTER_TYPE_BASE
  );
  png_write_info(png, info);  
  png_bytep *row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * countFil(mf));
  for(int y = 0; y < countFil(mf); y++) {
    row_pointers[y] = (png_byte*)malloc(png_get_rowbytes(png,info));
  }
  for(int y = 0; y < countFil(mf); y++) {
    for(int x = 0; x < countColumn(mf); x++) {
	  (&(row_pointers[y][x * 3]))[0] = (int)getDateMF(mf, y, x);
	  (&(row_pointers[y][x * 3]))[1] = (int)getDateMF(mf, y, x);
	  (&(row_pointers[y][x * 3]))[2] = (int)getDateMF(mf, y, x);
    }
  } 
  png_write_image(png, row_pointers);
  png_write_end(png, NULL); 
  for(int y = 0; y < countFil(mf); y++) {
    free(row_pointers[y]);
  }
  free(row_pointers);
  fclose(filepng);
  png_destroy_write_struct(&png, &info);
}

//Entradas: Lista de matrices, el umbral como entero, el cual indica el punto de clasificacion, el 
//          nombre de la imagen como char*.
//Funcionamiento: Clasifica las imagenes si cumplen con cierto criterio, si es nearly black o no.
//Salidas: listmf, el cual contiene las diferentes imagenes que fueron clasificadas.

listmf *classification(listmf *photothread, int umbral, char *namefile, int maxBlack, int actual, int maxfil, int mostrar){
	
	pthread_mutex_lock(&mutex2);
	for (int y = 0; y < countFil(getListMF(photothread,actual)); y++){
		for (int x = 0; x < countColumn(getListMF(photothread,actual)); x++){
			if (getDateMF(getListMF(photothread,actual), y, x) == 0.0000){
				maxBlack = maxBlack + 1;
			}
		}
	}
	pthread_mutex_unlock(&mutex2);
	pthread_barrier_wait(&barrier2);
	if (actual == maxfil-1){
		int cantrows = 0;
		for (int y = 0; y < lengthListMF(photothread); y++){
			for (int x = 0; x < countFil(getListMF(photothread,y)); x++){
				cantrows = cantrows + 1;
			}
		}
		int row = 0;
		matrixf *mf = createMF(cantrows,countColumn(getListMF(photothread,actual)));
		for (int z = 0; z < lengthListMF(photothread); z++){
			for (int y = 0; y < countFil(getListMF(photothread,z)); y++){
				for (int x = 0; x < countColumn(getListMF(photothread,z)); x++){
					mf = setDateMF(mf,row,x,getDateMF(getListMF(photothread,z), y, x));
				}
				row = row + 1;
			}
		}
		float porcentBlack = (maxBlack * 100.0000)/(countFil(mf) * countColumn(mf));

		if(mostrar==1){

			if (porcentBlack >= umbral){
				printf("|   %s   |         yes        |\n",namefile);
			}
			if (porcentBlack < umbral){
				printf("|   %s   |         no         |\n",namefile);
			}
			escribirPNG(namefile, mf);
		}
	}
	return photothread;
}


//Entradas: char** datefilter, int* cont.
//Funcionamiento: Permite convertir el filtro de 3x3, en una matriz.
//Salidas: matrixf, que contiene el contenido del filtro en matriz.

matrixf *convertFilter(char **datefilter, int cont){
	int colfilter = 1;
	for (int x = 0; x < strlen(datefilter[0]); x++){
		if ((datefilter[0][x] == '-') || (datefilter[0][x] == '0') || (datefilter[0][x] == '1') 
			|| (datefilter[0][x] == '2') || (datefilter[0][x] == '3') || (datefilter[0][x] == '4') 
		|| (datefilter[0][x] == '5') || (datefilter[0][x] == '6') || (datefilter[0][x] == '7') 
		|| (datefilter[0][x] == '8') || (datefilter[0][x] == '9')){
			colfilter = colfilter + 0;
		}
		else if (datefilter[0][x] == ' '){
			colfilter = colfilter + 1;
		}
	}
	matrixf *filter = createMF(cont, colfilter);
	int fil = 0, col = 0, pos = 0;
	char *digit = (char *)malloc(10*sizeof(char));
	for (int a = 0; a < cont; a++){
		col = 0;
		for (int b = 0; b < strlen(datefilter[a]); b++){
			if ((datefilter[a][b] == '-') || (datefilter[a][b] == '0') || (datefilter[a][b] == '1') 
			    || (datefilter[a][b] == '2') || (datefilter[a][b] == '3') || (datefilter[a][b] == '4') 
		        || (datefilter[a][b] == '5') || (datefilter[a][b] == '6') || (datefilter[a][b] == '7') 
		        || (datefilter[a][b] == '8') || (datefilter[a][b] == '9')){
			digit[pos] = datefilter[a][b];
			if (b == strlen(datefilter[a]) - 1){
				pos = 0;
				filter = setDateMF(filter, fil, col, (atoi(digit)) * 1.0000);
				col = col + 1;
				digit = (char *)malloc(10*sizeof(char));
			}
			pos = pos + 1;
			}
			else if((datefilter[a][b] == ' ') || (b == strlen(datefilter[a]) - 1)){
				pos = 0;
				filter = setDateMF(filter, fil, col, (atoi(digit)) * 1.0000);
				col = col + 1;
				digit = (char *)malloc(10*sizeof(char));
			}
		}
		fil = fil + 1;
	}
	return filter;
}


//Entradas: Se tienen las hebras entrantes, las cuales la cantidad de las mismas son indicas por el usuario.
//Funcionamiento: Trabaja con la hebra entrante. Si esta no posee la cantidad de filas correspondientes
//                para que pueda trabajar, entonces obtiene la informacion de buffer para almacenarla
//                impidiendo que otras hebras entren al buffer hasta que esta vacie lo necesario, o esperar
//                a que la hebra anterior desocupe el buffer para que lo pueda vaciar. En caso de que la hebra
//                tenga la cantidad de  filas de la imagen que le corresponden, entonces aplicara los procesos
//                de convolucion, rectificacion, pooling y clasificacion.
//Salidas: Void.

void *hebraConsumidora(void* hebras){

	int mostrar= args.mostrar;
	int *hebra = (int *) hebras;
	listmf *buffer = args.buffer;
	listmf *photothread = args.photothread;
	matrixf *filter = args.filter;
	int *datos = args.datos;
	char *imagenSalida=args.imagenSalida;
	
	if ((getListMF(photothread,*hebra)==NULL)||
	((getListMF(photothread,*hebra)!=NULL)&&(countFil(getListMF(photothread,*hebra))<datos[2]))){
		pthread_mutex_lock(&mutex);
		matrixf *newmf;
		int maxrow = 0;
		for (int x=0;x<lengthListMF(buffer);x++){
			if (((maxrow == datos[2])&&(*hebra<datos[3]-1))||((maxrow == datos[2]+datos[4])&&(*hebra==datos[3]-1))){
				break;
			}
			else{
				if (getListMF(buffer,x)!=NULL){
					if (getListMF(photothread,*hebra)==NULL){
						newmf = getListMF(buffer,x);
						photothread = setListMF(photothread,newmf,*hebra);
						buffer = setListMF(buffer,NULL,x);
						maxrow = maxrow + 1;
					}
					else{
						newmf = createMF(countFil(getListMF(photothread,*hebra))+1,countColumn(getListMF(buffer,x)));
						int fil = 0;
						for (int y=0;y<countFil(getListMF(photothread,*hebra));y++){
							for (int z=0;z<countColumn(getListMF(photothread,*hebra));z++){
								newmf = setDateMF(newmf,y,z,getDateMF(getListMF(photothread,*hebra),y,z));
							}
							fil = fil + 1;
						}
						for (int y=0;y<countFil(getListMF(buffer,x));y++){
							for (int z=0;z<countColumn(getListMF(buffer,x));z++){
								newmf = setDateMF(newmf,y+fil,z,getDateMF(getListMF(buffer,x),y,z));
							}
						}
						photothread = setListMF(photothread,newmf,*hebra);
						buffer = setListMF(buffer,NULL,x);
						maxrow = maxrow + 1;
					}
				}
			}
		}
		
		args.buffer=buffer;
		args.photothread=photothread;
		args.filter=filter;
		args.datos=datos;
		args.imagenSalida=imagenSalida;
		pthread_mutex_unlock(&mutex);
	}
	else if (*hebra < 0){
		int auxhebra = ((*hebra)*-1)-1;
		pthread_barrier_wait(&barrier);
		matrixf *mf=getListMF(photothread,auxhebra);
		mf = bidirectionalConvolution(mf,filter);
		mf = rectification(mf);
		mf = pooling(mf);
		photothread = setListMF(photothread,mf,auxhebra);
		photothread = classification(photothread,datos[0],imagenSalida,datos[1],auxhebra,datos[3], mostrar);
		
		args.buffer=buffer;
		args.photothread=photothread;
		args.filter=filter;
		args.datos=datos;
		args.imagenSalida=imagenSalida;
	}
}

// Funcion main: Funcion que toma por parametros los datos entrantes y pasa a la etapa de lectura,
//               la matriz del filtro para convulocion y el nombre de las imagenes.
// Entrada: los parametros ingresados por el usuario.
// Salida: Entero que representa fin de su ejecucion.

int main(int argc, char *argv[]){ /*Main principal de la funcion*/

    char *cflag = (char*)malloc(100*sizeof(char));
    char *mflag = (char*)malloc(100*sizeof(char));
    char *nflag = (char*)malloc(100*sizeof(char));
	char *hflag = (char*)malloc(100*sizeof(char));
	char *tflag = (char*)malloc(100*sizeof(char));
	char *uflag = (char*)malloc(100*sizeof(char));
    int numeroImagenes=0;
	int numeroHebras=0;
	int largoBuffer=0;
	int umbral=0;
    int caso;
    int mostrar=0;
    while((caso=getopt(argc,argv, "c:h:u:n:b:m:f"))!= -1){
        switch(caso){
            case 'c':
        
                strcpy(cflag, optarg); /*Numero Cantidad imagenes*/
        
                break;
            case 'h':
                strcpy(hflag, optarg); /*Numero Cantidad hebras*/
                break;    

            case 'u':
                strcpy(uflag, optarg); /*Umbral de binarizacion*/
                break;  

            case 'n':
                strcpy(nflag, optarg); /*Numero Umbral clasificacion*/
                break;	

            case 'b':
                strcpy(tflag, optarg); /*Numero Largo Buffer*/
                break;

            case 'm':
                strcpy(mflag, optarg); /*Archivo mascara filtro .txt*/
                break;  
        
            case 'f': /*Se muestra o no por pantalla*/
                mostrar=1;
                break;
             default:
                abort();          
        }   
    }
	char **datefilter = (char **)malloc(2000*sizeof(char *));
	char *date = (char *)malloc(2000*sizeof(char));
	FILE *filefilter = fopen(mflag,"r");

	int error = 0, cont = 0;
	while(error == 0){
		fseek(filefilter, 0, SEEK_END);
		if ((filefilter == NULL) || (ftell(filefilter) == 0)){
			perror("Error en lectura. Ingrese el nombre de un archivo existente.\n");
			error = 1;
		}
		else{
			date=(char*)malloc(2000*sizeof(char));
			rewind(filefilter);
			while(feof(filefilter) == 0){
				date = fgets(date, 1000, filefilter);
				datefilter[cont] = date;
				date = (char*)malloc(1000*sizeof(char));
				cont = cont + 1;
			}
			error = 1;
		}
	}
	rewind(filefilter);
	fclose(filefilter);
	matrixf *filter = convertFilter(datefilter, cont);
    numeroImagenes = atoi(cflag);
	numeroHebras = atoi(hflag);
	largoBuffer = atoi(tflag);
  	umbral = atoi(nflag);

  	if(mostrar==1){
  		printf("\n|     Imagen     |     Nearly Black     |\n");
  	}else{
  		printf("Fin del programa.\n");
  	}

  	
  	for(int image = 1; image <= numeroImagenes; image++){
		pthread_mutex_init(&mutex,NULL);
		pthread_mutex_init(&mutex2,NULL);
		pthread_barrier_init(&barrier, NULL, numeroHebras);
		pthread_barrier_init(&barrier2, NULL, numeroHebras);
		listmf *buffer = createArrayListMF(largoBuffer); /*Lista de matrices*/
		listmf *photothread = createArrayListMF(numeroHebras);
		matrixf *photomf;
		int width, height, fil, col;
		float date;
		png_byte color_type;
		png_byte bit_depth;
		png_bytep *row_pointers = NULL;
		char cantidadImg[10];
		char cantidadImgSalida[10];
	    sprintf(cantidadImg,"%d",image); /*Pasar de numero a string, cantidadImagen*/
	    sprintf(cantidadImgSalida,"%d",image); /*Pasar de numero a string, cantidadImagen*/
	    char *nombreFiltroConvolucion= mflag;
	    char imagenArchivo[] = "imagen_";
		char imagenSalida[] = "out_";
	    char extension[] = ".png"; 
	    char extension2[] = ".png";
	    strcat(imagenArchivo,cantidadImg);
	    strcat(imagenArchivo,extension); /*imagen_1.png*/
		strcat(imagenSalida,cantidadImgSalida);
		strcat(imagenSalida,extension2); /*out_1.png*/

		
		
		photomf = readPNG(imagenArchivo, photomf, width, height, color_type, bit_depth, row_pointers); /*Matriz de la imagen*/
		int rowsXthread = countFil(photomf)/numeroHebras; /*Numero de filas por hebra*/
		
		int aditionalRows = countFil(photomf)%numeroHebras; /*Numero de filas adicionales a ultima hebra*/
		int auxumbral = 0;
		int *datos=(int*)malloc(5*sizeof(int));
		datos[0]=umbral;
		datos[1]=auxumbral;
		datos[2]=rowsXthread;
		datos[3]=numeroHebras;
		datos[4]=aditionalRows;
		int *threads =(int*)malloc(numeroHebras*sizeof(int));
		/*Se guardan los datos en la estructura*/
		args.photothread = photothread;
		args.filter = filter;
		args.imagenSalida = imagenSalida;
		args.datos = datos;
		args.buffer = buffer;/*Se guarda el buffer aca, porque aqui se crea*/
		args.mostrar = mostrar;
		pthread_t *hebrasConsumidoras = (pthread_t *)malloc(numeroHebras*sizeof(pthread_t));
		for (int row=0;row<countFil(photomf);row++){
			matrixf *aux = createMF(1, countColumn(photomf));/*Matriz de una fila de la imagen con tantas columnas, vacia*/
			for (int x=0;x<countColumn(photomf);x++){
				aux=setDateMF(aux,0,x,getDateMF(photomf,row,x));
			}
			args.buffer = setListMF(args.buffer,aux,row%largoBuffer);
			if((fullListMF(args.buffer)==1)||(row==countColumn(photomf)-1)){
				for (int thread=0;thread<numeroHebras;thread++){
					threads[thread]=thread;
										
					pthread_create(&hebrasConsumidoras[thread],NULL,&hebraConsumidora,(void *)&threads[thread]);
				}
				for (int thread=0;thread<numeroHebras;thread++){
					
					pthread_join(hebrasConsumidoras[thread],NULL);
				}
			}
		}
		free(threads);
		threads =(int*)malloc(numeroHebras*sizeof(int));
		free(hebrasConsumidoras);
		hebrasConsumidoras = (pthread_t *)malloc(numeroHebras*sizeof(pthread_t));
		for (int thread=0;thread<numeroHebras;thread++){
			threads[thread]=(thread+1)*-1;
			
			pthread_create(&hebrasConsumidoras[thread],NULL,&hebraConsumidora,(void *)&threads[thread]);
		}
		for (int thread=0;thread<numeroHebras;thread++){
			
			pthread_join(hebrasConsumidoras[thread],NULL);
		}
  	}
}