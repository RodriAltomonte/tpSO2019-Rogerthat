/*
 * FuncionesConexiones.h
 *
 *  Created on: 2 sep. 2018
 *      Author: utnso
 */

#ifndef FUNCIONESCONEXIONES_H_
#define FUNCIONESCONEXIONES_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <commons/string.h>
#include <commons/log.h>
#include <commons/config.h>

typedef struct{
	int a;
	char key[40];
	char *value;
}Paquete;

extern const int SET;
extern const int GET;
extern const int STORE;

//Path de los servidores
extern const char *pathSAFA;
extern const char *pathDAM;
extern const char *pathMDJ;
extern const char *pathFM9;

//Los pongo en escritorio para que no tengamos problemas al commitear
extern const char *logSAFA;
extern const char *logDAM;
extern const char *logMDJ;
extern const char *logFM9;
extern const char *logCPU;
t_log *logger;

//Funciones utilizadas en general
struct sockaddr_in dameUnaDireccion(char *path,int ipAutomatica);
int crearConexionCliente(char*path);
int crearConexionServidor(char*path);
/*int transformarNumero(char *a,int start);
Paquete deserializacion(char* texto);
Paquete recibir(int socket);

char* transformarTamagnoKey(char key[]);
void serealizarPaquete(Paquete pack,char** buff);
void enviar(int socket,Paquete pack);*/
int ejemplo();

#endif /* FUNCIONESCONEXIONES_H_ */
