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
#include <commons/collections/list.h>
#include <unistd.h>
//#include <stdbool.h>
//#include <commons/bitarray.h>

typedef struct{
	int a;
	char key[40];
	char *value;
}Paquete;

typedef enum {nuevo,bloqueado,listo,ejecucion,finalizado}Estado;


typedef struct{  //Refactorizar Serializar
	char *nombre;
	char* direccionContenido;
	int numeroSegmento;
	int numeroPagina;
	int programCounter; //para ver que linea del archivo (escriptorio) se quiere ejecutar empieaza en 0
} DatosArchivo;

typedef struct{
	int idProceso;
	int socketProceso;
	char *pathEscriptorio;
	int flagInicializacion ;
	t_list *archivosAsociados;
	Estado estado;
	int seEstaCargando;
	t_list *recursosRetenidos;
}Proceso;



typedef struct{
	int tipoOperacion;  //numero palabras reservadas lenguaje escriptorio
	int resultado;		//1 = sePudoCompletar ; 0 = noSePudo
	int idProcesoSolicitante;
	DatosArchivo archivoAsociado;
} Paquete_DAM;

extern const int SET;
extern const int GET;
extern const int STORE;

//Path de los servidores
extern const char *pathSAFA;
extern const char *pathDAM;
extern const char *pathMDJ;
extern const char *pathFM9;
extern const char *pathCPU;

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
void ejecutarRetardo(char *path);

//Funciones Proceso
int enviarProceso(int socketProcesoEnviar, Proceso * proceso);
int recibirProceso(int socketSender, Proceso * proceso);

//Funciones DatosArchivo
int enviarDatosArchivo(int socketProcesoEnviar, DatosArchivo * datoArchivo);
int recibirDatosArchivo(int socketSender, DatosArchivo * datoArchivo);

//Funciones Paquete_DAM

/**
*  @DESC: envia un paquete dam
*/
int enviarPaqueteDAM(int socketProcesoEnviar, Paquete_DAM * paqueteDAM);
/**
*  @DESC: recibe un paquete dam
*/
int recibirPaqueteDAM(int socketSender, Paquete_DAM * paqueteDAM);
/**
*  @DESC: libera un paquete dam
*/
void liberarPaqueteDAM(Paquete_DAM * p);
/**
*  @DESC: devuelve un puntero paquete dam con variables en default
*/
Paquete_DAM* inicializarPaqueteDam();




/*int transformarNumero(char *a,int start);
Paquete deserializacion(char* texto);
Paquete recibir(int socket);

char* transformarTamagnoKey(char key[]);
void serealizarPaquete(Paquete pack,char** buff);
void enviar(int socket,Paquete pack);*/
int ejemplo();

#endif /* FUNCIONESCONEXIONES_H_ */
