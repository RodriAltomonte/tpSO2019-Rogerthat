#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <stdio.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include "FuncionesConexiones.h"
#include "Consola_S-AFA.h"
#include "S-AFA.h"

int guard(int n, char * err) { if (n == -1) { perror(err); exit(1); } return n; }
int cantidadDeCPUs = 0 ;
int cantidadProcesosEnMemoria = 0;
int sockDAM;
int quantumRestante;

void* conexionCPU(void* nuevoCliente){
	int sockCPU = *(int*)nuevoCliente; //asi estaba ..NO ANDA
//	ConexionCPU *cpu = list_get(cpu_conectadas,list_size(cpu_conectadas)-1);
//	int sockCPU = (*cpu).socketCPU;   //con esto lo copia bien el socket pero no anda el recv
//	int sockCPU = dup(sock);
	log_info(logger, "socket cpu dentro del hilo..: %d ",sockCPU);
//	send(sockCPU,'p',2,0); //prueba..dps borrar
	log_info(logger,"Estoy en el hilo del CPU");
	sem_post(&sem_esperaInicializacion);
	t_config *config=config_create(pathSAFA);
	int quantum = config_get_int_value(config, "Quantum");
	char *buf ;
//	fcntl(sockCPU, F_SETFL, O_NONBLOCK); //para hacer noBlockeante el sockCPU
	while(1)
	{
		 buf = malloc(2);
//		 while(recv(sockCPU, buf, 2, 0) == -1){
//			 log_info(logger,"buffer por ahora.. : %s",buf);
//			 log_info(logger, "socket cpu dentro del hilo..: %d ",sockCPU);
//		 }
		 recv(sockCPU, buf, 2, 0);
		 log_info(logger,"llego un dato de cpu");
		 log_info(logger,"buffer.. : %s",buf);
		   //aca hago un case de los posibles send de una Cpu, que son
		   //f: termino de ejecutar; r: ejecuto una sentencia (puede desalojar por fin de quantum o no)
		   Estado estado;
		   int tam;
		   if((buf[0] == 'f') || (buf[0] == 'e'))
		   {	//se abortó su ejecucion
			   if (buf[0] == 'e')
			   {
				   log_info(logger,"Se aborta el DTBlock porque el script de la CPU tuvo que abortar debido a que ocurrio un error inesperado");
			   }
			   else //finalizo su ejecucion
			   {
				   log_info(logger,"La CPU termino de ejecutar su script");
			   }
			   if(procesoEnEjecucion!=NULL){
				   idCPUBuscar = sockCPU;
				   log_info(logger,"EEEEE");
				   liberarRecursos(procesoEnEjecucion);
				   ConexionCPU *cpuUtilizada = (ConexionCPU*) list_find(cpu_conectadas,&cpuUsadaEsIdABuscar);
//				   ConexionCPU *cpuUtilizada = list_get(cpu_conectadas,sockCPU);//fijarse si funciona
//				   log_info(logger,"conexion cpu es %d,el num %d", cpuUtilizada->socketCPU,cpuUtilizada->tieneDTBasociado );
				   (*cpuUtilizada).tieneDTBasociado = 0;
				   log_info(logger,"ANDA EL FIND");
				   list_add(procesos_terminados,procesoEnEjecucion);
				   procesoEnEjecucion->estado = finalizado ;
				   log_info(logger,"oooooooOooOooOOO");
				   procesoEnEjecucion=NULL;
				   sem_post(&sem_CPU_ejecutoUnaSentencia);
//				   if(list_size(procesos_listos) + list_size(procesos_bloqueados) < config_get_int_value(config, "Grado de multiprogramacion")) { //chequeo si puedo poner otro proceso en ready
//				  			sem_post(&sem_cargarProceso);    //desbloquea funcion que crea el dtbDummy
//				   }
				   sem_post(&sem_replanificar);
				   quantumRestante = quantum; //reinicio el quantum para el proximo proceso que se vaya a ejecutar

			   }
		   }
		   if(buf[0] == 'r')
		   { 	//CPU ejecuto una sentencia
			   log_info(logger,"La CPU ejecuto una sentencia");
			   analizarFinEjecucionLineaProceso(quantum);
		   }
		   if(buf[0] == 'd')  // DPS CAMBIAR ESTO PARA QUE  SE BLOQUEE EL DTBDUMMY (COMO ESTA ABAJO) Y CUANDO SE CARGA EL DTBDUMMY EN MEMORIA, SACARLO DE LA COLA DE BLOQUEADOS
		   { //CPU mando a ejecutar el dummy
			   log_info(logger,"La CPU mando  un dtbDummy al DAM");
			   procesoEnEjecucion->estado = bloqueado;
			   procesoEnEjecucion=NULL;
			   sem_post(&sem_replanificar);
			   quantumRestante = quantum;
		   }
		   if(buf[0] == 'b') //se debe bloquear el procesoEnEjecucion
		   {
			   quantumRestante--;
			   if(verificarSiFinalizarElProcesoEnEjecucion(quantum) == false) //si finalizo el proceso no lo tengo que bloquear
			   {
				   log_info(logger,"Se bloquea el DTBlock en ejecucion");
				   procesoEnEjecucion->estado = bloqueado;
				   ProcesoBloqueado *procesoAbloquear = malloc(sizeof(ProcesoBloqueado));
				   (*procesoAbloquear).proceso = procesoEnEjecucion;
				   char*algoritmo= config_get_string_value(config, "Algoritmo");
				   if(!strcmp(algoritmo,"VRR")){
					   (*procesoAbloquear).quantumPrioridad = quantumRestante;
				   }
				   else
				   {
					   (*procesoAbloquear).quantumPrioridad = 0; //como no es VRR no hay cola mayor prioridad
				   }

				   list_add(procesos_bloqueados,procesoAbloquear);
				   procesoEnEjecucion=NULL;
				   sem_post(&sem_replanificar);
				   quantumRestante = quantum;
			   }
			   sem_post(&sem_CPU_ejecutoUnaSentencia);
		   }
		   if(buf[0] == 'w')
		   {
			   log_info(logger,"La CPU ejecuto una sentencia de wait");
			   char *buf2 ;
			   recv(sockCPU,&tam,sizeof(int),0);
			   buf2 = malloc(tam);
			   recv(sockCPU, buf2, tam, 0);  //PONER EN EL FINALIZAR DTBLOCK QUE SI AL FINALIZAR EL PROCESO DEBE LIBERAR LOS RECURSOS QUE RETENIA
			   //buf2 tiene el recurso ahora
			   strcpy(recursoABuscar,buf2);
			   Recurso *recurso = (Recurso*) list_find(recursos,&existeRecurso);
			   if(recurso)  //si existe el recurso
			   {
				   if((*recurso).instancias > 0)
				   {
					   (*recurso).instancias--;
					   strcpy(recursoABuscar,buf2);
					   Recurso *recursoYaRetenido = (Recurso*) list_find((*procesoEnEjecucion).recursosRetenidos,&existeRecurso);
					   if(recursoYaRetenido){
						   (*recursoYaRetenido).instancias++;  //El proceso retiene otra instancia mas
					   }
					   else
					   {
						   Recurso *recursoRetenido = malloc(sizeof(Recurso));
						   (*recursoRetenido).nombre = (*recurso).nombre;
						   (*recursoRetenido).instancias = 1;
						   list_add((*procesoEnEjecucion).recursosRetenidos,recursoRetenido);
					   }
					   //Analizo lo mismo que cuando ejecuto una sentencia comun (cpun envia 'r')
					   analizarFinEjecucionLineaProceso(quantum);
				   }
				   else
				   {
					   quantumRestante--;
					   if(verificarSiFinalizarElProcesoEnEjecucion(quantum) == false) //si finalizo el proceso no lo tengo que bloquear
					   {
						   (*recurso).instancias--;
						   log_info(logger,"Se bloquea el proceso por falta de instancias disponibles del recurso pedido");
						   list_add((*recurso).procesosEnEspera,procesoEnEjecucion);
						   procesoEnEjecucion->estado = bloqueado ;
						   ProcesoBloqueado *procesoAbloquear = malloc(sizeof(ProcesoBloqueado));
						   (*procesoAbloquear).proceso = procesoEnEjecucion;
						   (*procesoAbloquear).quantumPrioridad = 0;
						   list_add(procesos_bloqueados,procesoAbloquear);
						   procesoEnEjecucion=NULL;
						   sem_post(&sem_replanificar);
						   quantumRestante = quantum;
					   }
					   sem_post(&sem_CPU_ejecutoUnaSentencia);
				   }
			   }
			   else //intancias = 0
			   {
				   log_info(logger,"Creando nuevo recurso");
				   Recurso *recursoNuevo = malloc(sizeof(Recurso));
				   (*recursoNuevo).nombre = buf2;
				   (*recursoNuevo).instancias = 1;	//lo inicializo en 1
				   (*recursoNuevo).instancias--;    //le decremento 1 instancia por el wait
				   (*recursoNuevo).procesosEnEspera = list_create();
				   Recurso *recursoNuevoARetener = malloc(sizeof(Recurso));
				   (*recursoNuevoARetener).nombre = (*recursoNuevo).nombre;
				   (*recursoNuevoARetener).instancias = 1;
				   list_add((*procesoEnEjecucion).recursosRetenidos,recursoNuevoARetener);
				   list_add(recursos,recursoNuevo);

				   //Analizo lo mismo que cuando ejecuto una sentencia comun (cpun envia 'r')
				   analizarFinEjecucionLineaProceso(quantum);
			   }

			   free(buf2);
		   }
		   if(buf[0] == 's')
		   {
			   log_info(logger,"La CPU ejecuto una sentencia de signal");
			   char *buf3 ;
			   recv(sockCPU,&tam,sizeof(int),0);
			   buf3 = malloc(tam);
			   recv(sockCPU, buf3, tam, 0);   //buf3 tiene el recurso ahora
			   strcpy(recursoABuscar,buf3);
			   Recurso *recurso2 = (Recurso*) list_find(recursos,&existeRecurso);
			   if(recurso2)  //si existe el recurso
			   {
				   if((*recurso2).instancias >= 0){
					   (*recurso2).instancias++;
				   }
				   else  //intancias < 0
				   {
					   log_info(logger,"Se desbloquea el primer proceso que estaba esperando el recurso en cuestion");
					   (*recurso2).instancias++;
					   Proceso *procesoADesbloquear = (Proceso*) list_remove((*recurso2).procesosEnEspera,0);
					   idBuscar = (*procesoADesbloquear).idProceso;
//					   procesoADesbloquear = (ProcesoBloqueado*) list_find(procesos_bloqueados,&procesoEsIdABuscar);
					   list_remove_by_condition(procesos_bloqueados,&procesoBloqueadoEsIdABuscar);
					   list_add(procesos_listos,procesoADesbloquear);
				   }
			   }
			   else
			   {
				   log_info(logger,"Creando nuevo recurso");
				   Recurso *recursoNuevo2 = malloc(sizeof(Recurso));
				   (*recursoNuevo2).nombre = buf3;
				   (*recursoNuevo2).instancias = 1;	//lo inicializo en 1
				   (*recursoNuevo2).procesosEnEspera = list_create();
				   list_add(recursos,recursoNuevo2);
			   }
			   //Analizo lo mismo que cuando ejecuto una sentencia comun (cpu envia 'r')
			   analizarFinEjecucionLineaProceso(quantum);
			   free(buf3);
		   }
		 free(buf);
	}
	return 0;
}


