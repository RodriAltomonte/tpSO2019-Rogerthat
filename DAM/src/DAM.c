#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <stdio.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include "FuncionesConexiones.h"
#include "DAM.h"

int guard(int n, char * err) { if (n == -1) { perror(err); exit(1); } return n; }
int cantidadDeCPUs = 0 ;

void* conexionCPU(void* nuevoCliente){
	int sockCPU = *(int*)nuevoCliente;
	sem_post(&sem_esperaInicializacion);
	//void* buf = malloc(sizeof(Paquete_DAM));
	int partes;
	t_config *config=config_create(pathDAM);
	int transferSize = config_get_int_value(config,"TRANSFER_SIZE");//asignar tamaño
	config_destroy(config);

	log_info(logger,"Estoy en el hilo del CPU");
//	fcntl(sockCPU, F_SETFL, O_NONBLOCK); //para hacer noBlockeante el sockCPU
	Paquete_DAM *paquete = inicializarPaqueteDam();
	while(1)
	{

//		Paquete_DAM *paquete = malloc(sizeof(Paquete_DAM));
		if(recibirPaqueteDAM(sockCPU, paquete) !=-1)
		log_info(logger,"Se recibio un paquete de operacion proveniente del CPU");
		else
			log_info(logger,"Error de un paquete de operacion proveniente del CPU");

		switch((*paquete).tipoOperacion)
		{
			case 1: //abrir

     		log_info(logger,"Ehhh, voy a buscar %s para %d ",(*paquete).archivoAsociado.direccionContenido,
     														(*paquete).idProcesoSolicitante);


     		char* buffer = malloc(transferSize* sizeof(char));
     		int flagMDJ=0; //si es 1 ok existe en mdj 0 no existe
//     		char* contenido = string_new();

     		enviarPaqueteDAM(sockMDJ, paquete);
     		recv(sockMDJ,&flagMDJ,sizeof(int),0);


     		//verifico si existe
     		if(flagMDJ==1){
     			recv(sockMDJ,&partes,sizeof(int),0);
     			enviarPaqueteDAM(sockFM9, paquete);
     			log_info(logger,"Partes: %d", partes);
     			log_info(logger,"Enviando orden a FM9 para que cargue en memoria el archivo (via transfer size)");
     			send(sockFM9,&partes,sizeof(int),0);
     			int transferSizeRestante;
     			while( partes > 0){
     				if(partes == 1)
     				{
     					recv(sockMDJ,&transferSizeRestante,sizeof(int),0);
     					recv(sockMDJ,buffer,transferSizeRestante,0);
     					send(sockFM9,&transferSizeRestante,sizeof(int),0);
     					send(sockFM9,buffer,transferSizeRestante,0);
     				}
     				else
     				{
						recv(sockMDJ,buffer,transferSize,0);
	//     				string_append(&(contenido), buffer);
						send(sockFM9,buffer,transferSize,0);
     				}
     				partes--;
     				log_info(logger,"Partes restantes..: %d", partes);
     			}
     			recibirPaqueteDAM(sockFM9, paquete);
     			log_info(logger,"FM9 pudo guardar el archivo");
     		}
     		else if(flagMDJ==0){
     			paquete->resultado = 0; //por Path inexistente
     		}


//     		//para pruebas (dps comentar  y poner la linea de arriba del sendMDJ -------------
//			(*paquete).resultado = 1 ;
//			free((*paquete).archivoAsociado.direccionContenido);
//			(*paquete).archivoAsociado.direccionContenido = string_new();
//			string_append(&((*paquete).archivoAsociado.direccionContenido), "abrir racing.txt \n signal manzana \n signal manzana \n");
     		log_info(logger,"Enviando a SAFA resultado operacion..");
			if(enviarPaqueteDAM(sockSAFA, paquete) == -1)
				log_info(logger,"error al enviarPaqueteDAM al Safa");

			//-----------------
			break;
			case 8:  //crear

				enviarPaqueteDAM(sockMDJ, paquete);
				recv(sockMDJ,&partes,sizeof(int),0);

				//termine el crear
				if(partes == 1){
					paquete->resultado = 1;
				}else if(partes == 0)
					paquete->resultado = 0;
				enviarPaqueteDAM(sockSAFA, paquete);

			break;
			case 9:  //borrar

				enviarPaqueteDAM(sockMDJ, paquete);
				recv(sockMDJ,&partes,sizeof(int),0);

				//termine el borrar
				if(partes == 1){
					paquete->resultado =1;
				}else if(partes == 0)
					paquete->resultado =0;
				enviarPaqueteDAM(sockSAFA, paquete);

			break;
			case 6:  //flush

				enviarPaqueteDAM(sockFM9, paquete);
				recv(sockFM9,&partes,sizeof(int),0);
				enviarPaqueteDAM(sockMDJ, paquete);
				send(sockMDJ,&partes,sizeof(int),0);

				while( partes > 0){

					recv(sockFM9,buffer,transferSize,0);
	//     				string_append(&(contenido), buffer);
					send(sockMDJ,buffer,transferSize,0);
					partes--;
				}

				recv(sockMDJ,&partes,sizeof(int),0);

				//termine el flush
				if(partes == 1){
					paquete->resultado =1;
				}
				else if(partes == 0)
					paquete->resultado =0;

				enviarPaqueteDAM(sockSAFA, paquete);


			break;
//			case 39:  //pedir proxima sentencia
//
//							enviarPaqueteDAM(sockFM9, paquete);
//							int nombreTam = 0;
//							recv(sockFM9,&nombreTam,sizeof(int),0);
//							send(sockCPU,&nombreTam,sizeof(int),0);
//							if(nombreTam){
//
//										void *buffer2 = malloc(nombreTam);
//										recv(sockFM9,buffer2,nombreTam,0);
//										send(sockCPU,buffer2,nombreTam,0);
//										free(buffer2);
//
//							}
//							recibirPaqueteDAM(sockFM9, paquete);
//							enviarPaqueteDAM(sockSAFA, paquete);
//
//			break;
		}
	}
	return 0;
}


