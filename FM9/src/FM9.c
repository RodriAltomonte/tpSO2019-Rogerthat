#include "FuncionesConexiones.h"
#include "FM9.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <stdio.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>


int cantidadDeCPUs = 0 ;
char string_from_dam[] = {"miguel\nnicolas\njuan\nnicolas\nandrea"}; //hardcodeado porque no tenemos lo que llega del mdj
int TAMANO_MEMORIA = 2048;
int TAMANO_MAXIMO_LINEA = 128;

int proximo_puntero_libre = 0;


void* conexionCPU(void* nuevoCliente){
  int sockCPU = *(int*)nuevoCliente;
  t_config *config=config_create(pathFM9);
  log_info(logger,"Estoy en el hilo del CPU");
  sem_post(&sem_esperaInicializacion);
  int flag ;
  while(1)
  {
//	  Paquete_DAM* paqueteDAM = malloc(sizeof(Paquete_DAM));
	  Paquete_DAM* paqueteDAM = inicializarPaqueteDam();
	  log_info(logger,"Esperando paquete del CPU..");
	  recibirPaqueteDAM(sockCPU,paqueteDAM);
	  log_info(logger,"Recibi un paqueteDAM del CPU..");
	  if(paqueteDAM->tipoOperacion == 7)  //Operacion Close
	  {
		  flag = liberarMemoria(paqueteDAM);
//		  en esta funcion estan los ifs para que libere segun el modo de la memoria usando el dato del paquete
//		  que le sea util (segmento,pagina,etc)
		  send(sockCPU,&flag,sizeof(int),0);
	  }
	  if(paqueteDAM->tipoOperacion == 3)  //Operacion Asignar
	  {
		  flag = asignarModificarArchivoEnMemoria(paqueteDAM); //splitear (*paqueteDAM).archivoAsociado.nombre en path linea datos, el path no se usa, se usa el segmento,pagina,etc.
		  send(sockCPU,&flag,sizeof(int),0);
	  }
		if(paqueteDAM->tipoOperacion == 39)//pido sentencia
		{
			log_info(logger,"Buscando Sentencia para devolversela a CPU");
			int nombreTam =0;
			char* sentencia ;
			char*modo= config_get_string_value(config, "MODO");
//			int tamanioLinea = config_get_int_value(config,"MAX_LINEA");

			if(!strcmp(modo,"SEG"))
			{
				int offset = paqueteDAM->archivoAsociado.programCounter;//numero de linea
				numSegmentoAbuscar = paqueteDAM->archivoAsociado.numeroSegmento;
				SegmentoDeTabla *segmentoBuscado = list_find(lista_segmentos,&segmentoEsIdABuscar);
				if(segmentoBuscado->estaLibre == false && offset < segmentoBuscado->limite){

				int tam =segmentoBuscado->CantBytes;
				sentencia = malloc(tam +1);
				memcpy(sentencia,memoriaReal + segmentoBuscado->base , tam);
				sentencia[tam] = '\0';
				char ** lineas = string_split(sentencia, "\n");
				nombreTam = string_length(lineas[offset]);
				send(sockCPU,&nombreTam,sizeof(int),0);
				if(nombreTam)
				send(sockCPU,lineas[offset],nombreTam,0);
				}

			}
			else if(!strcmp(modo,"TPI")){

			}
			else if(!strcmp(modo,"SPA")){

			}

		}

	  liberarPaqueteDAM(paqueteDAM);
	}
  return 0;
}

