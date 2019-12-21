/*
 * MDJ.h
 *
 *  Created on: 1 nov. 2018
 *      Author: utnso
 */

#ifndef MDJ_H_
#define MDJ_H_

int socketDAM;
t_config *configMDJ;
int retardo;
char* puntoMontaje;
int puerto;
char* rutaBloque;
char* rutaBitMap;
char* rutaArchivoActual;
int tamanioBloque;
int cantidadBloques;
char* map;
int sizeBitmap;

t_bitarray *bitmap;

void atenderDAM();
int validarArchivo(char* nombreArchivo, int* size);
char* obtenerDatosArchivo(char* nombreArchivo ,int offset, int size);
int borrarArchivo(char* nombreArchivo);
int crearArchivo(char* nombreArchivo,int cantBytes);
int guardarDatosArchivo(char* nombreArchivo, int offset, int size, char* codigoArchivo);
void testeoMDJ();
void inicializarVariables();
int asignarNuevosBloques(char *codigoArchivo,int posicion,int cantBloquesFaltantes, int cantidadBloquesTotales, char** bloques, int nuevoTamanioArchivo);
void actualizarScriptArchivo(char** bloques,t_list* bloqueNuevo,int nuevoTamanioArchivo);
#endif /* MDJ_H_ */
