/*
 * FuncionesConexiones.c
 *
 *  Created on: 2 sep. 2018
 *      Author: utnso
 */

#include "FuncionesConexiones.h"

const char* ESI = "e";
const char* INSTANCIA = "i";
const int SET=0;
const int GET=1;
const int STORE=2;

//Path de los servidores
const char *pathSAFA="/home/utnso/workspace/tp-2018-2c-RogerThat/Config/S-AFA.cfg";
const char *pathDAM="/home/utnso/workspace/tp-2018-2c-RogerThat/Config/DAM.cfg";
const char *pathMDJ="/home/utnso/workspace/tp-2018-2c-RogerThat/Config/MDJ.cfg";
const char *pathFM9="/home/utnso/workspace/tp-2018-2c-RogerThat/Config/FM9.cfg";
const char *pathCPU="/home/utnso/workspace/tp-2018-2c-RogerThat/Config/CPU.cfg";

//Los pongo en escritorio para que no tengamos problemas al commitear
const char *logSAFA="/home/utnso/Escritorio/S-AFA.log";
const char *logDAM="/home/utnso/Escritorio/DAM.log";
const char *logMDJ="/home/utnso/Escritorio/MDJ.log";
const char *logFM9="/home/utnso/Escritorio/FM9.log";
const char *logCPU="/home/utnso/Escritorio/CPU.log";

struct sockaddr_in dameUnaDireccion(char *path,int ipAutomatica){
	t_config *config=config_create(path);
	struct sockaddr_in midireccion;
	midireccion.sin_family = AF_INET;
	midireccion.sin_port = htons(config_get_int_value(config, "Puerto"));
	if(ipAutomatica){
		midireccion.sin_addr.s_addr = INADDR_ANY;
	}
	else{
			midireccion.sin_addr.s_addr = inet_addr(config_get_string_value(config,"Ip"));
	}
	config_destroy(config);
	return midireccion;
}
// estas Funciones retornan -1 en caso de error hay que ver despues como manejarlas
int crearConexionCliente(char*path){//retorna el descriptor de fichero
	int sock=socket(AF_INET, SOCK_STREAM, 0);
	if(sock<0){
	return -1;// CASO DE ERROR
	}
	struct sockaddr_in midireccion=dameUnaDireccion(path,0);
	memset(&midireccion.sin_zero, '\0', 8);
	if(connect(sock, (struct sockaddr *)&midireccion, sizeof(struct sockaddr))<0){
		return -1;
	}
	return sock;
}
int crearConexionServidor(char*path){//Retorna el sock del Servidor
	int sockYoServidor=socket(AF_INET, SOCK_STREAM, 0);// no lo retorno me sierve para la creacion del servidor
	if(sockYoServidor<0){
		printf("Error, no se pudo crear el socket\n");
		return -1;// CASO DE ERROR
	}
	struct sockaddr_in miDireccion=dameUnaDireccion(path,0);
	memset(&miDireccion.sin_zero, '\0', 8);
	//struct sockaddr_in their_addr;//Aca queda almacenada la informacion del cliente
	int activado = 1;
    if(setsockopt(sockYoServidor, SOL_SOCKET,SO_REUSEADDR,&activado,sizeof(activado)) == -1){
    	printf("Error en la funcion setsockopt");
    	return -1;
    };
    if((bind(sockYoServidor, (struct sockaddr *)&miDireccion, sizeof(struct sockaddr)))<0){
    	printf("Error, no se pudo hacer el bind al puerto\n");
    	return -1;
    }
    return sockYoServidor;
}

int ejemplo(){
	return 23;
}


int enviarDatosArchivo(int socketProcesoEnviar, DatosArchivo * archivo){

	void *buffer;
	int total, resultado,nombreTam =0 ,direccionContenidoTam=0;


	if(!string_is_empty(archivo->direccionContenido))
		direccionContenidoTam = string_length(archivo->direccionContenido);

	if(!string_is_empty(archivo->nombre))
		nombreTam = string_length(archivo->nombre);

	buffer = malloc(total= (5*4+nombreTam+direccionContenidoTam)); //3*sizeof(int)

	memcpy(buffer, &(archivo->numeroSegmento),4 );
	memcpy(buffer + 4, &(archivo->numeroPagina),4);
	memcpy(buffer + 2* 4, &(archivo->programCounter),4) ;
	memcpy(buffer + 3*4,  &(nombreTam),4) ;
	memcpy(buffer + 4*4 ,&(direccionContenidoTam),4) ;
	memcpy(buffer+ 5*4, archivo->nombre, nombreTam);
	memcpy(buffer+ 5*4 + nombreTam, archivo->direccionContenido,direccionContenidoTam);


	resultado = send(socketProcesoEnviar,buffer,total,0);

	free(buffer);

	return resultado;

}