void* conexionDAM(void* nuevoCliente){
	int sockDAM = *(int*)nuevoCliente;
	t_config *config=config_create(pathFM9);
	int transferSize = config_get_int_value(config,"TRANSFER_SIZE");//asignar tamaño
	while(1)
	{
//		Paquete_DAM* paqueteDAM = malloc(sizeof(Paquete_DAM));
		Paquete_DAM* paqueteDAM = inicializarPaqueteDam();
		recibirPaqueteDAM(sockDAM,paqueteDAM);
		int partes;
		if(paqueteDAM->tipoOperacion == 1)  //Operacion Abrir
		{
			char* buffer = malloc(transferSize* sizeof(char)+1);
			recv(sockDAM,&partes,sizeof(int),0);
			int numeroSegmento = 0;
			char* nuevoEscriptorio = string_new();//hay q mandar la linea con \0 incluido
			int transferSizeRestante;
			while(partes > 0)
			{
				if(partes == 1)
				{
					recv(sockDAM,&transferSizeRestante,sizeof(int),0);
					char* buffer2 = malloc(transferSizeRestante* sizeof(char)+1);
					recv(sockDAM,buffer2,transferSizeRestante,0);
					buffer2[transferSizeRestante] ='\0';
					string_append(&(nuevoEscriptorio), buffer2);
					free(buffer2);
				}
				else
				{
					recv(sockDAM,buffer,transferSize,0);
					buffer[transferSize] ='\0';
					string_append(&(nuevoEscriptorio), buffer);
				}
/*
				recv(sockDAM,buffer,transferSize,0);
				buffer[transferSize] ='\0';
				string_append(&(nuevoEscriptorio), buffer);
*/
				partes--;
				//cambiar para q funcione en caso de q el tamaño del escriptorio no sea multiplo del transfersize
			}
			free(buffer);
			log_info(logger,"Codigo archivo: %s", nuevoEscriptorio);
			char*modo= config_get_string_value(config, "MODO");
			if(!strcmp(modo,"SEG")){
				log_info(logger,"Agregando segmento...");
				numeroSegmento = agregarSegmento(nuevoEscriptorio,paqueteDAM);

			}
			else if(!strcmp(modo,"TPI"))
			{

			}
			else if(!strcmp(modo,"SPA"))
			{
				numeroSegmento = agregarSegmentoPaginado(nuevoEscriptorio,paqueteDAM);

			}

			if(numeroSegmento == 10002)
			{
				log_info(logger,"No se pudo agregar el segmento");
				paqueteDAM->resultado = 0;
			}
			else
			{
				log_info(logger,"Segmento agregado exitosamente");
				paqueteDAM->resultado = 1;
//					paqueteDAM->archivoAsociado.numeroSegmento = numeroSegmento;
			}
			enviarPaqueteDAM(sockDAM, paqueteDAM);
		}
		if(paqueteDAM->tipoOperacion == 6)  //Operacion Flush
		{
			char* info;
			char*modo= config_get_string_value(config, "MODO");
			if(!strcmp(modo,"SEG"))
			{
				info = obtenerInfoSegmento(paqueteDAM->archivoAsociado.numeroSegmento,paqueteDAM->idProcesoSolicitante);
			}
			else if(!strcmp(modo,"TPI")){

			}
			else if(!strcmp(modo,"SPA")){
				info = obtenerInfoSegmentoPaginado(paqueteDAM->archivoAsociado.numeroSegmento,paqueteDAM->idProcesoSolicitante);
			}
			int partesAenviar = sizeof(info)/transferSize;
			int resto = transferSize ;
			if (sizeof(info)%transferSize != 0)
			{
				resto = sizeof(info)%transferSize;
				partesAenviar++;
				//cambiar para q funcione en caso de q el tamaño del escriptorio no sea multiplo del transfersize
			}
			char *enviar = malloc(sizeof(char)*transferSize);
			int i = 0;
			enviarPaqueteDAM(sockDAM, paqueteDAM);
			send(sockDAM,&partesAenviar,sizeof(int),0);
			while(partes > 0)
			{
				if((partes == 1) && (resto != 0))
				{
					memcpy(enviar, (info+i*transferSize),resto);
				}
				else
				{
					memcpy(enviar, (info+i*transferSize),transferSize);
				}
				send(sockDAM,enviar,transferSize,0);
				partes--;
				i++;
			}
			free(enviar);
		}



		liberarPaqueteDAM(paqueteDAM);
	}
	return 0;
}

void crear_memoria(void) {
  memoriaReal = malloc(sizeof(char*) * TAMANO_MEMORIA);
  proximo_puntero_libre = 0;
}

/**
 * @NAME: Guardar_data_en_memoria
 * @DESC: Funcion "Abrir" de los escriptorios
 * @PARAM: char** data -> con formato ["contenidoLinea1", "contenidoLinea2", "contenidoLinea3", ...]
 */
void guardar_data_en_memoria(char** data,int lineas) {
    int i, j = 0;
    int puntero_memoria = 0;
    char contenido_linea[TAMANO_MAXIMO_LINEA]; //Creo un string auxiliar para ir llenando cada bloque de linea
    memset(contenido_linea, '\0', sizeof(contenido_linea)); //Lo inicializo con \0
    // int cantidad_lineas = sizeof(data)/sizeof(char); //Basofia Humana para saber el tamaño
    //Itero el contenido a grabar
    while( j<lineas) { //check
		strcpy(contenido_linea, data[j]);
		for(i=0; i<strlen(data[j]); i++){
		 memoriaReal[puntero_memoria] = contenido_linea[i];
		 puntero_memoria++;
		}
		proximo_puntero_libre += TAMANO_MAXIMO_LINEA;
		puntero_memoria = proximo_puntero_libre;
		j++;

    }

}