void* conexionDAM(void* nuevoCliente){
	int sockDAM = *(int*)nuevoCliente;
	sem_post(&sem_esperaInicializacion);
	log_info(logger,"Estoy en el hilo del DAM");
//	void* buf = malloc(sizeof(Paquete_DAM));
	int operacionRealizada;
	fcntl(sockDAM, F_SETFL, O_NONBLOCK); //para hacer noBlockeante el sockDAM
	ProcesoBloqueado *proceso;
	while(1){
		Paquete_DAM *paquete= malloc (sizeof(Paquete_DAM));
//		while(recv(sockDAM,buf,sizeof(Paquete_DAM),0) == -1){
//		}
		while(recibirPaqueteDAM(sockDAM,paquete) == -1);
//		Paquete_DAM *paquete= (Paquete_DAM *) deserializarPaqueteDAM(buf);
		log_info(logger,"Se recibio un paquete de fin de operacion del DAM");
		operacionRealizada = (*paquete).tipoOperacion ;
		switch(operacionRealizada)
		{
			case 1:  //Abrir archivo
				idBuscar = (*paquete).idProcesoSolicitante;
				Proceso *procesoAcargar = (Proceso*) list_find(procesos_nuevos,&procesoEsIdABuscar);
				list_remove_by_condition(procesos_nuevos,&procesoEsIdABuscar);
				(*procesoAcargar).seEstaCargando = 0 ; //ya se termino de hacer el dummy - ahora veo de ponerlo en ready..
				(*procesoAcargar).flagInicializacion = 1; //PROXIMAMENTE
				if(procesoAcargar){ //significa que era un abrir de un DTBdummy
					if ((*paquete).resultado == 1){ //operacion abrir realizada con exito
						log_info(logger,"DAM pudo abrir el archivo");
						DatosArchivo *archivoAsociado = malloc(sizeof(DatosArchivo));
						(*archivoAsociado).nombre = string_new();
						string_append(&((*archivoAsociado).nombre), (*paquete).archivoAsociado.nombre);
						(*archivoAsociado).direccionContenido = string_new();
						string_append(&((*archivoAsociado).direccionContenido), (*paquete).archivoAsociado.direccionContenido);
						(*archivoAsociado).numeroPagina = (*paquete).archivoAsociado.numeroPagina;
						(*archivoAsociado).numeroSegmento = (*paquete).archivoAsociado.numeroSegmento;
						list_add((*procesoAcargar).archivosAsociados,archivoAsociado);
						(*procesoAcargar).pathEscriptorio = string_new();
						string_append(&((*procesoAcargar).pathEscriptorio), (*paquete).archivoAsociado.direccionContenido);
//						strcpy(archivoAbuscar,(*paquete).archivoAsociado.nombre);
//						DatosArchivo *archivoAsociadoCargado = (DatosArchivo*) list_find((*procesoAcargar).archivosAsociados,&esArchivoBuscado);
						list_add(procesos_listos,procesoAcargar) ;
						log_info(logger,"Se agrego un proceso (luego de ejecutar dummy) a la cola de listos");
						log_info(logger,"cantidad procesos listos..: %d",list_size(procesos_listos));
						if(list_size(procesos_listos) == 1 && procesoEnEjecucion == NULL){ //SI NO SE ESTA EJECUTANDO NINGUN PROCESO,
							sem_post(&sem_replanificar); // Y EL UNICO QUE ESTA EN LA COLA DE LISTOS ES EL QUE SE ACABA DE INICIALIZAR LUEGO DEL DUMMY, REPLANIFICO PARA QUE LO EJECUTE
						}
					}
					if ((*paquete).resultado == 0){  //no se pudo abrir correctamente el archivo
						log_info(logger,"Operacion abrir no se pudo realizar correctamente.. enviando proceso a cola de exit");
						list_add(procesos_terminados,procesoAcargar) ;
					}
	//				procesoEnEjecucion->estado = finalizado ;
	//				procesoEnEjecucion=NULL;
	//				sem_post(&sem_CPU_ejecutoUnaSentencia);
	//				sem_post(&sem_replanificar);
				}
				else {  //era un abrir dentro del codigo escriptorio
					idBuscar = (*paquete).idProcesoSolicitante;
					ProcesoBloqueado *procesoADesbloquear = (ProcesoBloqueado*) list_find(procesos_bloqueados,&procesoBloqueadoEsIdABuscar);
					list_remove_by_condition(procesos_bloqueados,&procesoBloqueadoEsIdABuscar);
					DatosArchivo *archivoAsociado = malloc(sizeof(DatosArchivo));
					(*archivoAsociado).nombre = string_new();
					string_append(&((*archivoAsociado).nombre), (*paquete).archivoAsociado.nombre);
					(*archivoAsociado).direccionContenido = string_new();
					string_append(&((*archivoAsociado).direccionContenido), (*paquete).archivoAsociado.direccionContenido);
					(*archivoAsociado).numeroPagina = (*paquete).archivoAsociado.numeroPagina;
					(*archivoAsociado).numeroSegmento = (*paquete).archivoAsociado.numeroSegmento;
					list_add(procesoADesbloquear->proceso->archivosAsociados,archivoAsociado);
					t_config *config=config_create(pathSAFA);
					char*algoritmo= config_get_string_value(config, "Algoritmo");
					if(!strcmp(algoritmo,"VRR")){
						list_add(procesos_listos_MayorPrioridad,procesoADesbloquear);
					}
					else
					{
						list_add(procesos_listos,procesoADesbloquear->proceso);
					}
					config_destroy(config);
				}
			break;
			case 6:   //Porque es una operacion de flush
				idBuscar = (*paquete).idProcesoSolicitante;
				proceso = (ProcesoBloqueado*) list_find(procesos_bloqueados,&procesoBloqueadoEsIdABuscar);
				list_remove_by_condition(procesos_bloqueados,&procesoBloqueadoEsIdABuscar);
				if((*paquete).resultado == 1){
					t_config *config=config_create(pathSAFA);
					char*algoritmo= config_get_string_value(config, "Algoritmo");
					if(!strcmp(algoritmo,"VRR")){
						list_add(procesos_listos_MayorPrioridad,proceso);
					}
					else
					{
						list_add(procesos_listos,proceso->proceso);
					}
					config_destroy(config);
				}
				if ((*paquete).resultado == 0){  //no se pudo flushear correctamente el archivo
					log_info(logger,"Operacion flush no se pudo realizar correctamente.. enviando proceso a cola de exit");
					list_add(procesos_terminados,proceso) ;
				}
			break;
			case 8: //Porque es una operacion de crear
				idBuscar = (*paquete).idProcesoSolicitante;
				proceso = (ProcesoBloqueado*) list_find(procesos_bloqueados,&procesoBloqueadoEsIdABuscar);
				list_remove_by_condition(procesos_bloqueados,&procesoBloqueadoEsIdABuscar);
				if((*paquete).resultado == 1){
					log_info(logger,"Me aviso DAM que el archivo se creo bien");
					t_config *config=config_create(pathSAFA);
					char*algoritmo= config_get_string_value(config, "Algoritmo");
					if(!strcmp(algoritmo,"VRR")){
						list_add(procesos_listos_MayorPrioridad,proceso);
					}
					else
					{
						list_add(procesos_listos,proceso->proceso);
					}
					config_destroy(config);
				}
				if ((*paquete).resultado == 0){  //no se pudo crear correctamente el archivo
					log_info(logger,"Operacion crear no se pudo realizar correctamente.. enviando proceso a cola de exit");
					list_add(procesos_terminados,proceso) ;
				}
				if(list_size(procesos_listos) == 1 && procesoEnEjecucion == NULL){ //SI NO SE ESTA EJECUTANDO NINGUN PROCESO,
					sem_post(&sem_replanificar); // Y EL UNICO QUE ESTA EN LA COLA DE LISTOS ES EL QUE SE ACABA DE INICIALIZAR LUEGO DEL DUMMY, REPLANIFICO PARA QUE LO EJECUTE
					sem_post(&sem_CPU_ejecutoUnaSentencia);
				}
			break;
			case 9: //Porque es una operacion de borrar
				idBuscar = (*paquete).idProcesoSolicitante;
				proceso = (ProcesoBloqueado*) list_find(procesos_bloqueados,&procesoBloqueadoEsIdABuscar);
				list_remove_by_condition(procesos_bloqueados,&procesoBloqueadoEsIdABuscar);
				if((*paquete).resultado == 1){
					t_config *config=config_create(pathSAFA);
					char*algoritmo= config_get_string_value(config, "Algoritmo");
					if(!strcmp(algoritmo,"VRR")){
						list_add(procesos_listos_MayorPrioridad,proceso);
					}
					else
					{
						list_add(procesos_listos,proceso->proceso);
					}
					config_destroy(config);
				}
				if ((*paquete).resultado == 0){  //no se pudo borrar correctamente el archivo
					log_info(logger,"Operacion borrar no se pudo realizar correctamente.. enviando proceso a cola de exit");
					list_add(procesos_terminados,proceso) ;
				}
			break;
		}

	}

	return 0;
}