int recibirDatosArchivo(int socketSender, DatosArchivo * archivo){

	void* buffer = malloc(5*4);// es el tamaño de DatosArchivo +2 int 5* sizeof(int)
		int flag = recv(socketSender,buffer,20,0);

		if(flag != -1){

			int nombreTam;
			int direccionContenidoTam;

			memcpy(&(archivo->numeroSegmento), buffer, 4 );
			memcpy(&(archivo->numeroPagina),buffer + 4, 4 );
			memcpy(&(archivo->programCounter),buffer + 2*4, 4 );
			memcpy(&(nombreTam),buffer + 3*4, 4 );
			memcpy(&(direccionContenidoTam),buffer + 4 * 4 , 4 );

			printf("receive memcpy ");

			if(nombreTam){

				char * buffer2 = malloc(sizeof(char)*(nombreTam+1));
				recv(socketSender,buffer2,nombreTam,0);
				buffer2[nombreTam] = '\0';
				archivo->nombre = string_new();
				string_append(&(archivo->nombre), buffer2);
				free(buffer2);

				printf("receive q anda ");
			}

			if(direccionContenidoTam){

				char * buffer2 = malloc(sizeof(char)*(direccionContenidoTam+1));
				recv(socketSender,buffer2,direccionContenidoTam,0);
				buffer2[direccionContenidoTam] = '\0';
				archivo->direccionContenido = string_new();
				string_append(&(archivo->direccionContenido), buffer2);
				free(buffer2);

				printf("receive ");
			}


		}

		free(buffer);

		return flag;

}



int enviarProceso(int socketProcesoEnviar, Proceso * proceso){

//	no manda lista --- ahora si manda archivos asociados
	void *buffer;
	int total, resultado, pathEscriptorioTam = 0;
	int cantArchivos =0;
//	log_info(logger,"dentro de enviarProceso ");
//	log_info(logger,"con proceso %d",(*proceso).idProceso);
//	nunca va a ser 0
	pathEscriptorioTam = string_length(proceso->pathEscriptorio);// no mando \0

	if(!list_is_empty(proceso->archivosAsociados))
		cantArchivos = list_size(proceso->archivosAsociados);

	buffer = malloc(total= (7*4 + pathEscriptorioTam));//le agrego uno mas por cant de lista archivo

	memcpy(buffer, &(proceso->idProceso),4 );
	memcpy(buffer + 4   ,&(proceso->socketProceso),4);
	memcpy(buffer + 2*4 ,&(pathEscriptorioTam),4) ;
	memcpy(buffer + 3*4 ,&(proceso->flagInicializacion),4) ;
	memcpy(buffer + 4*4 ,&(proceso->estado),4) ;
	memcpy(buffer + 5*4 ,&(proceso->seEstaCargando),4);
	memcpy(buffer + 6*4 ,&(cantArchivos),4); //agrego cantidad
	memcpy(buffer + 7*4 , proceso->pathEscriptorio,pathEscriptorioTam) ;

	resultado = send(socketProcesoEnviar,buffer,total,0);

	int counter = 0;
	while(counter < cantArchivos)
	{
		DatosArchivo* unDato = list_get(proceso->archivosAsociados,counter); //preguntar si list_get empieza en 0
		resultado = enviarDatosArchivo(socketProcesoEnviar, unDato);
		counter++;
	}

	free(buffer);

	return resultado;

}

//nota la variable proceso tiene que estar en default
int recibirProceso(int socketSender, Proceso * proceso){

	void* buffer = malloc(7*4);
	int flag = recv(socketSender,buffer,28,0);

	if(flag != -1){

		int pathEscriptorioTam = 0;
		int cantArchivos =0;

		memcpy(&(proceso->idProceso), buffer ,4 );
		memcpy(&(proceso->socketProceso), buffer + 4, 4 );
		memcpy(&(pathEscriptorioTam), buffer + 2*4 ,4 );
		memcpy(&(proceso->flagInicializacion), buffer + 3*4 ,4 );
		memcpy(&(proceso->estado), buffer + 4*4 ,4 );
		memcpy(&(proceso->seEstaCargando), buffer + 5*4 ,4 );
		memcpy(&(cantArchivos), buffer + 6*4 ,4 );


		if(pathEscriptorioTam){

			char* buffer2 = malloc(sizeof(char)* (pathEscriptorioTam + 1));
			recv(socketSender,buffer2,pathEscriptorioTam,0);
			buffer2[pathEscriptorioTam] = '\0';
			proceso->pathEscriptorio = string_new();
			string_append(&(proceso->pathEscriptorio), buffer2);
			free(buffer2);

		}

		proceso->archivosAsociados = list_create();
		proceso->recursosRetenidos = list_create();

		while(cantArchivos){

			DatosArchivo * nuevoArchi = malloc(sizeof(DatosArchivo));
			flag = recibirDatosArchivo(socketSender, nuevoArchi);

			if(flag != -1){
				list_add(proceso->archivosAsociados,nuevoArchi);

			}else
				return flag;

			cantArchivos--;

		}





	}

	free(buffer);

	return flag;

}

