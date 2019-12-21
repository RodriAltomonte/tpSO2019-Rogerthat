#ifndef DAM_H_
#define DAM_H_

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


sem_t sem_esperaInicializacion;


//typedef enum {nuevo,bloqueado,listo,ejecucion,finalizado}Estado;
//
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
//typedef struct{
//	int nombre_tam; //nuevo
//	char *nombre;
//	int direccionContenido_tam; //nuevo
//	char* direccionContenido;
//} DatosArchivo;
//
//typedef struct{
//	int tipoOperacion;  //numero palabras reservadas lenguaje escriptorio
//	int resultado;		//1 = sePudoCompletar ; 0 = noSePudo
//	int idProcesoSolicitante;
//	DatosArchivo archivoAsociado;
////	char *informacionExtra;
//} Paquete_DAM;

pthread_t hiloSAFA;
pthread_t hiloMDJ;
pthread_t hiloCPU;
pthread_t hiloFM9;
pthread_t* hilosCPUs = NULL;
Proceso *DTB;
int sockSAFA;
int sockFM9;
int sockDAM ;
int sockMDJ;
sem_t sem_termine_Ejecutar_Script;
sem_t sem_Ejecutar_Script;

struct sockaddr_in dameUnaDireccion(char *path,int ipAutomatica);
int crearConexionCliente(char*path);
int crearConexionServidor(char*path);
void *ejecutarCodigoEscriptorio();
int parserSentencia(char* sentencia);
char* parser( char* aParsear);
//Proceso* deserializarProceso(void * buffer);
//int recibirPaqueteDAM(int socketSender, Paquete_DAM * paqueteDAM);
//int enviarPaqueteDAM(int socketProcesoEnviar, Paquete_DAM * paqueteDAM);
//void liberarPaqueteDAM(Paquete_DAM * p);

#endif /* CPU_H_ */
