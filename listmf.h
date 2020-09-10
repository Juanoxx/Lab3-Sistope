# pragma once

//Lista de matrices flotantes (listmf de mf).
struct listmf{
	matrixf *mf;
	struct listmf *next;
	struct listmf *previous;
};
typedef struct listmf listmf;

//Entradas: mf, dato tipo matrixf inicial.
//Funcionamiento: crea una listmf inicial con un unico dato matrixf.
//Salidas: listmatrixf, dato tipo listmf nueva.
listmf *createListMF(matrixf *mf){
	listmf *listmatrixf=(listmf *)malloc(sizeof(listmf));
	listmatrixf->mf=mf;
	listmatrixf->next=NULL;
	listmatrixf->previous=NULL;
	return listmatrixf;
}

//Entradas: listmatrixf, dato tipo listmf actual.
//Funcionamiento: avanza a la siguiente posicion de la listmf.
//Salidas: listmatrixf, dato tipo listmf en la siguiente posicion.
listmf *nextListMF(listmf *listmatrixf){
	listmf* previous=listmatrixf;
	listmatrixf=listmatrixf->next;
	previous->next=listmatrixf;
	listmatrixf->previous=previous;
	return listmatrixf;
}

//Entradas: listmatrixf, dato tipo listmf actual.
//Funcionamiento: vuelve al inicio de la listmf.
//Salidas: listmatrixf, dato tipo listmf en la posicion inicial.
listmf *initialListMF(listmf *listmatrixf){
	while (listmatrixf->previous!=NULL){
		listmatrixf=listmatrixf->previous;
	}
	return listmatrixf;
}

//Entradas: listmatrixf, dato tipo listmf.
//Funcionamiento: obtiene el largo de listmf contando la cantidad de mfs que posee. 
//                Si tiene solo un elemento y es nulo, entonces el largo es 0.
//Salidas: length, dato tipo int que representa el largo de la listmf.
int lengthListMF(listmf *listmatrixf){
	listmf *list_matrixf=listmatrixf;
	int length=0,fin=0;
	/*if (list_matrixf->mf==NULL){
		return length;
	}*/
	while(fin==0){
		if(list_matrixf->next==NULL){
			length=length+1;
			fin=1;
		}
		else{
			length=length+1;
			list_matrixf=nextListMF(list_matrixf);
		}
	}
	list_matrixf=initialListMF(list_matrixf);
	return length;
}

//Entradas: listmatrixf, dato tipo listmf actual; mf, dato tipo matrixf a insertar; 
//          pos, dato tipo int que indica la posicion a insertar.
//Funcionamiento: inserta el dato mf en la posicion pos de listmf, incrementando su 
//                largo. Si pos es mayor al largo de listmf, se inserta al final. Si
//                pos es menor o igual a 0, entonces se inserta al inicio. Si listmf 
//                tiene un solo elemento y es nulo, entonces reemplaza el elemento por mf.
//Salidas: listmatrixf, dato tipo listmf con mf insertado.
listmf *insertListMF(listmf *listmatrixf,matrixf *mf,int pos){
	/*if ((lengthListMF(listmatrixf)==1)&&(listmatrixf->mf==NULL)){
		listmatrixf->mf=mf;
		listmatrixf->next=NULL;
		listmatrixf->previous=NULL;
		return listmatrixf;
	}*/
	/*else */if (pos==0){
		listmf *newlistmatrixf = createListMF(mf);
		newlistmatrixf->previous=NULL;
		newlistmatrixf->next=listmatrixf;
		listmatrixf->previous=newlistmatrixf;
		listmatrixf=initialListMF(listmatrixf);
		return listmatrixf;
	}
	else if(pos>=lengthListMF(listmatrixf)){
		while (listmatrixf->next!=NULL){
			listmatrixf=nextListMF(listmatrixf);
		}
		listmf *newlistmatrixf = createListMF(mf);
		newlistmatrixf->previous=listmatrixf;
		listmatrixf->next=newlistmatrixf;
		listmatrixf=initialListMF(nextListMF(listmatrixf));
		return listmatrixf;
	}
	else{
		for(int x=0;x<pos-1;x++){
			listmatrixf=nextListMF(listmatrixf);
		}
		listmf *newlistmatrixf = createListMF(mf);
		listmf *auxlistmatrixf=listmatrixf->next;
		newlistmatrixf->previous=listmatrixf;
		newlistmatrixf->next=auxlistmatrixf;
		auxlistmatrixf->previous=newlistmatrixf;
		listmatrixf->next=newlistmatrixf;
		listmatrixf=initialListMF(nextListMF(listmatrixf));
		return listmatrixf;
	}
}