int enviarPaqueteDAM(int socketProcesoEnviar, Paquete_DAM * paqueteDAM){


	void *buffer;
	int total, resultado,nombreTam =0 ,direccionContenidoTam=0;


	if(!string_is_empty(paqueteDAM->archivoAsociado.direccionContenido))
		direccionContenidoTam = string_length(paqueteDAM->archivoAsociado.direccionContenido);

	if(!string_is_empty(paqueteDAM->archivoAsociado.nombre))
		nombreTam = string_length(paqueteDAM->archivoAsociado.nombre);

	buffer = malloc(total= (32+nombreTam+direccionContenidoTam)); //32 ya que 8*sizeof(int)

	memcpy(buffer, &(paqueteDAM->tipoOperacion),4 );
	memcpy(buffer + 4, &(paqueteDAM->resultado),4);
	memcpy(buffer + 2* 4, &(paqueteDAM->idProcesoSolicitante),4) ;
	memcpy(buffer + 3*4,  &(nombreTam),4) ;
	memcpy(buffer + 4*4 ,&(direccionContenidoTam),4) ;
	memcpy(buffer+ 5*4, &(paqueteDAM->archivoAsociado.numeroPagina),4 );
	memcpy(buffer+ 6*4, &(paqueteDAM->archivoAsociado.numeroSegmento),4 );
	memcpy(buffer+ 7*4, &(paqueteDAM->archivoAsociado.programCounter),4 );
	memcpy(buffer + 8*4 , paqueteDAM->archivoAsociado.nombre,nombreTam) ;
	memcpy(buffer+ 8*4 + nombreTam, paqueteDAM->archivoAsociado.direccionContenido,direccionContenidoTam);

	resultado = send(socketProcesoEnviar,buffer,total,0);

	free(buffer);

	return resultado;

}

int recibirPaqueteDAM(int socketSender, Paquete_DAM * paqueteDAM){

	void* buffer = malloc(32);// es el tamaño de Paquete_DAM sin los char* =>8*4 32
	int flag = recv(socketSender,buffer,32,0);

	if(flag != -1){

		int nombreTam;
		int direccionContenidoTam;

		memcpy(&(paqueteDAM->tipoOperacion), buffer, 4 );
		memcpy(&(paqueteDAM->resultado),buffer + 4, 4 );
		memcpy(&(paqueteDAM->idProcesoSolicitante),buffer + 2*4, 4 );
		memcpy(&(nombreTam),buffer + 3*4, 4 );
		memcpy(&(direccionContenidoTam),buffer + 4 * 4 , 4 );
		memcpy(&(paqueteDAM->archivoAsociado.numeroPagina),buffer+ 5*4, 4 );
		memcpy(&(paqueteDAM->archivoAsociado.numeroSegmento),buffer+ 6*4, 4 );
		memcpy(&(paqueteDAM->archivoAsociado.programCounter),buffer+ 7*4, 4 );

		if(nombreTam){

			char * buffer2 = malloc(sizeof(char)*(nombreTam+1));
			recv(socketSender,buffer2,nombreTam,0);
			buffer2[nombreTam] = '\0';
			paqueteDAM->archivoAsociado.nombre = string_new();
			string_append(&(paqueteDAM->archivoAsociado.nombre), buffer2);
			free(buffer2);


		}

		if(direccionContenidoTam){

			char * buffer2 = malloc(sizeof(char)*(direccionContenidoTam+1));
			recv(socketSender,buffer2,direccionContenidoTam,0);
			buffer2[direccionContenidoTam] = '\0';
			paqueteDAM->archivoAsociado.direccionContenido = string_new();
			string_append(&(paqueteDAM->archivoAsociado.direccionContenido), buffer2);
			free(buffer2);

		}


	}

	free(buffer);

	return flag;

}


void liberarPaqueteDAM(Paquete_DAM * p){

		//free(p->archivoAsociado.direccionContenido);
		//free(p->archivoAsociado.nombre);
		free(p);

	}

void ejecutarRetardo(char *path){

	printf("Ejecutando Retardo...");//faltaa con sleep
	t_config *config=config_create(path);
	int tiempoRetardo = config_get_int_value(config,"Retardo"); //tiempo en milisegundos
	usleep(tiempoRetardo*1000); //usleep toma el tiempo en microsegundo (por eso multiplico por 1000)
	config_destroy(config);
	printf("Termino el retardo");
}

Paquete_DAM* inicializarPaqueteDam(){

	Paquete_DAM *paquete = malloc(sizeof(Paquete_DAM));

	paquete->tipoOperacion = -1;
	paquete->resultado = -1;
	paquete->idProcesoSolicitante = -1;
	paquete->archivoAsociado.numeroPagina = -1;
	paquete->archivoAsociado.numeroSegmento = -1;
	paquete->archivoAsociado.programCounter = -1;
	paquete->archivoAsociado.direccionContenido = string_new();
	paquete->archivoAsociado.nombre = string_new();

	return paquete;
}
