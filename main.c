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
#include "matrixf.h"
#include "listmf.h"
#include "funciones.h"
#include <stdint.h>
#include <math.h>
#include "jpeglib.h"
#include <setjmp.h>

matrixF *convertFilter(char **datefilter, int cont);

matrixF *conversion(matrixF *mf);

matrixF *filtracion(matrixF *mf, matrixF *filter);

matrixF *binarizacion(matrixF *mf, int umbral);

GLOBAL(void) escribirJPG(char *nombre, matrixF *mf, int fil, int col);
void clasificacion(matrixF *mf, int umbral, char *namefile, int aux);

METHODDEF(void) my_error_exit (j_common_ptr cinfo);

GLOBAL(matrixF*) leerJPG(char *nombre);

/*Estructura para manejar imágenes tipo .jpg*/
struct my_error_mgr {
  struct jpeg_error_mgr pub;	
  jmp_buf setjmp_buffer;
};
typedef struct my_error_mgr * my_error_ptr;

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


METHODDEF(void)
my_error_exit (j_common_ptr cinfo){
	my_error_ptr myerr = (my_error_ptr) cinfo->err;
	(*cinfo->err->output_message) (cinfo);
	longjmp(myerr->setjmp_buffer, 1);
}

/*Función que se encarga de leer imágen en formato .jpg*/
/*Entrada: Imágen jpg.*/
/* Salida: Imagen leída, caso contrario retorna cero*/
GLOBAL(matrixF*)
leerJPG(char *nombre){
	struct jpeg_decompress_struct cinfo;
	struct my_error_mgr jerr;
	FILE * imagen;
	JSAMPARRAY buffer;
	int row_stride;
	if ((imagen = fopen(nombre, "rb")) == NULL) {
		fprintf(stderr, "can't open %s\n", nombre);
		return 0;
	}
	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = my_error_exit;
	if (setjmp(jerr.setjmp_buffer)) {
		jpeg_destroy_decompress(&cinfo);
		fclose(imagen);
		return 0;
	}
	jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo, imagen);
	(void) jpeg_read_header(&cinfo, TRUE);
	(void) jpeg_start_decompress(&cinfo);
	row_stride = cinfo.output_width * cinfo.output_components;
	unsigned char* filaPixel = (unsigned char*)malloc( cinfo.output_width*cinfo.output_height*cinfo.num_components );
	buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);
	int acum = 0;
	while (cinfo.output_scanline < cinfo.output_height) {
		(void) jpeg_read_scanlines(&cinfo, buffer, 1);
		for(int i = 0;i < cinfo.image_width*cinfo.num_components;i++){
			filaPixel[i + acum] = buffer[0][i];
			if (i == cinfo.image_width*cinfo.num_components - 1){
				acum = acum + i + 1;
			}
		}
	}
	matrixF *mf = createMF(cinfo.image_height, cinfo.image_width*3);
	for (int i = 0; i < cinfo.image_height; i++){
		for(int j = 0; j < cinfo.image_width; j++)
		{
			mf = setDateMF(mf,i,j*3,(float)filaPixel[(i*cinfo.image_width*3)+(j*3)+0]);
			mf = setDateMF(mf,i,j*3 + 1,(float)filaPixel[(i*cinfo.image_width*3)+(j*3)+1]);
			mf = setDateMF(mf,i,j*3 + 2,(float)filaPixel[(i*cinfo.image_width*3)+(j*3)+2]);
		}
	}
	(void) jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
	fclose(imagen);
	return mf;
}