int atenderConexiones() {
	int sockSAFA;
	sockSAFA=crearConexionServidor(pathSAFA);
	if (listen(sockSAFA, 10) == -1){ //ESCUCHA!
		    log_error(logger, "No se pudo escuchar");
			log_destroy(logger);
			return -1;
	}
//	fcntl(sockSAFA, F_SETFL, O_NONBLOCK);  //para hacer noBlockeante el sockSAFA
	log_info(logger, "Se esta escuchando");
  int i=0;
  char* buff;
	pthread_t hiloCPU;
	pthread_t hiloDAM;
	pthread_t* hilosCPUs = NULL;
	int tipoCliente,sockCliente;
	int flag1 = 0;
  while((sockCliente = accept(sockSAFA, NULL, NULL))) {
		if (sockCliente == -1) {
		  if (errno == EWOULDBLOCK) {
//			printf("No pending connections; sleeping for one second.\n");
//			sleep(1);
		  } else {
			perror("error when accepting connection");
			return -1;
		  }
		} else {
		  i++;
		  buff = malloc(2);
		  recv(sockCliente,buff,2,0);
		  log_info(logger, "Llego una nueva conexion");
		  if(buff[0] == '1'){
			  log_info(logger, "socket cpu: %d ",sockCliente); //dps borrar
			  ConexionCPU *nuevaCPU = malloc(sizeof(ConexionCPU));
			  (*nuevaCPU).tieneDTBasociado=0;
			  (*nuevaCPU).socketCPU=sockCliente;
//			  list_add_in_index(cpu_conectadas, sockCliente, nuevaCPU);//probar si funciona
		      list_add(cpu_conectadas,nuevaCPU);
//			  sockCPU = sockCliente; //dps borrar
			  hilosCPUs = realloc(hilosCPUs,sizeof(pthread_t)*(cantidadDeCPUs+1));
			  pthread_create(&(hilosCPUs[cantidadDeCPUs]),NULL,conexionCPU,(void*)&sockCliente);
			  cantidadDeCPUs++;
			  if(flag1 == 1){
				  sem_post(&sem_estadoOperativo);
			  }
		  }
		  else if(buff[0] == '2'){
			  pthread_create(&hiloDAM,NULL,conexionDAM,(void*)&sockCliente);
			  sockDAM = sockCliente;
			  flag1 = 1;
			  if(cantidadDeCPUs >= 1){
				  sem_post(&sem_estadoOperativo);
			  }
		  }
		  free(buff);
//      send(sockCliente, msg, sizeof(msg), 0);
//      close(sockCliente);
    }
		sem_wait(&sem_esperaInicializacion);
  }
  sleep(1);
  return 1;
}

