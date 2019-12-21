#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "FuncionesConexiones.h"
#include "CPU.h"
#include <unistd.h>

int main(void){

	instructionPointer = 0;
	flagError =0;
	logger =log_create(logCPU,"CPU",1, LOG_LEVEL_INFO);
	sockSAFA=crearConexionCliente(pathSAFA);
	sockDAM=crearConexionCliente(pathDAM);
	sockFM9=crearConexionCliente(pathFM9);
    if((sockSAFA<0) || (sockDAM<0) || (sockFM9<0)){
		log_error(logger,"Error en la conexion con los clientes");
		return 1;
    }
    log_info(logger,"Se realizo correctamente la conexion con el SAFA,DAM,FM9");
//	enviarTipoDeCliente(sockcoordinador,ESI);
//	enviar(sockcoordinador,pack);
    send(sockSAFA,"1",2,0);
    send(sockDAM,"1",2,0);
    send(sockFM9,"1",2,0);
    Proceso *proc= malloc(sizeof(Proceso));
    Paquete_DAM *paquete = malloc(sizeof(Paquete_DAM));
    while(1){

		recibirProceso(sockSAFA, proc);
		log_info(logger,"Me llego un Proceso del safa");

		if((*proc).flagInicializacion == 0){ //LLego un DTB_DUMMY
			log_info(logger,"LLego un dtbDummy");

			(*paquete).idProcesoSolicitante = (*proc).idProceso;
			(*paquete).archivoAsociado.direccionContenido = string_new();
			string_append(&((*paquete).archivoAsociado.direccionContenido), (*proc).pathEscriptorio);
			(*paquete).resultado = 0; //Despues DAM pone si se pudo hacer el abrir o no (1 o 0)
			(*paquete).tipoOperacion = 1; //Porque es una operacion de ABRIR un archivo escriptorio, para despues cargarlo en memoria
			(*paquete).archivoAsociado.nombre = string_new();
			string_append(&((*paquete).archivoAsociado.nombre), (*paquete).archivoAsociado.direccionContenido);

//			------------------------------------------------------------------------------------------
//			que pasa con las otras variables de archivos asociados de paquete se inicializan aca?
//			revisar si esta bien
			(*paquete).archivoAsociado.numeroPagina =-1;
			(*paquete).archivoAsociado.numeroSegmento =-1;
			(*paquete).archivoAsociado.programCounter =-1;
//			-------------------------------------------------------------------


			enviarPaqueteDAM(sockDAM, paquete);
			log_info(logger,"Mando el dtbDummy al DAM");
			send(sockSAFA,"d",2,0); // Le aviso al SAFA que mande el dummy al DAM
			log_info(logger,"aviso al SAFA que mande el dummy al DAM");
		}
		else if ((*proc).flagInicializacion == 1) //LLego un DTB ya inicializado (el posta)
		{
			log_info(logger,"LLego un dtb posta que se va a ejecutar linea por linea");
//			recibirPaqueteDAM(sockSAFA, paquete);    VER SI ESTA LINEA VA O NO... (CREEMOS QUE NO VA)
//			-----------------------------------------------------------------
//			si le asignas proc porque desp le haces un malloc? no tiene sentido
			DTB = proc; //fijarse como se escribe
//			lo comento
/*
			DTB = malloc(sizeof(Proceso));
			(*DTB).idProceso = (*proc).idProceso;
			(*DTB).estado = (*proc).estado;
			(*DTB).socketProceso = (*proc).socketProceso;
			(*DTB).seEstaCargando = (*proc).seEstaCargando; //EEEEEEEEEEEEEEEEEEE
			(*DTB).archivosAsociados = list_create();
			(*DTB).archivosAsociados = (*proc).archivosAsociados;

*/

//			(*DTB). = (*proc).seEstaCargando;
			sem_init(&sem_Ejecutar_Script,0,0);
			sem_init(&sem_termine_Ejecutar_Script,0,0);
			sem_post(&sem_Ejecutar_Script);
			pthread_create(&hiloEjecutarScript,NULL,ejecutarCodigoEscriptorio,NULL);
			sem_wait(&sem_termine_Ejecutar_Script);
			if(flagError == 1){
				log_info(logger,"Termine de ejecutar mi escriptorio");
				send(sockSAFA,"f",2,0);
			}else if(flagError == 0){
				log_info(logger,"Abortando cpu ...enviando al Safa");
			}

			DTB = NULL;
		}

	}

    free(proc);
	liberarPaqueteDAM(paquete);
	log_destroy(logger);
	close(sockSAFA);
	close(sockFM9);
	close(sockDAM);
	return 1;
}