char* copiar(char* palabra) {
  char* tmp = malloc(sizeof(char) * strlen(palabra) + 1);
  memcpy(tmp, palabra, strlen(palabra));
  tmp[strlen(palabra)] = '\0';
  return tmp;
}
int cantidadLineas(char* string){
	int tamanoString=strlen(string);
		int barrasN=0;
		for(int i=0;i<tamanoString;i++){
			if(string[i]=='\n'){
						barrasN++;
					}
		}
		barrasN++;
	return barrasN;
}
char** stringToArray(char* string){
	int tamanoString=strlen(string);
	int barrasN=0;
	for(int i=0;i<tamanoString;i++){
		if(string[i]=='\n'){
			barrasN++;
		}
	}
	barrasN++;
	char **lineas = malloc (sizeof(char *) * (barrasN++));

	int i=0;
	char* palabras=strtok(string,"\n");
	while(palabras!= NULL){
		lineas[i]=(char*)malloc(128*sizeof(char));
		strcpy(lineas[i],palabras);
		palabras=strtok(NULL,"\n");
		i++;
	}
	lineas[i]='\0';

	return lineas;
}

t_config* leerArcConfig(){
    t_config* arcCon;
    char* ruta = malloc(strlen(getenv("FM9.config"))+1);
    strcpy(ruta, getenv("FM9.config"));
    arcCon = config_create(ruta);
    free(ruta);
    return arcCon;
}

tNodoBloques *crearNodoListaAdministrativaSegmentos (int32_t tamanio, int32_t dirReal, int32_t inicioEnTabla, int32_t pID, int32_t idSg){
    tNodoBloques *nodoACrear = malloc(sizeof(tNodoBloques));
    nodoACrear->tamanio = tamanio;
    nodoACrear->inicioEnTabla = inicioEnTabla;
    nodoACrear->pID = pID;
    nodoACrear->idSg = idSg;
//    nodoACrear->dirV = dirV;
    return nodoACrear;
}

void guardarDatosConfigFM9(t_config *arcConfig,char **modoEjecucion, int32_t *tamMemPpal, int32_t *tamMaximoLinea){
    *tamMaximoLinea = config_get_string_value(arcConfig,"TamañoMaximoLinea");
    *tamMemPpal     = config_get_int_value(arcConfig,"TamañoMemoria");
    *modoEjecucion  = config_get_int_value(arcConfig,"ModoEjecucion");
    return;
}

void ver_estado_memoria(){
  int i;
  for(i = 0; i<TAMANO_MEMORIA; i++){
	printf("Printeo Memoria: %c \n", memoriaReal[i]);
  }

}