// Funcion: Escribe un archivo en formato png, resultante
// Entrada: en nombre del archivo y lamatriz resultante.
// Salida: void
GLOBAL(void)
escribirJPG(char *nombre, matrixF *mf, int fil, int col){
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	FILE * outfile;		/* target file */
	JSAMPROW row_pointer[1];	/* pointer to JSAMPLE row[s] */
	int row_stride;		/* physical row width in image buffer */
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);
	if ((outfile = fopen(nombre, "wb")) == NULL) {
		fprintf(stderr, "can't open %s\n", nombre);
		exit(1);
	}

	jpeg_stdio_dest(&cinfo, outfile);
	cinfo.image_width = col; 	/* image width and height, in pixels */
	cinfo.image_height = fil;
	cinfo.input_components = 3;		/* # of color components per pixel */
	cinfo.in_color_space = JCS_RGB; 	/* colorspace of input image */
	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, 10, TRUE /* limit to baseline-JPEG values */);
	jpeg_start_compress(&cinfo, TRUE);
	row_stride = col * 3;	/* JSAMPLEs per row in image_buffer */
	JSAMPLE *buffer = (JSAMPLE*)malloc(fil*col*3*sizeof(JSAMPLE));
	unsigned char* pixel_row = (unsigned char*)(buffer);
	for (int i = 0; i < cinfo.jpeg_height; i++){
		for(int j = 0; j < cinfo.jpeg_width; j++)
		{
			pixel_row[(i*cinfo.jpeg_width*3)+(j*3)+0]=(unsigned char)((int)getDateMF(mf, i, j));
			pixel_row[(i*cinfo.jpeg_width*3)+(j*3)+1]=(unsigned char)((int)getDateMF(mf, i, j));
			pixel_row[(i*cinfo.jpeg_width*3)+(j*3)+2]=(unsigned char)((int)getDateMF(mf, i, j));
		}
	}
	buffer = pixel_row;
	while (cinfo.next_scanline < cinfo.image_height) {
		row_pointer[0] = &buffer[cinfo.next_scanline * row_stride];
		(void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
	}
	jpeg_finish_compress(&cinfo);
	fclose(outfile);
	jpeg_destroy_compress(&cinfo);
}

/*Función que se encarga de convertir pixeles en escala de grises*/
/*Entrada: pixeles, alto y largo.*/
/* Salida: Matriz con escala de grises*/
matrixF *conversion(matrixF *mf) {
	matrixF *newmf = createMF(countFil(mf), countColumn(mf)/3);
	for(int y = 0; y < countFil(newmf); y++) {
		for(int x = 0; x < countColumn(newmf); x++) {
			float prom = getDateMF(mf,y,x*3)*0.299+getDateMF(mf,y,x*3 + 1)*0.587+getDateMF(mf,y,x*3 + 2)*0.114;
			newmf = setDateMF(newmf, y, x, prom);
		}
	}
	return newmf;
}

/*Función que se encarga de convertir el filtro en una matriz para aplicar la convolución*/
/*Entrada: Valores de convolución y contador.*/
/* Salida: EMatriz de convolución*/