void *ejecutarCodigoEscriptorio(){
	char* buff = malloc(2);
	while(1){
		sem_wait(&sem_Ejecutar_Script);
		log_info(logger,"se entro a ejecutar el escriptorio  en ejecucion");
		char *codigo = DTB->pathEscriptorio;//revisar
//		paqueteActual= inicializarPaqueteDam();
//		char *codigoRestante = malloc(500);//va el tam de linea
//		codigoRestante = codigo;
		log_info(logger,"eee");
	  //para probar .. dps borrar ......
//		char test[] = "abrir racing.txt \n signal manzana \n signal manzana \n";
//		codigoRestante = test;
//		codigoRestante = "abrir racing.txt \n signal manzana \n signal manzana \n" ;
		//char str[] = "\n\nabrir \n\n \n close\n#pepe close\n##abrir\n flush";
	///--------------
		char *sentencia;
		while(1){
//			if(recv(sockSAFA,buff,2,0)){
//				//analizas primera linea del script, ejecutas sentencia
//
//				codigoRestante = parser(codigoRestante);
//				if(codigoRestante == NULL){ // si devuelve null significa que ejecuto la ultima linea
//					break;
//				}
//			}//EJECUCION
			//me tiene que mandar un paq dam
			if(recv(sockSAFA,buff,2,0)){
				if(buff[0] == '1'){

					sentencia = pedirSentencia();
					if (sentencia == NULL){
						flagError = 1;
						break;
					}
					instructionPointer++;
					parserSentencia(sentencia);

				}
			}

		}
		sem_post(&sem_termine_Ejecutar_Script);
	}


	}