int main(void) {
//    t_log * logger = log_create("LOG_MEMORIA", "PROCESO FM9", 1, LOG_LEVEL_TRACE);
//    log_trace(logger, "Arranca la Memoria");
//	t_config *arcConfig = leerArcConfig();
//	int32_t retardo;
//	int32_t tamMemPpal;
//	int32_t tamMaximoLinea;
//	guardarDatosConfigFM9(arcConfig, &modoEjecucion,&tamMemPpal,&tamMaximoLinea);
//	crear_memoria();

	logger = log_create(logFM9,"FM9",1, LOG_LEVEL_INFO);
	t_config *config=config_create(pathFM9);
	tamanioMemoria = config_get_int_value(config, "TAMANIO");
	int tamMaxPagina = config_get_int_value(config, "TAM_PAGINA");
	memoriaReal = malloc(tamanioMemoria);	//STORAGE O MEMORIAREAL
    char* modoEjecucion;
    sem_init(&sem_esperaInicializacion,0,0);
    lista_procesos = list_create();
	char*modo= config_get_string_value(config, "MODO");
	if(!strcmp(modo,"SEG")){
		lista_segmentos = list_create();
//		direccionBaseProxima = memoriaReal; //le asigno la primera posicion de memoria de la MP

		idGlobal = 0;
	}
	else if(!strcmp(modo,"TPI")){

	}
	else if(!strcmp(modo,"SPA")){
		idMarcoGlobal = 0;
		cantidadMaximaMarcos = tamanioMemoria/tamMaxPagina;
		lista_marcos = list_create();
	}
	//crear hilo consola FM9

    int sockDAM=crearConexionCliente(pathDAM);
      if(sockDAM<0){
    	  log_error(logger,"Error en la conexion con DAM");
    	  return 1;
      }
      log_info(logger,"Se realizo correctamente la conexion DAM");
      send(sockDAM,"3",2,0);
      //hiloDAM
      pthread_create(&hiloDAM,NULL,conexionDAM,(void*)&sockDAM);
      int sockFM9 = crearConexionServidor(pathFM9);
      if (listen(sockFM9, 10) == -1){ //ESCUCHA!
        log_error(logger, "No se pudo escuchar");
        log_destroy(logger);
        return -1;
      }
      pthread_t* hilosCPUs = NULL;
      fcntl(sockFM9, F_SETFL, O_NONBLOCK);  //para hacer noBlockeante el sockFM9
      log_info(logger, "Se esta escuchando");
      char* buff;
      int sockCPU;
    while(sockCPU = accept(sockFM9, NULL, NULL)){
      if (sockCPU == -1) {
          if (errno == EWOULDBLOCK) {
//					log_info(logger,"No pending connections; sleeping for one second.\n");
//					sleep(1);
          } else {
          log_info(logger,"error when accepting connection");
          exit(1);
          }
        } else {
          buff = malloc(2);
//          recv(sockFM9,buff,2,0); porq dice sockFM9?
          recv(sockCPU,buff,2,0);
          log_info(logger, "Llego una nueva conexion");
          log_info(logger,"Se realizo correctamente la conexion con CPU");
          hilosCPUs = realloc(hilosCPUs,sizeof(pthread_t)*(cantidadDeCPUs+1));
          pthread_create(&(hilosCPUs[cantidadDeCPUs]),NULL,conexionCPU,(void*)&sockCPU);
          cantidadDeCPUs++;
          if(buff[0] == '1'){
            log_info(logger,"Se realizo correctamente la conexion con CPU");
          }
          free(buff);
          sem_wait(&sem_esperaInicializacion);
        }
    }
      sleep(1);
      close(sockDAM);
      close(sockCPU);
      close(sockFM9);

  return EXIT_SUCCESS;
}


int agregarSegmento(char *nuevoEscriptorio,Paquete_DAM* paqueteDAM)
{
	int idProcesoSolicitante = paqueteDAM->idProcesoSolicitante;
//	SegmentoDeTabla *nuevoSegmento = malloc(sizeof(SegmentoDeTabla));
//	(*nuevoSegmento).estaLibre = false;
	int numSegmentoNuevo;
	char ** lineas = string_split(nuevoEscriptorio, "\n");
	int i =0;
	while(lineas[i++] != NULL); --i; //i tiene la cantidad de lineas
	int resultadoOperacion = dameProximaDireccionBaseDisponible(string_length(nuevoEscriptorio),&numSegmentoNuevo,i);
	if (resultadoOperacion == -1){
		log_info(logger,"Espacio insuficiente en FM9");
		return 10002;
	}
	paqueteDAM->archivoAsociado.numeroSegmento = numSegmentoNuevo; //direccion segmento
	SegmentoDelProceso *nuevoSegmentoDelProceso = malloc(sizeof(SegmentoDelProceso));
	(*nuevoSegmentoDelProceso).base = resultadoOperacion; //direccion base posta
	memcpy(memoriaReal + (*nuevoSegmentoDelProceso).base, nuevoEscriptorio,string_length(nuevoEscriptorio)); //guardo el escriptorio en la memoria real en forma de bits sin \0
//	(*nuevoSegmentoDelProceso).limite = string_length(nuevoEscriptorio)+1; //el limite es la cantidad maximas de lineas que tiene
	(*nuevoSegmentoDelProceso).limite = i; //el limite es excluyente
	(*nuevoSegmentoDelProceso).idProceso = idProcesoSolicitante;
	(*nuevoSegmentoDelProceso).numeroSegmento = numSegmentoNuevo;
	TablaSegmentosDeProceso *unaTablaSegmentosDeProceso;
	idTablaBuscar = idProcesoSolicitante;
	unaTablaSegmentosDeProceso = list_find(lista_procesos,&esProcesoAbuscar);
	if(unaTablaSegmentosDeProceso == NULL) //el proceso es nuevo, no tenia ningun segmento asociado
	{
		TablaSegmentosDeProceso *nuevaTablaSegmentosDeProceso = malloc (sizeof(TablaSegmentosDeProceso));
		(*nuevaTablaSegmentosDeProceso).idProceso = idProcesoSolicitante;
		(*nuevaTablaSegmentosDeProceso).segmentosDelProceso = list_create();
		list_add((*nuevaTablaSegmentosDeProceso).segmentosDelProceso,nuevoSegmentoDelProceso);
	}
	else
	{
		list_add((*unaTablaSegmentosDeProceso).segmentosDelProceso,nuevoSegmentoDelProceso);
	}
	return 1;
}