void* inicializarSAFA(){
	t_config *config=config_create(pathSAFA);
	sem_wait(&sem_estadoOperativo);
	log_info(logger,"SAFA ahora esta en estado operativo");
	flag_seEnvioSignalPlanificar=0;
	flag_quierenDesalojar=0;
	procesos_nuevos=list_create();
	procesos_listos=list_create();
	procesos_terminados=list_create();
	procesos_bloqueados=list_create();
	recursos = list_create();
	sem_init(&sem_liberarRecursos,0,1);
	sem_init(&sem_liberador,0,0);
	sem_init(&sem_replanificar,0,0);
	sem_init(&sem_procesoEnEjecucion,0,0);
	sem_init(&sem_esperaInicializacion,0,0);
	sem_init(&sem_CPU_ejecutoUnaSentencia,0,1);
	sem_init(&sem_finDeEjecucion,0,1);
	sem_init(&semCambioEstado,0,0);
	sem_init(&sem_pausar,0,1);
	sem_init(&sem_cargarProceso,0,0);
	//
	pthread_mutex_init(&mutex_pausa,NULL);
	idGlobal=1;
	bloquearDT = 0;
	//
	procesoEnEjecucion = NULL;
	Proceso*(*miAlgoritmo)();
	log_info(logger,"log error 1");
	char*algoritmo= config_get_string_value(config, "Algoritmo");
	pthread_t hilo_planificadorCortoPlazo;
	pthread_t liberadorRecursos;
	pthread_t hilo_ejecutarEsi;
	pthread_t hilo_consola;
	if(!strcmp(algoritmo,"FIFO")){
			miAlgoritmo=&fifo;
		flag_desalojo=0;
	}
	if(!strcmp(algoritmo,"RR")){
			miAlgoritmo=&round_robin;
			quantumRestante = config_get_int_value(config, "Quantum");
		}
	if(!strcmp(algoritmo,"VRR")){
			procesos_listos_MayorPrioridad = list_create();
			miAlgoritmo=&virtual_round_robin;
		}
	log_info(logger,"Se asigno el algoritmo %s",algoritmo);
	//	pthread_create(&liberadorRecursos,NULL,liberadorDeRecursos,NULL); proximamente..
	pthread_create(&hilo_planificadorCortoPlazo,NULL,planificadorCortoPlazo,(void *)miAlgoritmo);
	pthread_create(&hiloEjecucionProceso,NULL,ejecutarProceso,NULL);
	log_info(logger,"Se creo el hilo planificador CORTO PLAZO");

	int flags = fcntl(0, F_GETFL, 0);	//Para hacer no Blockeante el getline()
	fcntl(0, F_SETFL, flags | O_NONBLOCK);
	pthread_create(&hilo_consola,NULL,(void *)consola,NULL);
	log_info(logger,"Se creo hilo para la CONSOLA");
	config_destroy(config);
	while(1){} // DESPUES  VER SI HAY Q SACAR ESTE WHILE(1)
	return 0;
}