int parserSentencia(char* sentencia){
	log_info(logger,"analizando sentencia");
	 char* token = strtok_r(sentencia, " ", &sentencia);
	 log_info(logger,"sentencia: %s",token);
		if(token != NULL){
			ejecutarRetardo(pathCPU); //Se ejecuta el retardo antes de cada operacion
			if(token[0]=='#'){

				printf("estoy en numeral\n");
				return 0;
			}
			if(!strcmp(token,"abrir")){

				printf("estoy en abrir\n");
				archivoABuscar = string_new();
				string_append(&archivoABuscar,sentencia);
				if(list_any_satisfy(DTB->archivosAsociados, &archivoAbierto)){
					log_info(logger, "Archivo se encontraba abierto");
					send(sockSAFA,"r",2,0); // Le aviso al SAFA que ejecuto sentencia;
				}else
				{
					DatosArchivo *archivoAsociado = malloc(sizeof(DatosArchivo));
					(*archivoAsociado).nombre = sentencia;
					(*archivoAsociado).direccionContenido = sentencia;
					list_add(DTB->archivosAsociados,archivoAsociado);

					Paquete_DAM *paquete = inicializarPaqueteDam();
					(*paquete).idProcesoSolicitante = (*DTB).idProceso;
					string_append(&((*paquete).archivoAsociado.direccionContenido), sentencia);
					(*paquete).resultado = 0; //Despues DAM pone si se pudo hacer el abrir o no (1 o 0)
					(*paquete).tipoOperacion = 1; //Porque es una operacion de ABRIR
					string_append(&((*paquete).archivoAsociado.nombre), sentencia);
					enviarPaqueteDAM(sockDAM, paquete);
	     			send(sockSAFA,"b",2,0); // Le aviso al SAFA que bloquee
	     			liberarPaqueteDAM(paquete);

				}

				return 0;


			}else if(!strcmp(token,"concentrar")){
//agregar concentrar
				printf("estoy en concentrar\n");
				send(sockSAFA,"r",2,0);
				return 0;

			}else if(!strcmp(token,"asignar")){

				printf("estoy en asignar\n");
				archivoABuscar = string_new();
				string_append(&archivoABuscar,sentencia);
				int* flagFM9 = malloc(sizeof(int));
				if(list_any_satisfy(DTB->archivosAsociados, &archivoAbierto))
				{
					Paquete_DAM *paquete = inicializarPaqueteDam();
					(*paquete).idProcesoSolicitante = (*DTB).idProceso;
					string_append(&((*paquete).archivoAsociado.direccionContenido), sentencia);
					(*paquete).resultado = 0;
					(*paquete).tipoOperacion = 3; //Porque es una operacion de asignar
					string_append(&((*paquete).archivoAsociado.nombre), sentencia);
					enviarPaqueteDAM(sockFM9, paquete);
					recv(sockFM9,flagFM9,sizeof(int),0);
					recibirPaqueteDAM(sockFM9, paquete);//puede ser que sea el gral
					//verifico que no este cargado
					if((*flagFM9) == 20002)
					{
						log_info(logger,"Error: 40002 fallo de segmento/ memoria");
						send(sockSAFA,"e",2,0); // Le aviso al SAFA del error
						sem_post(&sem_termine_Ejecutar_Script);
						pthread_cancel(pthread_self());
					}
//					liberarPaqueteDAM(paquete);
				}
				else
				{

					log_info(logger,"Error:20001 no se encontraba abierto");
					send(sockSAFA,"e",2,0); // Le aviso al SAFA que del error
					sem_post(&sem_termine_Ejecutar_Script);
					pthread_cancel(pthread_self());
				}



			return 0;

			}else if(!strcmp(token,"wait")){

				printf("estoy en wait");
				int tam =string_length(sentencia);
				send(sockSAFA,"w",2,0);//por sentencia wait
				void* buffer = malloc(tam+4);//4 por int
				memcpy(buffer,&(tam),4) ;
				memcpy(buffer+4,sentencia,tam) ;
				send(sockSAFA,buffer,tam+ 4,0);
				free(buffer);
//				char* buf = malloc(2);
//				recv(sockSAFA, buf, 2, 0);
//				if(buf[0] == '0'){
//					//se detiene la ejecucion
//				}


			}else if(!strcmp(token,"signal")){

				printf("estoy en signal");
				int tam =string_length(sentencia);
			    send(sockSAFA,"s",2,0);//por sentencia signal
				void* buffer = malloc(tam+4);
				memcpy(buffer,&(tam),4) ;
				memcpy(buffer+4,sentencia,tam) ;
				send(sockSAFA,buffer,tam+ 4,0);
				free(buffer);
//				char* buf = malloc(2);
//				recv(sockSAFA, buf, 2, 0);
//				if(buf[0] == '0'){
//								//error en la ejecucion
//				}


			}else if(!strcmp(token,"flush")){

				printf("estoy en flush\n");
				//verificacion del archivo abierto

				if(list_any_satisfy(DTB->archivosAsociados, &archivoAbierto))
				{
					Paquete_DAM *paquete = inicializarPaqueteDam();

					(*paquete).idProcesoSolicitante = (*DTB).idProceso;
					string_append(&((*paquete).archivoAsociado.direccionContenido), sentencia);
					(*paquete).resultado = 0; //Despues DAM pone si se pudo hacer el abrir o no (1 o 0)
					(*paquete).tipoOperacion = 6; //Porque es una operacion de flush
					string_append(&((*paquete).archivoAsociado.nombre), sentencia);//ver mas a delante
					enviarPaqueteDAM(sockDAM, paquete);
					send(sockSAFA,"b",2,0); // Le aviso al SAFA que bloquee

					liberarPaqueteDAM(paquete);
				}
				else
				{
					log_info(logger,"Error:30001 no se encontraba abierto");
					send(sockSAFA,"e",2,0); // Le aviso al SAFA que del error
					sem_post(&sem_termine_Ejecutar_Script);
					pthread_cancel(pthread_self());
				}



			}else if(!strcmp(token,"close")){

				printf("estoy en close\n");
				int* flagFM9 = malloc(sizeof(int));
				if(list_any_satisfy(DTB->archivosAsociados, &archivoAbierto))
				{
					Paquete_DAM *paquete = inicializarPaqueteDam();

					(*paquete).idProcesoSolicitante = (*DTB).idProceso;
					string_append(&((*paquete).archivoAsociado.direccionContenido), sentencia);
					(*paquete).resultado = 0;
					(*paquete).tipoOperacion = 7; //Porque es una operacion de close
					string_append(&((*paquete).archivoAsociado.nombre), sentencia);
					enviarPaqueteDAM(sockFM9, paquete);
					recv(sockFM9,flagFM9,sizeof(int),0);

					//verifico que no este cargado
					if((*flagFM9) == 40002)
					{
						log_info(logger,"Error: 40002 fallo de segmento/ memoria");
						send(sockSAFA,"e",2,0); // Le aviso al SAFA del error
						sem_post(&sem_termine_Ejecutar_Script);
						pthread_cancel(pthread_self());
					}
					send(sockSAFA,"r",2,0);
					liberarPaqueteDAM(paquete);
					free(flagFM9);
					strcpy(archivoABuscar,sentencia);
					list_remove_by_condition(DTB->archivosAsociados, &archivoAbierto);
				}
				else
				{
					log_info(logger,"Error:40001 no se encontraba abierto");
					send(sockSAFA,"e",2,0); // Le aviso al SAFA que del error
					sem_post(&sem_termine_Ejecutar_Script);
					pthread_cancel(pthread_self());
				}
			}else if(!strcmp(token,"crear")){

				printf("estoy en crear\n");

				Paquete_DAM *paquete = inicializarPaqueteDam();

				(*paquete).idProcesoSolicitante = (*DTB).idProceso;
				string_append(&((*paquete).archivoAsociado.direccionContenido), sentencia);
				(*paquete).resultado = 0;
				(*paquete).tipoOperacion = 8; //Porque es una operacion de crear
				string_append(&((*paquete).archivoAsociado.nombre), sentencia);
				enviarPaqueteDAM(sockDAM, paquete);
				send(sockSAFA,"b",2,0);


			}else if(!strcmp(token,"borrar")){

				printf("estoy en borrar");
				Paquete_DAM *paquete = inicializarPaqueteDam();

				(*paquete).idProcesoSolicitante = (*DTB).idProceso;
				string_append(&((*paquete).archivoAsociado.direccionContenido), sentencia);
				(*paquete).resultado = 0;
				(*paquete).tipoOperacion = 9; //Porque es una operacion de borrar
				string_append(&((*paquete).archivoAsociado.nombre), sentencia);

				enviarPaqueteDAM(sockDAM, paquete);
				send(sockSAFA,"b",2,0);


			}
			else if(token[0]=='\0'){
				printf("estoy en (\\0) barra cero\n");


			}else printf("lenguaje invalido");


		}else{

				printf("sentencia nula\n");
			}

		return 1;
}