int dameProximaDireccionBaseDisponible(int cantBytes, int *numSegmentoNuevo, int lineas){
	tamanioArchivoAMeter = cantBytes;
	direccionBaseAusar = 0;
	int baseSegmento;
	SegmentoDeTabla *unSegmento = malloc (sizeof(SegmentoDeTabla));
	unSegmento = list_find(lista_segmentos,&entraArchivoEnSegmento);
	if(unSegmento == NULL)
	{
		if(direccionBaseAusar + cantBytes <= tamanioMemoria)
		{
			baseSegmento = direccionBaseAusar;
			SegmentoDeTabla *segmentoNuevoEntero = malloc (sizeof(SegmentoDeTabla));
			idGlobal++;
			(*segmentoNuevoEntero).base = baseSegmento;
			(*segmentoNuevoEntero).limite = lineas;
			(*segmentoNuevoEntero).estaLibre = false;
			(*segmentoNuevoEntero).CantBytes = cantBytes;
			(*segmentoNuevoEntero).numeroSegmento = idGlobal;
			list_add(lista_segmentos,segmentoNuevoEntero);
			(*numSegmentoNuevo) = (*segmentoNuevoEntero).numeroSegmento;
		}
		else  //no entra
		{

			return -1;
		}
	}
	else
	{
		numSegmentoAbuscar = (*unSegmento).numeroSegmento;
		baseSegmento = (*unSegmento).base;
		if(cantBytes != (*unSegmento).CantBytes)
		{
			SegmentoDeTabla *segmentoNuevo = malloc (sizeof(SegmentoDeTabla));
			idGlobal++;
			(*segmentoNuevo).base = baseSegmento;
			(*segmentoNuevo).limite = lineas;
			(*segmentoNuevo).estaLibre = false;
			(*segmentoNuevo).CantBytes = cantBytes;
			(*segmentoNuevo).numeroSegmento = idGlobal;
			list_add(lista_segmentos,segmentoNuevo);
			(*numSegmentoNuevo) = (*segmentoNuevo).numeroSegmento;
			SegmentoDeTabla *segmentoNuevo2 = malloc (sizeof(SegmentoDeTabla));
			idGlobal++;
			(*segmentoNuevo2).base = (*segmentoNuevo).base + (*segmentoNuevo).CantBytes ;
			(*segmentoNuevo2).limite = 0;
			(*segmentoNuevo).CantBytes = (*unSegmento).CantBytes - cantBytes; //cantidad de Bytes disponibles
			(*segmentoNuevo2).estaLibre = true ;
			(*segmentoNuevo2).numeroSegmento = idGlobal;
			list_add(lista_segmentos,segmentoNuevo2);
			list_remove_by_condition(lista_segmentos,&segmentoEsIdABuscar);
		}
	}
	return baseSegmento;  //no importa el numero
}

bool entraArchivoEnSegmento(void *segmentoDeTabla){
	SegmentoDeTabla *b=(SegmentoDeTabla*) segmentoDeTabla;
	direccionBaseAusar = (*b).base ;
	if(((*b).CantBytes >= tamanioArchivoAMeter) && ((*b).estaLibre == true)){
		return true;
	}
	else
		direccionBaseAusar = (*b).base + (*b).CantBytes ;
		return false;
}

bool segmentoEsIdABuscar(void * segmento){
	SegmentoDeTabla *seg=(SegmentoDeTabla*) segmento;
	if((*seg).numeroSegmento==numSegmentoAbuscar)
		return true;
	else
		return false;
}

bool segmentoPaginadoEsIdABuscar(void * segmento){
	SegmentoPaginadoDelProceso *seg=(SegmentoPaginadoDelProceso*) segmento;
	if((*seg).numeroSegmento==numSegmentoAbuscar)
		return true;
	else
		return false;
}


bool esProcesoAbuscar(void * proceso){

	TablaSegmentosDeProceso * tabla = (TablaSegmentosDeProceso *) proceso;
	if((*tabla).idProceso==idTablaBuscar)
		return true;
	else
		return false;
}

