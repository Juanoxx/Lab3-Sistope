# pragma once

//matrices de digitos flotantes.
struct matrixf{
	float digit;
	struct matrixf *up;
	struct matrixf *down;
	struct matrixf *left;
	struct matrixf *right;
};
typedef struct matrixf matrixF;

//Entradas: mf, dato tipo matrixF inicial.
//Funcionamiento: avanza una posicion a la derecha de la matrixF.
//Salidas: mf, dato tipo matrixF en la siguiente posicion.
matrixF *rightMF(matrixF *mf){
	matrixF *leftmf = mf;
	mf = mf->right;
	leftmf->right = mf;
	mf->left = leftmf;
	return mf;
}

//Entradas: mf, dato tipo matrixF inicial.
//Funcionamiento: avanza una posicion hacia abajo de la matrixF.
//Salidas: mf, dato tipo matrixF en la siguiente posicion.
matrixF *downMF(matrixF *mf){
	matrixF *upmf = mf;
	mf = mf->down;
	upmf->down = mf;
	mf->up = upmf;
	return mf;
}

//Entradas: mf, dato tipo matrixF inicial.
//Funcionamiento: regresa a la posicion inicial a la izquierda de la matrixF.
//Salidas: mf, dato tipo matrixF en la posicion inicial de la izquierda.
matrixF *startLeftMF(matrixF *mf){
	while (mf->left != NULL){
		mf = mf->left;
	}
	return mf;
}

//Entradas: mf, dato tipo matrixF inicial.
//Funcionamiento: regresa a la posicion inicial hacia arriba de la matrixF.
//Salidas: mf, dato tipo matrixF en la posicion inicial de arriba.
matrixF *startUpMF(matrixF *mf){
	while (mf->up != NULL){
		mf = mf->up;
	}
	return mf;
}

//Entradas: height, dato tipo int que representa la altura (cantidad de filas de la
//          matrixF); width, dato tipo int que presenta el ancho (cantidad de columnas
//          de la matrixF).
//Funcionamiento: crea una matrixF con (height) cantidad de filas y (width) cantidad
//                de columnas, donde cada dato de la matrixF inicia en 0.
//Salidas: mf, dato tipo matrixF nueva.
matrixF *createMF(int height, int width){
	matrixF *mf = (matrixF *)malloc(sizeof(matrixF));
	mf->digit = 0.0000;
	mf->up = NULL;
	mf->down = NULL;
	mf->left = NULL;
	mf->right = NULL;
	for (int cont = 0; cont < width - 1; cont++){
		matrixF *auxmf = (matrixF *)malloc(sizeof(matrixF));
		auxmf->up = NULL;
		auxmf->down = NULL;
		auxmf->left = NULL;
		auxmf->right = NULL;
		auxmf->digit = 0.0000;
		mf->right = auxmf;
		auxmf->left = mf;
		mf = rightMF(mf);
	}
	mf = startLeftMF(mf);
	for (int fil = 0; fil < height - 1; fil++){
		matrixF *newmf = (matrixF *)malloc(sizeof(matrixF));
		newmf->up = NULL;
		newmf->down = NULL;
		newmf->left = NULL;
		newmf->right = NULL;
		newmf->digit = 0.0000;
		for (int col = 0; col < width - 1; col++){
			matrixF *auxmf = (matrixF *)malloc(sizeof(matrixF));
			auxmf->up = NULL;
			auxmf->down = NULL;
			auxmf->left = NULL;
			auxmf->right = NULL;
			auxmf->digit = 0.0000;
			newmf->right = auxmf;
			auxmf->left = newmf;
			newmf = rightMF(newmf);
		}
		newmf = startLeftMF(newmf);
		mf->down = newmf;
		newmf->up = mf;
		mf = downMF(mf);
	}
	mf = startLeftMF(mf);
	mf = startUpMF(mf);
	return mf;
}

