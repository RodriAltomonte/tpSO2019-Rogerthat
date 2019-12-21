/*
 * S-AFA.h
 *
 *  Created on: 2 sep. 2018
 *      Author: utnso
 */

#ifndef S_AFA_H_
#define S_AFA_H_

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

int idBuscar;// esto es por ahora
int idCPUBuscar;
int idGlobal;// esto es por ahora
int bloquearDT;
int idBanquero;
char* archivoAbuscar;
char *claveABuscar;
t_list *procesos_nuevos;
t_list *procesos_listos;
t_list *procesos_terminados;
t_list *procesos_bloqueados;

t_list *procesos_listos_MayorPrioridad ; //para VIRTURAL ROUND ROBIN

sem_t sem_replanificar;
sem_t sem_procesoEnEjecucion;
sem_t sem_CPU_ejecutoUnaSentencia;
sem_t sem_finDeEjecucion;
sem_t sem_liberarRecursos;
sem_t sem_liberador;
sem_t sem_estadoOperativo;
sem_t sem_pausar;
sem_t sem_cargarProceso;
sem_t sem_esperaInicializacion;
pthread_t hiloDummy;
pthread_t hiloEjecucionProceso;
pthread_t hiloPLPcargaAmemoria;
pthread_mutex_t mutex_pausa;

int flag_seEnvioSignalPlanificar;
int flag_quierenDesalojar;
//
sem_t sem_finDeEsiCompleto;
sem_t semCambioEstado;

int flag_desalojo;
//
int flag_nuevoProcesoEnListo;
//
int tiempo_de_ejecucion;
//
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
//	int nombre_tam; //nuevo
//	char *nombre;
//	int direccionContenido_tam; //nuevo
//	char* direccionContenido;
//} DatosArchivo;

typedef struct{
	Proceso *proceso;
	int quantumPrioridad;
} ProcesoBloqueado;

pthread_mutex_t planiCorto;

Proceso *procesoEnEjecucion;

t_list *cpu_conectadas;
typedef struct{
	int socketCPU;
	int tieneDTBasociado; //1:Si ; 0:NO
} ConexionCPU;

t_list *recursos;
typedef struct{
	char* nombre;
	int instancias;
	t_list *procesosEnEspera;
} Recurso;

char* recursoABuscar;

//

//typedef struct{
//	int tipoOperacion;  //numero palabras reservadas lenguaje escriptorio
//	int resultado;		//1 = sePudoCompletar ; 0 = noSePudo
//	int idProcesoSolicitante;
//	DatosArchivo archivoAsociado;
////	char *informacionExtra;
//} Paquete_DAM;

struct sockaddr_in dameUnaDireccion(char *path,int ipAutomatica);
int crearConexionCliente(char*path);
int crearConexionServidor(char*path);
void* conexionDAM(void* nuevoCliente);
int guard(int n, char * err);
void planificadorLargoPlazo(int sockCPU, char* pathEscriptorioCPU);
bool procesoEsIdABuscar(void * proceso);
bool procesoBloqueadoEsIdABuscar(void * proceso);
bool cpuUsadaEsIdABuscar(void * cpu);
void mostrarInfoArchivoAsociado(DatosArchivo * archivoAsociado);
//Paquete_DAM* deserializarPaqueteDAM(void * buffer);
//void* serializarPaqueteDAM(Paquete_DAM * paqueteDAM);
//void* serializarProceso(Proceso * proceso);
void* conexionCPU(void* nuevoCliente);
Proceso* fifo();
Proceso* round_robin();
Proceso* virtual_round_robin();
void *planificadorCortoPlazo(void *miAlgoritmo);
void *ejecutarProceso();
void cargarProcesoEnMemoria();
void finalizarProceso(char *idDT);
void statusProceso(char *idDT);
void mostrarDatosDTBlock(Proceso *procesoStatus);
void mostrarInfoRecursoRetenido(Recurso *recurso);
void statusColasSistema();
void analizarFinEjecucionLineaProceso(int quantum);
bool verificarSiFinalizarElProcesoEnEjecucion(int quantum);
bool existeRecurso(void *recurso);
void liberarRecursos(Proceso *proceso);
void liberarRecurso(Recurso *recursoAliberar);
bool esArchivoBuscado(void *archivo);


#endif /* S_AFA_H_ */