int liberarMemoria(Paquete_DAM* paqueteDAM)
{
	t_config *config=config_create(pathFM9);
	char*modo= config_get_string_value(config, "MODO");
	TablaSegmentosDeProceso* unaTablaSegmentosDeProceso;
	if(!strcmp(modo,"SEG"))
	{
		idTablaBuscar = paqueteDAM->idProcesoSolicitante;
		unaTablaSegmentosDeProceso = list_find(lista_procesos,&esProcesoAbuscar);
		numSegmentoAbuscar = paqueteDAM->archivoAsociado.numeroSegmento;
		list_remove_by_condition(unaTablaSegmentosDeProceso->segmentosDelProceso,&segmentoEsIdABuscar);
		SegmentoDeTabla *segmentoBuscado = list_find(lista_segmentos,&segmentoEsIdABuscar);
		(*segmentoBuscado).estaLibre = true;
		(*segmentoBuscado).limite =1;
		if(segmentoBuscado == NULL){ //estaba cerrado
			return 40001;
		}
		//falta ver cuando nos puede tirar 40002: Fallo de segmento/memoria.
		return 1;

	}
	else if(!strcmp(modo,"TPI"))
	{

	}
	else if(!strcmp(modo,"SPA"))
	{
		idTablaBuscar = paqueteDAM->idProcesoSolicitante;
		unaTablaSegmentosDeProceso = list_find(lista_procesos,&esProcesoAbuscar);
		numSegmentoPaginadoAbuscar = paqueteDAM->archivoAsociado.numeroSegmento;
		SegmentoPaginadoDelProceso* segmentoPaginado = list_find(unaTablaSegmentosDeProceso->segmentosDelProceso,&segmentoPaginadoEsIdABuscar);
		list_iterate(segmentoPaginado->paginas, &liberarMarcoAsociado);
		list_remove_by_condition(unaTablaSegmentosDeProceso->segmentosDelProceso,&segmentoPaginadoEsIdABuscar);
		//finalmente borras el segmento que tenia referencia a ese archivo
	}
	return 1;
}

void liberarMarcoAsociado(void * pagina){
	Pagina* paginaSegmento = (Pagina*) pagina;
	Marco* marco = list_get(lista_marcos,paginaSegmento->numeroMarco);
	(*marco).estaLibre = true;
}

int asignarModificarArchivoEnMemoria(Paquete_DAM* paqueteDAM){ //splitear (*paqueteDAM).archivoAsociado.nombre en path linea datos, el path no se usa, se usa el segmento,pagina,etc.

	t_config *config=config_create(pathFM9);
	char *nombraArchivoAescribir = string_new();
	char *lineaAescribir = string_new();
	char* datoAescribir = string_new();
	char *sentencia= string_new();
	string_append(&sentencia, paqueteDAM->archivoAsociado.nombre);
	string_trim(&sentencia);
	char** datos = string_n_split(sentencia, 3, " " );
	string_append(&nombraArchivoAescribir, datos[0]);
	string_append(&lineaAescribir, datos[1]);
	string_append(&datoAescribir, datos[2]);
	int numLineaACambiar = atoi(lineaAescribir);


	char*modo= config_get_string_value(config, "MODO");
	if(!strcmp(modo,"SEG"))
	{
		idTablaBuscar = paqueteDAM->idProcesoSolicitante;
		TablaSegmentosDeProceso* unaTablaSegmentosDeProceso = list_find(lista_procesos,&esProcesoAbuscar);
		numSegmentoAbuscar = paqueteDAM->archivoAsociado.numeroSegmento;
		list_remove_by_condition(unaTablaSegmentosDeProceso->segmentosDelProceso,&segmentoEsIdABuscar);
		SegmentoDeTabla *segmentoBuscado = list_find(lista_segmentos,&segmentoEsIdABuscar);
		char* viejoEscriptorio = malloc((*segmentoBuscado).CantBytes+1);
		memcpy(viejoEscriptorio,memoriaReal + (*segmentoBuscado).base,(*segmentoBuscado).CantBytes);
		viejoEscriptorio[(*segmentoBuscado).CantBytes] = '\0';
		char ** lineas = string_split(viejoEscriptorio, "\n");
		int i =0;
		char* nuevoEscriptorio = string_new();
		while(lineas[i]!=NULL){

			if(numLineaACambiar != i){
				string_append(&nuevoEscriptorio, lineas[i]);
			}else
			{
				string_append(&nuevoEscriptorio, datoAescribir);
			}

			string_append(&nuevoEscriptorio,"\n");
			i++;
		}

		segmentoBuscado->estaLibre =true;
		int flag = agregarSegmento(nuevoEscriptorio,paqueteDAM);
		return flag;

//		int tamanioLinea = config_get_int_value(config,"MAX_LINEA");

//		memcpy(memoriaReal + (*segmentoBuscado).base + paqueteDAM->archivoAsociado.programCounter,datoAescribir ,tamanioLinea );


	}
	else if(!strcmp(modo,"SPA"))
	{
		idTablaBuscar = paqueteDAM->idProcesoSolicitante;
		TablaSegmentosDeProceso* unaTablaSegmentosDeProceso = list_find(lista_procesos,&esProcesoAbuscar);
		numSegmentoPaginadoAbuscar = paqueteDAM->archivoAsociado.numeroSegmento;
		SegmentoPaginadoDelProceso *segmentoPaginadoBuscado = list_find((*unaTablaSegmentosDeProceso).segmentosDelProceso,&segmentoEsIdABuscar);
		int maxLinea = config_get_int_value(config, "MAX_LINEA");
		int posicionDato = numLineaACambiar * maxLinea;
		int paginaDelDato = posicionDato/config_get_int_value(config, "TAM_PAGINA");
		Pagina *pagina = list_get((*segmentoPaginadoBuscado).paginas,paginaDelDato);
		Marco *marco = list_get(lista_marcos,(*pagina).numeroMarco);
		memcpy(memoriaReal + (*marco).direccionBase,datoAescribir ,config_get_int_value(config, "MAX_LINEA")); //guardo el escriptorio en la memoria real
	}
	else if(!strcmp(modo,"TPI"))
	{

	}

	return 1 ;
}