int main(int argc, char**argv)
{
	logger =log_create(logSAFA,"S-AFA",1, LOG_LEVEL_INFO);
	if(argc > 1){
	    	pathSAFA = argv[1];
	}
	cpu_conectadas = list_create();
	sem_init(&sem_estadoOperativo,0,0);
	pthread_t hilo_inicializaciones;
	pthread_create(&hilo_inicializaciones,NULL,(void *)inicializarSAFA,NULL);
	atenderConexiones();
	//cerrarPlanificador();
	return 0;
}

Proceso* fifo(){
	Proceso *proceso=list_get(procesos_listos,0);
	list_remove(procesos_listos,0);
	return proceso;
}

Proceso* round_robin(){
	Proceso *proceso=list_get(procesos_listos,0);
	list_remove(procesos_listos,0);
	return proceso;
}

Proceso* virtual_round_robin(){
	ProcesoBloqueado *proceso;
	if(list_is_empty(procesos_listos_MayorPrioridad))
	{
		proceso=list_get(procesos_listos,0);
		list_remove(procesos_listos,0);
	}
	else
	{
		proceso=list_get(procesos_listos_MayorPrioridad,0);
		list_remove(procesos_listos_MayorPrioridad,0);
	}
	quantumRestante = proceso->quantumPrioridad;
	return proceso;
}

void planificadorLargoPlazo(int sockCPU, char* pathEscriptorioCPU){

	Proceso* proceso= malloc(sizeof(Proceso));
	(*proceso).idProceso =idGlobal;
	(*proceso).socketProceso = sockCPU;
	(*proceso).estado=nuevo;
	(*proceso).flagInicializacion = 1;
	(*proceso).seEstaCargando = 0;
	(*proceso).pathEscriptorio = string_new();
	string_append(&((*proceso).pathEscriptorio), pathEscriptorioCPU);
	(*proceso).archivosAsociados = list_create();  //antes decia = NULL
	(*proceso).recursosRetenidos = list_create();
	log_info(logger, "Se asigno un nuevo dtb");
	//me fijo que tiene
	log_info(logger, "el path tienee %s con tam %d",(*proceso).pathEscriptorio, string_length((*proceso).pathEscriptorio) );//sacar
//	log_info(logger, "el tamaño de archivos asociados: %d ",list_size((*proceso).archivosAsociados) );//sacar
	list_add(procesos_nuevos, proceso);
	flag_nuevoProcesoEnListo = 1;
	idGlobal++;
	int hayProcesoEnExec = 0 ;
	if (procesoEnEjecucion){
		hayProcesoEnExec = 1;
	}
//	sem_post(&sem_CPU_ejecutoUnaSentencia);  //ESTE NO VA
	t_config *config=config_create(pathSAFA);
	if(list_size(procesos_listos) + list_size(procesos_bloqueados) + hayProcesoEnExec < config_get_int_value(config, "Multiprogramacion")) {
		log_info(logger,"Se dio senial para que arranque a crear el dtbdummy");
//		sem_post(&sem_cargarProceso);    //desbloquea funcion que crea el dtbDummy
	//	list_add(procesos_listos,proceso);
		cargarProcesoEnMemoria();
		}
	config_destroy(config);
}
//	free (*proceso);