void* conexionFM9(void* nuevoCliente){
	sockFM9 = *(int*)nuevoCliente;
	sem_post(&sem_esperaInicializacion);
	/*
	fcntl(sockFM9, F_SETFL, O_NONBLOCK); //para hacer noBlockeante el sockFM9
	log_info(logger,"Estoy en el hilo del FM9");
//	void* buf = malloc(sizeof(Paquete_DAM));
	while(1)
	{
		Paquete_DAM *paquete= malloc(sizeof(Paquete_DAM));
		while(recibirPaqueteDAM(sockFM9,paquete) == -1){
		}
//		while(recv(sockFM9,buf,sizeof(Paquete_DAM),0) == -1){
//		}
//		Paquete_DAM *paquete= (Paquete_DAM *) deserializarPaqueteDAM(buf);
		log_info(logger,"Se recibio un paquete de operacion proveniente del FM9");
		switch((*paquete).tipoOperacion)
		{
			case 1: //abrir
			log_info(logger,"Mandando al SAFA el resultado de la carga en memoria del archivo");
			enviarPaqueteDAM(sockSAFA,paquete);
//			send(sockSAFA,buf,sizeof(Paquete_DAM),0);
			break;
			case 6:  //flush
			enviarPaqueteDAM(sockMDJ,paquete);
//			send(sockMDJ,buf,sizeof(Paquete_DAM),0);
			break;

		}
//		liberarPaqueteDAM(paquete);
	}*/
	return 0;
}

void* conexionSAFA(void* nuevoCliente){
	int sockSAFA = *(int*)nuevoCliente;
	log_info(logger,"Estoy en el hilo del SAFA");


	return 0;
}