//Entradas: mf, dato tipo matrixF actual; fil, dato tipo int que representa la posicion en
//          en la fila; col, dato tipo int que representa la posicion en la columna; date,
//          dato tipo float que representa el digito flotante a agregar.
//Funcionamiento: cambia el valor en la posicion (fil,col) de matrixF por el dato date.
//Salidas: mf, dato tipo matrixF con el valor cambiado en la posicion especifica.
matrixF *setDateMF(matrixF *mf, int fil,int col, float date){
	if (fil < 0){
		fil = 0;
	}
	if (col < 0){
		col = 0;
	}
	int x = 0, y = 0;
	while ((y < fil) && (mf->down != NULL)){
		mf = downMF(mf);
		y = y + 1;
	}
	while ((x < col) && (mf->right != NULL)){
		mf = rightMF(mf);
		x = x + 1;
	}
	mf->digit = date;
	mf = startLeftMF(mf);
	mf = startUpMF(mf);
	return mf;
}

//Entradas: mf, dato tipo matrixF actual; fil, dato tipo int que representa la posicion en
//          en la fila; col, dato tipo int que representa la posicion en la columna.
//Funcionamiento: obtiene el dato en la posicion (fil,col) de la matrixF.
//Salidas: date, dato tipo float que corresponde al dato obtenido.
float getDateMF(matrixF *mf, int fil,int col){
	if (fil < 0){
		fil = 0;
	}
	if (col < 0){
		col = 0;
	}
	int x = 0, y = 0;
	while ((y < fil) && (mf->down != NULL)){
		mf = downMF(mf);
		y = y + 1;
	}
	while ((x < col) && (mf->right != NULL)){
		mf = rightMF(mf);
		x = x + 1;
	}
	float date = mf->digit;
	mf = startLeftMF(mf);
	mf = startUpMF(mf);
	return date;
}

//Entradas: mf, dato tipo matrixF actual.
//Funcionamiento: obtiene la cantidad de filas de la matrixF.
//Salidas: length, dato tipo int correspondiente a la cantidad de filas.
int countFil(matrixF *mf){
	int length = 1;
	while (mf->down != NULL){
		length = length + 1;
		mf = downMF(mf);
	}
	mf = startUpMF(mf);
	return length;
}

//Entradas: mf, dato tipo matrixF actual.
//Funcionamiento: obtiene la cantidad de columnas de la matrixF.
//Salidas: length, dato tipo int correspondiente a la cantidad de columnas.
int countColumn(matrixF *mf){
	int length = 1;
	while (mf->right != NULL){
		length = length + 1;
		mf = rightMF(mf);
	}
	mf = startLeftMF(mf);
	return length;
}

//Entradas: mf, dato tipo matrixF actual.
//Funcionamiento: agrega 2 filas y 2 columnas con datos iniciales 0 a cada extremo correspondiente
//                de la matrixF, aumentando la cantidad de filas en 2 y la cantidad de columnas en 2.
//Salidas: newmf, dato tipo matrixF correspondiente a la matrixF mf ampliada.
matrixF *amplifyMF(matrixF *mf){
	matrixF *newMF = createMF(countFil(mf) + 2, countColumn(mf) + 2);
	for (int fil = 0; fil < countFil(mf); fil++){
		for (int col = 0; col < countColumn(mf); col++){
			newMF = setDateMF(newMF, fil + 1, col + 1, getDateMF(mf, fil, col));
		}
	}
	return newMF;
}

//Entradas: mf, dato tipo matrixF actual.
//Funcionamiento: quita 2 filas y 2 columnas de cada uno de los extremos correspondientes de la matrixF,
//                disminuyendo la cantidad de filas en 2 y la cantidad de columnas en 2.
//Salidas: newmf, dato tipo matrixF correspondiente a la matrixF mf reducida.
matrixF *decreaseMF(matrixF *mf){
	matrixF *newMF = createMF(countFil(mf) - 2, countColumn(mf) - 2);
	for (int fil = 0; fil < countFil(newMF); fil++){
		for (int col = 0; col < countColumn(newMF); col++){
			newMF = setDateMF(newMF, fil, col, getDateMF(mf, fil + 1, col + 1));
		}
	}
	return newMF;
}