//void *cargarProcesoEnMemoria(){
//	while(1)
//	{
//	do
//	{
void cargarProcesoEnMemoria(){
//		sem_wait(&sem_cargarProceso);
		int pos = 0;
		Proceso *procesoAcargar = (Proceso*) list_get(procesos_nuevos,pos);    //PLP planifica segun FIFO
		while((*procesoAcargar).seEstaCargando == 1){ 	//pero me tengo que fijar de no agarrar a un dtb que se este inicializando con su dummy
			log_info(logger, "eeeeeee se trabo");
			pos++;
			procesoAcargar = (Proceso*) list_get(procesos_nuevos,pos);//cuando lo elimina de procesos_nuevos? sigue en linea 608
		}
		pos = 0;    //reinicio variables para proxima busqueda
		//Proceso *procesoAcargar = (Proceso*) list_get(procesos_nuevos,pos);
		(*procesoAcargar).seEstaCargando = 1 ;

//		-----------------------------------------------------
//		creo que deberiamos hacer un remove de lista proc nuevos y luego Proceso *procesoDummy =procesoAcargar
//		-----------------------------------------------------

		Proceso *procesoDummy=malloc(sizeof(Proceso));//pq malloc? que pasa con *proceso
		(*procesoDummy).idProceso = (*procesoAcargar).idProceso ;
		(*procesoDummy).socketProceso = (*procesoAcargar).socketProceso;
		(*procesoDummy).estado=listo;
		(*procesoDummy).flagInicializacion = 0;  //indicador que es un DtbDummy
		(*procesoDummy).pathEscriptorio = string_new();
		string_append(&((*procesoDummy).pathEscriptorio), (*procesoAcargar).pathEscriptorio);
		procesoDummy->archivosAsociados = list_create();
		procesoDummy->recursosRetenidos = list_create();
//		(*procesoDummy).pathEscriptorio = (*procesoAcargar).pathEscriptorio;
		log_info(logger, "Se asigno un nuevo dtbDummy");
//		-----------------------------------------------------
//      y entonces aca el proceso agregado a la lista procesos_listos es el  proceso original y no una copia
//		-----------------------------------------------------
		list_add(procesos_listos,procesoDummy);  //ahora espero a que el PCP planifique al DTBdummy...
		sem_post(&sem_replanificar);
//	}
//	while(1);
}

bool procesoEsIdABuscar(void * proceso){
	Proceso *proc=(Proceso*) proceso;
	if((*proc).idProceso==idBuscar)

		return true;
	else
		return false;
}
bool procesoBloqueadoEsIdABuscar(void * proceso){
	ProcesoBloqueado *proc=(ProcesoBloqueado*) proceso;
	if((*proc).proceso->idProceso==idBuscar)
		return true;
	else
		return false;
}

bool cpuUsadaEsIdABuscar(void * cpu){
	ConexionCPU *cpuConectada=(ConexionCPU*) cpu;
	if((*cpuConectada).socketCPU==idCPUBuscar)
		return true;
	else
		return false;
}


void *planificadorCortoPlazo(void *miAlgoritmo){//como parametro le tengo que pasar la direccion de memoria de mi funcion algoritmo
//	t_log *log_planiCorto;
	// se tiene que ejecutar todo el tiempo en un hilo aparte
	while(1)
	{
//		while(list_size(procesos_listos) == 0){
			//espera
//		}
		sem_wait(&sem_replanificar);
		log_info(logger,"se obtuvo senial de sem planificador");
		ejecutarRetardo(pathSAFA); //Se ejecuta el retardo antes de planificacion
		Proceso*(*algoritmo)();
		algoritmo=(Proceso*(*)()) miAlgoritmo;
		// aca necesito sincronizar para que se ejecute solo cuando le den la segnal de replanificar
		//
		Proceso *proceso; // este es el proceso que va a pasar de la cola de ready a ejecucion
		proceso = (*algoritmo)();//habia que inicializar las variables en dtbdummy en que momento sale de lista_nuevo?

		if(proceso)
		{
			log_info(logger,"PCP selecciono el Proceso ID %d",(*proceso).idProceso);
	//		logTest("Esperando el semaforo sem fin de ejecucion");
	//		sem_wait(&sem_finDeEjecucion);
	//		logTest("Se paso el semaforo sem fin de ejecucion");
			(*proceso).estado=ejecucion;
			idBuscar = (*proceso).idProceso;
//			list_remove_by_condition(procesos_listos,&procesoEsIdABuscar); lo pongo dep que lo envia
			procesoEnEjecucion=proceso;

			enviarProceso((*procesoEnEjecucion).socketProceso, procesoEnEjecucion);//mando el DTB al CPU asociado
			list_remove_by_condition(procesos_listos,&procesoEsIdABuscar);
//			void* buff = serializarProceso(procesoEnEjecucion);
//			send((*procesoEnEjecucion).socketProceso,buff,sizeof(Proceso),0);
			log_info(logger,"Se asigno el proceso como proceso en ejecucion");
			flag_seEnvioSignalPlanificar=0;
			if((*procesoEnEjecucion).flagInicializacion == 1){
				sem_post(&sem_procesoEnEjecucion);  //para que habilite al hilo que ejecuta el proceso a seguir
			}
		}
	}
}


void *ejecutarProceso(){
	while(1){
		sem_wait(&sem_procesoEnEjecucion);
		log_info(logger,"se entro a ejecutar el proceso en ejecucion");
		while(procesoEnEjecucion && (*procesoEnEjecucion).estado==ejecucion){
//			sem_wait(&sem_liberarRecursos);
//			pthread_mutex_lock(&mutex_pausa);
			log_info(logger,"esperando semaforo de que la CPU ejecuto una sentencia");
			sem_wait(&sem_CPU_ejecutoUnaSentencia);
			if(procesoEnEjecucion){
				send((*procesoEnEjecucion).socketProceso,"1",2,0);// este habilita a la cpu para que ejecute 1 sentencia de su codigo escriptorio
				log_info(logger,"Se da orden  a la CPU de ejecutar el proceso: %d",(*procesoEnEjecucion).idProceso);
//				sem_post(&sem_liberarRecursos);
			}//EJECUCION
//			sem_wait(&semCambioEstado);
//			pthread_mutex_unlock(&mutex_pausa);
		}
		//el proceso esi actual dejo de ser el que tiene que ejecutar
//		sem_post(&sem_finDeEjecucion);
		log_info(logger,"Se termino de ejecutar el CPU en cuestion");
	}

}