void* conexionMDJ(void* nuevoCliente){
	int sockMDJ = *(int*)nuevoCliente;
	log_info(logger,"Estoy en el hilo del MDJ");
	fcntl(sockMDJ, F_SETFL, O_NONBLOCK); //para hacer noBlockeante el sockDAM
//	void* buf = malloc(sizeof(Paquete_DAM));
//	while(1)
//	{
		Paquete_DAM *paquete= malloc(sizeof(Paquete_DAM));
		while(recibirPaqueteDAM(sockMDJ,paquete) == -1){
		}
//		while(recv(sockMDJ,buf,sizeof(Paquete_DAM),0) == -1){
//		}
//		Paquete_DAM *paquete= (Paquete_DAM *) deserializarPaqueteDAM(buf);
		log_info(logger,"Se recibio un paquete de operacion proveniente del MDJ");
		switch((*paquete).tipoOperacion)
		{
			case 1:  //abrir
			log_info(logger,"Enviando orden a FM9 para que cargue en memoria el archivo");
			enviarPaqueteDAM(sockFM9,paquete);
//			send(sockFM9,buf,sizeof(Paquete_DAM),0);
			break;
			case 6:  //flush
			log_info(logger,"Enviando orden a SAFA resultado operacion del flush");
			enviarPaqueteDAM(sockSAFA,paquete);
//			send(sockSAFA,buf,sizeof(Paquete_DAM),0);
			break;

		}
//		liberarPaqueteDAM(paquete);
//	}
	return 0;
}




int main(void){

	logger = log_create(logDAM,"DAM",1, LOG_LEVEL_INFO);

//		recv(cliente,buffer,25, MSG_WAITALL);
//		printf("%s",buffer);
//		printf("\n");

	sem_init(&sem_esperaInicializacion,0,0);
	sockSAFA=crearConexionCliente(pathSAFA);
	if(sockSAFA<0 ){
		log_error(logger,"Error en la conexion con el cliente");
		return 1;
	}
	log_info(logger,"Se realizo correctamente la conexion con SAFA ");
	send(sockSAFA,"2",2,0);
	pthread_create(&hiloSAFA,NULL,conexionSAFA,(void*)&sockSAFA);
	sockMDJ=crearConexionCliente(pathMDJ);
	if(sockMDJ<0 ){
		log_error(logger,"Error en la conexion con el cliente");
		return 1;
	}
	log_info(logger,"Se realizo correctamente la conexion con MDJ");
	send(sockMDJ,"2",2,0);
//	 pthread_create(&hiloMDJ,NULL,conexionMDJ,(void*)&sockMDJ);   REVISAAAAAAAAAAAAAR SI NO VA ESTO
	 sockDAM = crearConexionServidor(pathDAM);
	 if (listen(sockDAM, 10) == -1){ //ESCUCHA!
		 log_error(logger, "No se pudo escuchar");
	 	 log_destroy(logger);
	 	 return -1;
	 }
	 int flags = guard(fcntl(sockDAM, F_GETFL), "could not get flags on TCP listening socket");
//	 fcntl(sockDAM, F_SETFL, O_NONBLOCK); //para hacer noBlockeante el sockDAM
	 log_info(logger, "Se esta escuchando");
	  int i=0;
	  char* buff;
		int tipoCliente,sockCliente;

	  while((sockCliente = accept(sockDAM, NULL, NULL))) {
			if (sockCliente == -1) {
			  if (errno == EWOULDBLOCK) {
//				log_info(logger,"No pending connections; sleeping for one second.\n");
//				sleep(1);
			  } else {
				log_info(logger,"error when accepting connection");
				exit(1);
			  }
			} else {
			  i++;
			  buff= malloc(2);
			  recv(sockCliente,buff,2,0);
			  log_info(logger, "Llego una nueva conexion");
			  if(buff[0] == '1'){
				  hilosCPUs = realloc(hilosCPUs,sizeof(pthread_t)*(cantidadDeCPUs+1));
				  if(pthread_create(&(hilosCPUs[cantidadDeCPUs]),NULL,conexionCPU,(void*)&sockCliente)<0)
				  {
					  log_error(logger,"No se pudo crear un hilo");
					  return -1;
				  }
				  cantidadDeCPUs++;
			  }
			  else if(buff[0] == '3'){
				  pthread_create(&hiloFM9,NULL,conexionFM9,(void*)&sockCliente);
			  }
			  free(buff);
		//      send(sockCliente, msg, sizeof(msg), 0);
	//	      close(sockCliente);
			}
			sem_wait(&sem_esperaInicializacion);
	  }
	  sleep(1);
	  close(sockDAM);

}