matrixF *convertFilter(char **datefilter, int cont){
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
	matrixF *filter = createMF(cont, colfilter);
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

/*Función que se encarga de realizar la convolución a la imágen e imprimirla si se introdujo -b*/
/*Entrada: imagen y filtro.*/
/*Salida: Matriz convolucionada*/
matrixF *filtracion(matrixF *mf, matrixF *filter){
	if ((countFil(filter) == countColumn(filter))&&(countFil(filter)%2 == 1)){
		int increase = 0, initial = countFil(filter);
		while (initial != 1){
			initial = initial - 2;
			increase = increase + 1;
		}
		
		matrixF *newmf = createMF(countFil(mf),countColumn(mf));
		for (int cont = 0; cont < increase; cont++){
			mf = amplifyMF(mf);
		}
		for (int fil = increase; fil < countFil(mf) - increase; fil++){
			for (int col = increase; col < countColumn(mf) - increase; col++){
				float sum = 0.0000;
				for (int y = 0; y < countFil(filter); y++){
					for (int x = 0; x < countColumn(filter); x++){
						float result = getDateMF(filter, y, x)*getDateMF(mf, y + fil - increase, x + col - increase);
						sum = sum + result;
					}
				}
				newmf = setDateMF(newmf, fil - increase, col - increase, sum);
				
			}
		}
		for (int cont2 = 0; cont2 < increase; cont2++){
			mf = decreaseMF(mf);
		}
		
		return newmf;
	}
	else{
		return mf;
	}
}
/*Función que se encarga de convertir pixeles en valores binarios (0 o 255)*/
/*Entrada: matriz y umbral.*/
/*Salida: Matriz binaria*/
matrixF *binarizacion(matrixF *mf, int umbral){
  for (int y = 0; y < countFil(mf); y++){
    for (int x = 0; x < countColumn(mf); x++){
      if (getDateMF(mf,y,x) <= umbral){
        mf = setDateMF(mf, y, x, 0.0000);
      }
      else{
        mf = setDateMF(mf, y, x, 255.0000);
      }
    }
  }
  
  return mf;
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
		matrixF *mf = createMF(cantrows,countColumn(getListMF(photothread,actual)));
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
			escribirJPG(namefile, mf, cantrows, countColumn(getListMF(photothread,actual)));
		}
	}
	return photothread;
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
	matrixF *filter = args.filter;
	int *datos = args.datos;
	char *imagenSalida=args.imagenSalida;
	if ((getListMF(photothread,*hebra)==NULL)||
	((getListMF(photothread,*hebra)!=NULL)&&(countFil(getListMF(photothread,*hebra))<datos[2])&&(*hebra<datos[3]-1))||
	((getListMF(photothread,*hebra)!=NULL)&&(countFil(getListMF(photothread,*hebra))<datos[2]+datos[4])&&(*hebra==datos[3]-1))){
		pthread_mutex_lock(&mutex);
		matrixF *newmf;
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
						if (countFil(getListMF(photothread,*hebra)) == datos[2]){
							break;
						}
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
						if (((countFil(getListMF(photothread,*hebra)) == datos[2])&&(*hebra<datos[3]-1))||
						((countFil(getListMF(photothread,*hebra)) == datos[2]+datos[4])&&(*hebra==datos[3]-1))){
							break;
						}
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
		matrixF *mf=getListMF(photothread,auxhebra);
		mf = conversion(mf);
		mf = filtracion(mf,filter);
		mf = binarizacion(mf,datos[5]);
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
	int umbralC=0;
	int umbralB=0;
    int caso;
    int mostrar=0;
    while((caso=getopt(argc,argv, "c:h:u:n:b:m:f"))!= -1){
        switch(caso){
            case 'c':
        
                strcpy(cflag, optarg); /*Cantidad de imagenes*/
        
                break;
            case 'h':
                strcpy(hflag, optarg); /*Cantidad de hebras*/
                break;    

            case 'u':
                strcpy(uflag, optarg); /*Umbral de binarizacion*/
                break;  

            case 'n':
                strcpy(nflag, optarg); /*Umbral de clasificacion*/
                break;	

            case 'b':
                strcpy(tflag, optarg); /*Largo del buffer*/
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
	matrixF *filter = convertFilter(datefilter, cont);
    numeroImagenes = atoi(cflag);
	numeroHebras = atoi(hflag);
	largoBuffer = atoi(tflag);
  	umbralC = atoi(nflag);
	umbralB = atoi(uflag);

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
		matrixF *photomf;
		char cantidadImg[10];
		char cantidadImgSalida[10];
	    sprintf(cantidadImg,"%d",image); /*Pasar de numero a string, cantidadImagen*/
	    sprintf(cantidadImgSalida,"%d",image); /*Pasar de numero a string, cantidadImagen*/
	    char *nombreFiltroConvolucion= mflag;
	    char imagenArchivo[] = "imagen_";
		char imagenSalida[] = "out_";
	    char extension[] = ".jpg"; 
	    char extension2[] = ".jpg";
	    strcat(imagenArchivo,cantidadImg);
	    strcat(imagenArchivo,extension); /*imagen_1.png*/
		strcat(imagenSalida,cantidadImgSalida);
		strcat(imagenSalida,extension2); /*out_1.png*/
		
		photomf = leerJPG(imagenArchivo); /*Matriz de la imagen*/
		int rowsXthread = countFil(photomf)/numeroHebras; /*Numero de filas por hebra*/
		
		int aditionalRows = countFil(photomf)%numeroHebras; /*Numero de filas adicionales a ultima hebra*/
		int auxumbral = 0;
		int *datos=(int*)malloc(6*sizeof(int));
		datos[0]=umbralC;
		datos[1]=auxumbral;
		datos[2]=rowsXthread;
		datos[3]=numeroHebras;
		datos[4]=aditionalRows;
		datos[5]=umbralB;

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
			matrixF *aux = createMF(1, countColumn(photomf));/*Matriz de una fila de la imagen con tantas columnas, vacia*/
			for (int x=0;x<countColumn(photomf);x++){
				aux=setDateMF(aux,0,x,getDateMF(photomf,row,x));
			}
			args.buffer = setListMF(args.buffer,aux,row%largoBuffer);
			if((fullListMF(args.buffer)==1)||(row==countFil(photomf)-1)){
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