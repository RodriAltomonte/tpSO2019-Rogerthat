/*
 * S-AFA.h
 *
 *  Created on: 2 sep. 2018
 *      Author: utnso
 */

#ifndef CPU_H_
#define CPU_H_

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <FuncionesConexiones.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <pthread.h>
#include <semaphore.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <commons/string.h>
#include <commons/config.h>

//typedef enum {nuevo,bloqueado,listo,ejecucion,finalizado}Estado;
//typedef struct{
//	int idProceso;
//	int socketProceso;
//	char *pathEscriptorio;
//	int flagInicializacion ;
//	t_list *archivosAsociados;
//	Estado estado;
//	int seEstaCargando;
//}Proceso;
//
//
//typedef struct{
//	char *nombre;
//	char* direccionContenido;
//} DatosArchivo;
//
//typedef struct{
//	int tipoOperacion;  //numero palabras reservadas lenguaje escriptorio
//	int resultado;		//1 = sePudoCompletar ; 0 = noSePudo
//	int idProcesoSolicitante;
//	DatosArchivo archivoAsociado;
//} Paquete_DAM;

pthread_t hiloEjecutarScript;
//Paquete_DAM * paqueteActual; //me lo tiene que mandar el safa
Proceso *DTB;
int instructionPointer;
int sockSAFA;
int sockDAM;
int sockFM9;
char* archivoAbuscar;
sem_t sem_termine_Ejecutar_Script;
sem_t sem_Ejecutar_Script;
int flagError;
char* archivoABuscar;
struct sockaddr_in dameUnaDireccion(char *path,int ipAutomatica);
int crearConexionCliente(char*path);
int crearConexionServidor(char*path);
void *ejecutarCodigoEscriptorio();
int parserSentencia(char* sentencia);
char* parser( char* aParsear);
bool archivoAbierto(void* a);
char* pedirSentencia();
bool esArchivoBuscado(void *archivo);

//Proceso* deserializarProceso(void * buffer);


#endif /* CPU_H_ */