char* parser( char* aParsear){

	char* resto = aParsear;
	char* sentencia;

	if(aParsear !=NULL){
		log_info(logger,"arranco a parsear");
		log_info(logger,"codigo a parsear...: %s", resto);
		sentencia = strtok_r(resto,"\n", &resto);		//ESTO TIRA SEGMENTATION FAULT
		log_info(logger,"aaaaa");
		if (parserSentencia(sentencia) == 1){
			log_info(logger,"\n\nresto..: %s", resto );
			log_info(logger,"\n\nSTERLENresto..: %d", strlen(resto));
			if (strlen(resto) == 0){
				log_info(logger,"\n\n\nENTRE AL SUPER IF");
				return NULL;
			}
			send(sockSAFA,"r",2,0);
		}
		else{
			parser(resto); 	//Recursivo, se saltea el comentario y ejecuta la sentencia que sigue
		}
		return resto;
	}
	return NULL;
}

//bool existePath(char * a){
//	char* path = string_new();
//	string_append(&path, a);
//	Proceso *proc=(Proceso*) proceso;
//	if((*proc).idProceso==idBuscar)
//
//		return true;
//	else
//		return false;
//}

bool archivoAbierto(void *archivo){
	DatosArchivo *b=(DatosArchivo*) archivo;
	if (strcmp ((*b).nombre , archivoABuscar) == 0){ //son iguales
		return true;
	}
	else
		return false;
}