//Entradas: length, dato tipo int que indica el largo para la listmf.
//Funcionamiento: crea una listmf de largo length con todas sus posiciones nulas.
//Salidas: listmatrixf, dato tipo listmf nueva vacia.
listmf *createArrayListMF(int length){
	listmf *newlistmatrixf = createListMF(NULL);
	while (length>lengthListMF(newlistmatrixf)){
		newlistmatrixf = insertListMF(newlistmatrixf,NULL,0);
	}
	return newlistmatrixf;
}

//Entradas: listmatrixf, dato tipo listmf actual; mf, dato tipo matrixf a insertar; 
//          pos, dato tipo int que indica la posicion a insertar.
//Funcionamiento: cambia el dato ubicado en la posicion pos de listmf por el dato mf.
//Salidas: listmatrixf, dato tipo listmf el dato cambiado.
listmf *setListMF(listmf *listmatrixf, matrixf *mf,int pos){
	if (pos<0){
		pos=0;
	}
	int x=0;
	while ((x<pos)&&(listmatrixf->next!=NULL)){
		listmatrixf=nextListMF(listmatrixf);
		x=x+1;
	}
	listmatrixf->mf=mf;
	listmatrixf=initialListMF(listmatrixf);
	return listmatrixf;
}

//Entradas: listmatrixf, dato tipo listmf actual; pos, dato tipo int que indica la 
//          posicion a insertar.
//Funcionamiento: obtiene el dato matrixf en la posicion pos de listmf.
//Salidas: resultmf, dato tipo matrixf obtenido de la listmf.
matrixf *getListMF(listmf *listmatrixf,int pos){
	if (pos<0){
		pos=0;
	}
	int x=0;
	while ((x<pos)&&(listmatrixf->next!=NULL)){
		listmatrixf=nextListMF(listmatrixf);
		x=x+1;
	}
	matrixf *resultmf=listmatrixf->mf;
	listmatrixf=initialListMF(listmatrixf);
	return resultmf;
}

//Entradas: listmatrixf, dato tipo listmf actual.
//Funcionamiento: crea una nueva listmf con los mismos datos de la listmf original.
//Salidas: copylistmatrixf, dato tipo listmf copia del listmf de la original.
listmf *copyListMF(listmf *listmatrixf){
	listmf *copylistmatrixf=createListMF(NULL);
	for(int x=0;x<lengthListMF(listmatrixf);x++){
		copylistmatrixf=insertListMF(copylistmatrixf,getListMF(listmatrixf,x),x);
	}
	return copylistmatrixf;
}

//Entradas: listmatrixf, dato tipo listmf actual.
//Funcionamiento: verifica si la listmf esta vacia.
//Salidas: empty, tipo de dato int. Es 1 si todos los datos de listmf son nulos o
//         es 0 si alguno no es 0.
int emptyListMF(listmf *listmatrixf){
	int empty = 1;
	for (int i=0;i<lengthListMF(listmatrixf);i++){
		if (getListMF(listmatrixf,i)!=NULL){
			empty = 0;
			break;
		}
	}
	return empty;
}

//Entradas: listmatrixf, dato tipo listmf actual.
//Funcionamiento: verifica si la listmf esta llena.
//Salidas: full, tipo de dato int. Es 1 si ningun dato de listmf es nulo o es 0 si
//         alguno no es nulo.
int fullListMF(listmf *listmatrixf){
	int full = 1;
	for (int i=0;i<lengthListMF(listmatrixf);i++){
		if (getListMF(listmatrixf,i)==NULL){
			full = 0;
			break;
		}
	}
	return full;
}