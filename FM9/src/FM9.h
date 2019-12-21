/*
 * FM9.h
 *
 *  Created on: 24/10/2018
 *      Author: utnso
 */

#ifndef FM9_H_
#define FM9_H_
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


typedef struct {
	int32_t tamanio;
	int32_t dirReal;
	int32_t inicioEnTabla;
	int32_t pID;
	int32_t idSg;
//	t_puntero dirV;
} tNodoBloques;

typedef struct {
	int idProceso;
	int numeroSegmento;
	int base;
	int limite;
} SegmentoDelProceso;

typedef struct {
	int idProceso;
	int numeroSegmento;
	t_list *paginas;
} SegmentoPaginadoDelProceso;

typedef struct {
	int numeroPagina;
	int numeroMarco;
} Pagina ;

typedef struct {
	int numeroMarco;
	int direccionBase;
	bool estaLibre;
} Marco ;

typedef struct {
	int numeroSegmento; //direccion logica base de la posicion dentro de la memoria
	int base; //direccion real de la posicion dentro de la memoria e incluye
	int limite; //el limite es la cantidad maximas de lineas que tiene es excluyente el valor
	int CantBytes; // tamaño del archivo en Bytes
	bool estaLibre;
} SegmentoDeTabla;

typedef struct {
	int idProceso;
	t_list *segmentosDelProceso;
} TablaSegmentosDeProceso;

t_list *lista_segmentos;
t_list *lista_procesos;
t_list *lista_marcos;

pthread_t hiloDAM;
sem_t sem_esperaInicializacion;
int tamanioMemoria;
char* memoriaReal;
int direccionBaseProxima ;
int direccionBaseAusar ; //direccion real de la posicion dentro de la memoria
int tamanioArchivoAMeter;
int numSegmentoAbuscar;
int idGlobal;
int idTablaBuscar;
int idProcesoAbuscar;
int idMarcoGlobal;
int cantidadMaximaMarcos;
int numSegmentoPaginadoAbuscar;
char* infoArchivoFlush;

tNodoBloques *crearNodoListaAdministrativaSegmentos (int32_t tamanio, int32_t dirReal, int32_t inicioEnTabla, int32_t pID, int32_t idSg);
void* conexionCPU(void* nuevoCliente);
void* conexionDAM(void* nuevoCliente);
int agregarSegmento(char *nuevoEscriptorio,Paquete_DAM* paqueteDAM);
char* obtenerInfoSegmento(int numeroSegmento,int idProcesoSolicitante);
char* obtenerInfoSegmentoPaginado(int numeroSegmento,int idProcesoSolicitante);
int asignarModificarArchivoEnMemoria(Paquete_DAM* paqueteDAM);
int liberarMemoria(Paquete_DAM* paqueteDAM);
bool esProcesoAbuscar(void * proceso);
bool segmentoEsIdABuscar(void * segmento);
bool entraArchivoEnSegmento(void *segmentoDeTabla);
int dameProximaDireccionBaseDisponible(int tamanio, int *numSegmentoNuevo,int i);
int agregarSegmentoPaginado(char *nuevoEscriptorio,Paquete_DAM * paquete);
bool segmentoPaginadoEsIdABuscar(void * segmento);
void liberarMarcoAsociado(void * pagina);
void extraerInfoMarcoAsociado(void * pagina);
bool marcoEstaLibre(void *marcoDeTabla);
#endif /* FM9_H_ */