void finalizarProceso(char *idDT){
	int flag = 0;
	if ((procesoEnEjecucion != NULL) && (procesoEnEjecucion->idProceso == atoi(idDT))){
		bloquearDT = 1;
		flag = 2;
	}
	else
	{
		idBuscar = atoi(idDT);
		Proceso *procesoAfinalizar = (Proceso*) list_find(procesos_nuevos,&procesoEsIdABuscar);
		if (procesoAfinalizar)
		{
			list_add(procesos_terminados,procesoAfinalizar);
			list_remove_by_condition(procesos_nuevos,&procesoEsIdABuscar);
			flag = 1;
		}
		Proceso *procesoAfinalizar2 = (Proceso*) list_find(procesos_listos,&procesoEsIdABuscar);
		if (procesoAfinalizar2)
		{
			liberarRecursos(procesoEnEjecucion);
			list_add(procesos_terminados,procesoAfinalizar2);
			list_remove_by_condition(procesos_listos,&procesoEsIdABuscar);
			flag = 1;
		}
		Proceso *procesoAfinalizar3 = (Proceso*) list_find(procesos_bloqueados,&procesoEsIdABuscar);
		if (procesoAfinalizar3)
		{
			liberarRecursos(procesoEnEjecucion);
			list_add(procesos_terminados,procesoAfinalizar3);
			list_remove_by_condition(procesos_bloqueados,&procesoEsIdABuscar);
			flag = 1;
		}
		if (flag == 0)
		{
			log_info(logger,"El IDBlock ingresado no existe en el sistema");
		}
		else if (flag == 1)
		{
			log_info(logger,"El IDBlock ingresado fue finalizado exitosamente");
		}
	}

}

void statusProceso(char *idDT){
	int flag = 0;
	idBuscar = atoi(idDT);
	Proceso *procesoStatus = (Proceso*) list_find(procesos_nuevos,&procesoEsIdABuscar);
	if (procesoStatus)
	{
		mostrarDatosDTBlock(procesoStatus);
		flag = 1;
	}
	Proceso *procesoStatus2 = (Proceso*) list_find(procesos_listos,&procesoEsIdABuscar);
	if (procesoStatus2)
	{
		mostrarDatosDTBlock(procesoStatus2);
		flag = 1;
	}
	Proceso *procesoStatus3 = (Proceso*) list_find(procesos_bloqueados,&procesoEsIdABuscar);
	if (procesoStatus3)
	{
		mostrarDatosDTBlock(procesoStatus3);
		flag = 1;
	}
	Proceso *procesoStatus4 = (Proceso*) list_find(procesos_terminados,&procesoEsIdABuscar);
	if (procesoStatus4)
	{
		mostrarDatosDTBlock(procesoStatus4);
		flag = 1;
	}
	if (flag == 0)
	{
		log_info(logger,"El IDBlock ingresado no existe en el sistema");
	}


}

void mostrarDatosDTBlock(Proceso *procesoStatus)
{
	log_info(logger,"Datos DTBlock:");
	log_info(logger,"ID: %d",procesoStatus->idProceso);
	log_info(logger,"Path Escriptorio: %s",procesoStatus->pathEscriptorio);
	log_info(logger,"Socket CPU asociado: %d",procesoStatus->socketProceso);
	log_info(logger,"Flag Inicializacion: %d",procesoStatus->flagInicializacion);
	switch(procesoStatus->estado){
		case ejecucion:
		log_info(logger,"Estado: En Ejecucion");
		break;
		case nuevo:
		log_info(logger,"Estado: Nuevo");
		break;
		case bloqueado:
		log_info(logger,"Estado: Bloqueado");
		break;
		case listo:
		log_info(logger,"Estado: Listo");
		break;
		case finalizado:
		log_info(logger,"Estado: Finalizado");
		break;
	}
	log_info(logger,"Cantidad Archivos Abiertos: %d",list_size(procesoStatus->archivosAsociados));
	if(list_size(procesoStatus->archivosAsociados) > 0){
		 list_iterate(procesoStatus->archivosAsociados, &mostrarInfoArchivoAsociado);
	}
	log_info(logger,"Cantidad de Recursos Retenidos diferentes: %d",list_size(procesoStatus->recursosRetenidos));
	if(list_size(procesoStatus->recursosRetenidos) > 0){
		 list_iterate(procesoStatus->recursosRetenidos, &mostrarInfoRecursoRetenido);
	}



}

void mostrarInfoArchivoAsociado(DatosArchivo * archivoAsociado){
	log_info(logger,"Archivo Abierto:");
	log_info(logger,"Nombre: %s",archivoAsociado->nombre);
//	log_info(logger,"Direccion en memoria: %s",archivoAsociado->direccionContenido); //esto no se si hay q mostrarlo
}

void mostrarInfoRecursoRetenido(Recurso *recurso){
	log_info(logger,"Recurso Retenido:");
	log_info(logger,"Nombre: %s",recurso->nombre);
	log_info(logger,"Cantidad Instancias: %d",recurso->instancias);
}

void statusColasSistema(){
	log_info(logger,"Datos de Cada Cola del Sistema:");
	log_info(logger,"Cola procesos nuevos:");
	log_info(logger,"Cantidad DTBlocks en la cola: %d",list_size(procesos_nuevos));
	list_iterate(procesos_nuevos, &mostrarDatosDTBlock);
	log_info(logger,"Cola procesos listos:");
	log_info(logger,"Cantidad DTBlocks en la cola: %d",list_size(procesos_listos));
	list_iterate(procesos_listos, &mostrarDatosDTBlock);
	log_info(logger,"Cola procesos bloqueados:");
	log_info(logger,"Cantidad DTBlocks en la cola: %d",list_size(procesos_bloqueados));
	list_iterate(procesos_bloqueados, &mostrarDatosDTBlock);
	log_info(logger,"Cola procesos terminados:");
	log_info(logger,"Cantidad DTBlocks en la cola: %d",list_size(procesos_terminados));
	list_iterate(procesos_terminados, &mostrarDatosDTBlock);
	if(procesoEnEjecucion){
		log_info(logger,"DTBlock en ejecucion:");
		mostrarDatosDTBlock(procesoEnEjecucion);
	}
}