char* pedirSentencia(){
	log_info(logger,"DTB PATH: %s ", DTB->pathEscriptorio);
	log_info(logger,"Pidiendo Sentencia al FM9...");
	char* sentencia = NULL;
	int nombreTam = 0;
//	Paquete_DAM * paqueteActual = malloc(sizeof(Paquete_DAM));
	Paquete_DAM * paqueteActual = inicializarPaqueteDam();
	paqueteActual->idProcesoSolicitante =DTB->idProceso;
	paqueteActual->resultado = -1;
	paqueteActual->tipoOperacion = 39;
	paqueteActual->archivoAsociado.programCounter = instructionPointer;
	log_info(logger,"Estoy medio 0 pedir sentencia...");
//	----------------------------------------
//	strcpy(archivoAbuscar,DTB->pathEscriptorio);  creo que esto tiraba seg fault ya que no tenia malloc
	archivoAbuscar = string_new();
	string_append(&archivoAbuscar, DTB->pathEscriptorio);
//	----------------------------------------
//	log_info(logger,"Estoy medio pedir sentencia...");
	DatosArchivo *archivoAsociadoCargado = (DatosArchivo*) list_find((*DTB).archivosAsociados,&esArchivoBuscado);
//	log_info(logger,"Estoy medio 1 pedir sentencia...");
	paqueteActual->archivoAsociado.numeroPagina=(*archivoAsociadoCargado).numeroPagina;
	paqueteActual->archivoAsociado.numeroSegmento = (*archivoAsociadoCargado).numeroSegmento;
	log_info(logger,"pido datos : %d, %d ", paqueteActual->archivoAsociado.numeroPagina,paqueteActual->archivoAsociado.numeroSegmento);
	string_append(&(paqueteActual->archivoAsociado.direccionContenido),DTB->pathEscriptorio);
	string_append(&(paqueteActual->archivoAsociado.nombre),DTB->pathEscriptorio);
	//	log_info(logger,"Estoy medio 1.7 pedir sentencia...");
//	agrego un send de prueba
//	 send(sockFM9,"1",2,0);

	enviarPaqueteDAM(sockFM9, paqueteActual);
	log_info(logger,"Estoy medio 2 pedir sentencia...");
	recv(sockFM9,&nombreTam,sizeof(int),0); //se queda esperando aca----------------
	if(nombreTam > 0){

		log_info(logger,"Estoy medio 2.1 pedir sentencia if...");
				char * buffer2 = malloc(nombreTam+1);
				recv(sockFM9,buffer2,nombreTam,0);
				log_info(logger,"Estoy medio 2.2 pedir sentencia if...");
				buffer2[nombreTam] = '\0';
				sentencia = string_new();
				string_append(&(sentencia), buffer2);
				free(buffer2);


	}
	log_info(logger,"Estoy medio 3 pedir sentencia...");
	free(paqueteActual);
	log_info(logger,"Estoy medio 4 pedir sentencia...");
	return sentencia;
}

bool esArchivoBuscado(void *archivo){
	DatosArchivo *b=(DatosArchivo*) archivo;
	if (strcmp ((*b).nombre , archivoAbuscar) == 0){ //son iguales
		return true;
	}
	else
		return false;
}