char* obtenerInfoSegmento(int numeroSegmento,int idProcesoSolicitante)
{
	idTablaBuscar = idProcesoSolicitante;
	TablaSegmentosDeProceso* unaTablaSegmentosDeProceso = list_find(lista_procesos,&esProcesoAbuscar);
	numSegmentoAbuscar = numeroSegmento;
	list_remove_by_condition(unaTablaSegmentosDeProceso->segmentosDelProceso,&segmentoEsIdABuscar);
	SegmentoDeTabla *segmentoBuscado = list_find(lista_segmentos,&segmentoEsIdABuscar);
	char *infoArchivo;
	memcpy(infoArchivo,memoriaReal + (*segmentoBuscado).base + (*segmentoBuscado).limite,(*segmentoBuscado).limite);
	return infoArchivo;
}

char* obtenerInfoSegmentoPaginado(int numeroSegmento,int idProcesoSolicitante)
{
	char *infoArchivo;
	idTablaBuscar = idProcesoSolicitante;
	TablaSegmentosDeProceso* unaTablaSegmentosDeProceso = list_find(lista_procesos,&esProcesoAbuscar);
	idTablaBuscar = idProcesoSolicitante;
	unaTablaSegmentosDeProceso = list_find(lista_procesos,&esProcesoAbuscar);
	numSegmentoPaginadoAbuscar = numeroSegmento;
	SegmentoPaginadoDelProceso* segmentoPaginado = list_find(unaTablaSegmentosDeProceso->segmentosDelProceso,&segmentoPaginadoEsIdABuscar);
	list_iterate(segmentoPaginado->paginas, &extraerInfoMarcoAsociado);
	strcpy(infoArchivo,infoArchivoFlush);
	strcpy(infoArchivoFlush,""); //resetear el infoFLush (variable global), revisar si es asi o con memset
	return infoArchivo;
}

void extraerInfoMarcoAsociado(void * pagina){
	Pagina* paginaSegmento = (Pagina*) pagina;
	Marco* marco = list_get(lista_marcos,paginaSegmento->numeroMarco);
	t_config *config=config_create(pathFM9);
	int tamanioMaxPagina = config_get_int_value(config,"TAM_PAGINA");
	char *aux;
	memcpy(aux,memoriaReal + (*marco).direccionBase,tamanioMaxPagina);
	strcat(infoArchivoFlush,aux); //vas concatenando los datos del archivo
	config_destroy(config);
}