bool existeRecurso(void *recurso){
	Recurso *b=(Recurso*) recurso;
	if (strcmp ((*b).nombre , recursoABuscar) == 0){ //son iguales
		return true;
	}
	else
		return false;
}

bool verificarSiFinalizarElProcesoEnEjecucion(int quantum)
{
	if(bloquearDT == 1){
		   list_add(procesos_terminados,procesoEnEjecucion);
		   procesoEnEjecucion=NULL;
		   bloquearDT = 0;
		   quantumRestante = quantum;
		   liberarRecursos(procesoEnEjecucion);
		   log_info(logger,"El IDBlock en ejecucion fue finalizado por orden de la consola del SAFA");
		   sem_post(&sem_replanificar);  //S-AFA debe replanificar
		   return true;
	}
	else
	{
		return false;
	}
}
void analizarFinEjecucionLineaProceso(int quantum)
{
	flag_quierenDesalojar=0;
    quantumRestante--;
    verificarSiFinalizarElProcesoEnEjecucion(quantum);
    if(quantumRestante == 0){ //desaloja por fin de quantum
	   procesoEnEjecucion->estado = listo;
	   list_add(procesos_listos,procesoEnEjecucion); //pasa al final de la cola de listos
	   procesoEnEjecucion=NULL;
	   quantumRestante = quantum;
//	   if(list_size(procesos_listos) + list_size(procesos_bloqueados) < config_get_int_value(config, "Grado de multiprogramacion")) { //chequeo si puedo poner otro proceso en ready
//			sem_post(&sem_cargarProceso);    //desbloquea funcion que crea el dtbDummy
//	   }
	   sem_post(&sem_replanificar);  //S-AFA debe replanificar
    }
    sem_post(&sem_CPU_ejecutoUnaSentencia);

}

void liberarRecursos(Proceso *proceso)
{
	 list_iterate(proceso->recursosRetenidos, &liberarRecurso);
	 log_info(logger,"Se liberaron todos los recursos del proceso");
}

void liberarRecurso(Recurso *recursoAliberar)
{
	 strcpy(recursoABuscar,(*recursoAliberar).nombre);
	 Recurso *recurso = (Recurso*) list_find(recursos,&existeRecurso);
	 (*recurso).instancias = (*recurso).instancias + (*recursoAliberar).instancias;
	 for(int i = 0; i < (*recursoAliberar).instancias; i++){
		 Proceso *procesoADesbloquear = (Proceso*) list_remove((*recurso).procesosEnEspera,i);
		 idBuscar = (*procesoADesbloquear).idProceso;
		 procesoADesbloquear = (Proceso*) list_find(procesos_bloqueados,&procesoEsIdABuscar);
		 list_remove_by_condition(procesos_bloqueados,&procesoEsIdABuscar);
		 list_add(procesos_listos,procesoADesbloquear);
	 }
}

bool esArchivoBuscado(void *archivo){
	DatosArchivo *b=(DatosArchivo*) archivo;
	if (strcmp ((*b).nombre , archivoAbuscar) == 0){ //son iguales
		return true;
	}
	else
		return false;
}


/*
 *  en la funcion que  manda a ejecutar el proceso
 cuando termina pegar esto:
 	if(list_size(procesosEnEspera)>0){
			Proceso *procesoQueEstabaEsperando=malloc(sizeof(Proceso));
			procesoQueEstabaEsperando = list_remove(procesosEnEspera,0);  //agarro el primero que estaba esperando y lo pongo en ready
			list_add(procesos_listos,procesoQueEstabaEsperando);
		}
		//  para que agarre al primero de la lista de espera y lo ponga en ready
//		Falta ver lo del grado de multiprogramacion mejor, y que el SDtbdummy no tiene que contar para eso

 */

//
//
//int recibirPaqueteDAM(int socketSender, Paquete_DAM * paqueteDAM){
//
//	void* buffer = malloc(20);// es el tamaño de Paquete_DAM sin los char* = 4*3(int) + 2*4 int del DatosArchivo
//	int flag = recv(socketSender,buffer,20,0);
//
//	if(flag != -1){
//
//		int nombreTam;
//		int direccionContenidoTam;
//
//		memcpy(&(paqueteDAM->tipoOperacion), buffer ,4 );
//		memcpy(&(paqueteDAM->resultado),buffer + 4, 4 );
//		memcpy(&(paqueteDAM->idProcesoSolicitante),buffer + 2 * 4 ,4 );
//		memcpy(&(paqueteDAM->archivoAsociado.nombre_tam),buffer + 3 * 4 ,4 );
//		memcpy(&(paqueteDAM->archivoAsociado.direccionContenido_tam),buffer + 4 * 4 , 4 );
//
//		nombreTam = paqueteDAM->archivoAsociado.nombre_tam;
//		direccionContenidoTam = paqueteDAM->archivoAsociado.direccionContenido_tam;
//
//		if(nombreTam){
//
//			buffer = realloc(buffer,nombreTam);
//			recv(socketSender,buffer,nombreTam,0);
//			paqueteDAM->archivoAsociado.nombre = malloc(nombreTam);
//			memcpy(paqueteDAM->archivoAsociado.nombre,buffer,nombreTam );
//		}
//
//		if(direccionContenidoTam){
//
//			buffer = realloc(buffer,direccionContenidoTam);
//			recv(socketSender,buffer,direccionContenidoTam,0);
//			paqueteDAM->archivoAsociado.direccionContenido = malloc(direccionContenidoTam);
//			memcpy(paqueteDAM->archivoAsociado.direccionContenido,buffer,direccionContenidoTam );
//		}
//
//
//	}
//
//	free(buffer);
//
//	return flag;
//
//}
//
//
//
//
//