int agregarSegmentoPaginado(char *nuevoEscriptorio,Paquete_DAM * paquete)
{
	int resultadoOperacion;
	int idProcesoSolicitante = paquete->idProcesoSolicitante;
	idTablaBuscar = idProcesoSolicitante;
	TablaSegmentosDeProceso* unProceso = list_find(lista_procesos,&esProcesoAbuscar);
	if(unProceso == NULL)
	{
		TablaSegmentosDeProceso *procesoNuevo = malloc(sizeof(TablaSegmentosDeProceso));
		(*procesoNuevo).segmentosDelProceso = list_create();
		(*procesoNuevo).idProceso = idProcesoSolicitante;
		SegmentoPaginadoDelProceso *nuevoSegmentoPaginadoDelProceso = malloc(sizeof(SegmentoPaginadoDelProceso));
		resultadoOperacion = paginarSegmento(nuevoSegmentoPaginadoDelProceso,nuevoEscriptorio);
		list_add((*procesoNuevo).segmentosDelProceso,nuevoSegmentoPaginadoDelProceso);
	}
	else
	{
		SegmentoPaginadoDelProceso *nuevoSegmentoPaginadoDelProceso2 = malloc(sizeof(SegmentoPaginadoDelProceso));
		resultadoOperacion = paginarSegmento(nuevoSegmentoPaginadoDelProceso2,nuevoEscriptorio);
		list_add((*unProceso).segmentosDelProceso,nuevoSegmentoPaginadoDelProceso2);
	}

	return resultadoOperacion;

}

int paginarSegmento(SegmentoPaginadoDelProceso *nuevoSegmentoPaginadoDelProceso,char *nuevoEscriptorio){
	t_config *config=config_create(pathFM9);
	int tamanioMaxPagina = config_get_int_value(config,"TAM_PAGINA");
	int cantidadPaginas = string_length(nuevoEscriptorio)/tamanioMaxPagina;
	int flag;
	if((string_length(nuevoEscriptorio)%tamanioMaxPagina) != 0)
	{
		cantidadPaginas++;
	}
	for (int i = 0; i < cantidadPaginas; i++)
	{
		 Pagina* nuevaPagina = malloc(sizeof(Pagina));
		 nuevaPagina->numeroPagina = i;
		 nuevaPagina->numeroMarco = idMarcoGlobal;
		 flag = agregarMarcoAtablaGeneral(nuevaPagina,nuevoEscriptorio,i);
		 if (flag == -1) //error , memoria insuficiente
		 {
			 return 20003;
		 }
		 list_add(nuevoSegmentoPaginadoDelProceso->paginas,nuevaPagina);
	}
	config_destroy(config);
	return flag;
}

int agregarMarcoAtablaGeneral (Pagina* nuevaPagina,char *nuevoEscriptorio,int posicionEnArchivo)
{
	t_config *config=config_create(pathFM9);
	int tamanioMaxPagina = config_get_int_value(config,"TAM_PAGINA");
	Marco* unMarco = list_find(lista_marcos,&marcoEstaLibre);
	if(unMarco == NULL)
	{
		if(cantidadMaximaMarcos < idMarcoGlobal)
		{
			Marco* nuevoMarco = malloc(sizeof(Marco));
			(*nuevoMarco).numeroMarco = (*nuevaPagina).numeroMarco;
			(*nuevoMarco).direccionBase = (*nuevoMarco).numeroMarco * tamanioMaxPagina;
			(*nuevoMarco).estaLibre = false;
			list_add(lista_marcos,nuevoMarco);
			idMarcoGlobal++;
			guardarInfoEnMemoria((*nuevoMarco).direccionBase,nuevoEscriptorio,posicionEnArchivo);
		}
		else
		{
			return -1 ; //no hay mas memoria;
		}
	}
	else
	{
		(*unMarco).estaLibre = false;
		guardarInfoEnMemoria((*unMarco).direccionBase,nuevoEscriptorio,posicionEnArchivo);
	}
	config_destroy(config);
	return 1 ; //success
}

bool marcoEstaLibre(void *marcoDeTabla){
	Marco *b=(Marco*) marcoDeTabla;
	if((*b).estaLibre == true){
		return true;
	}
	else{
		return false;
	}
}

void guardarInfoEnMemoria(int direccionBaseMarco,char *nuevoEscriptorio, int posicionEnArchivo)
{
	t_config *config=config_create(pathFM9);
	int tamanioMaxPagina = config_get_int_value(config,"TAM_PAGINA");
	memcpy(memoriaReal + direccionBaseMarco, nuevoEscriptorio + posicionEnArchivo*tamanioMaxPagina,tamanioMaxPagina); //guardo el escriptorio en la memoria real
	config_destroy(config);
}




