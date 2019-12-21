#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <dirent.h>
#include <stdio.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <commons/bitarray.h>
#include "FuncionesConexiones.h"
#include <readline/readline.h>
#include <readline/history.h>
#include "MDJ.h"

int main(void)
{
//	inicializarVariables();
	configMDJ = config_create(pathMDJ);
//	retardo = config_get_int_value(configMDJ, "Retardo");
	puntoMontaje = config_get_string_value(configMDJ, "PUNTO_MONTAJE");

	logger =log_create(logMDJ,"MDJ",1, LOG_LEVEL_INFO);



	int socketMDJ;
	char* buff= malloc(2);
	socketMDJ=crearConexionServidor(pathMDJ);
	if (listen(socketMDJ, 10) == -1){ //ESCUCHA!
			log_error(logger, "No se pudo escuchar");
			log_destroy(logger);
			return -1;
	}
	log_info(logger, "Se esta escuchando");
	socketDAM = accept(socketMDJ, NULL, NULL);
	if (socketDAM == -1) {
	  if (errno == EWOULDBLOCK) {
		log_info(logger,"No pending connections; sleeping for one second.\n");
		sleep(1);
	  } else {
		log_info(logger,"error when accepting connection");
		exit(1);
	  }
	} else {
	  recv(socketDAM,buff,2,0);
	  log_info(logger, "Llego una nueva conexion");
	  if(buff[0] == '2'){
		  log_info(logger,"Se realizo correctamente la conexion con DAM");
	  }

	}

	inicializarVariables();
//	testeoMDJ();
//	munmap(map,sizeBitmap);
    atenderDAM();
	close(socketDAM);
	close(socketMDJ);
	return EXIT_SUCCESS;
}

void inicializarVariables(){

////	prueba
//	   puntoMontaje =string_new();
//	   string_append(&puntoMontaje, "/home/utnso/fifa-examples/fifa-checkpoint/");
//
////	   \prueba
		rutaBloque = string_new();
//		rutaArchivoActual = string_new();
		string_append(&rutaBloque, puntoMontaje);
		string_append(&rutaBloque, "Bloques/");
		///////


		char* rutaMetadata = string_new();
		string_append(&rutaMetadata, puntoMontaje);
		string_append(&rutaMetadata, "Metadata/Metadata.bin");	//fifa-examples/fifa-checkpoint/Metadata/Metadata.bin
		t_config *configArchivo = config_create(rutaMetadata);
		tamanioBloque = config_get_int_value(configArchivo, "TAMANIO_BLOQUES");
		cantidadBloques = config_get_int_value(configArchivo, "CANTIDAD_BLOQUES");

//		log_info(logger,"Creando bitmap..");
		printf("Creando bitmap..");
		char* rutaBitmap = string_new();
		string_append(&rutaBitmap, puntoMontaje);
		string_append(&rutaBitmap, "Metadata/Bitmap.bin");
		printf("%s\n",rutaBitmap);//sacar
//		char *bitarray = string_new();
//		char *bitarray ;
//		int desc = open(rutaBitmap, O_RDWR | O_CREAT | O_TRUNC, 0777);
		int desc = open(rutaBitmap,O_RDWR);
//		int desc =fopen(rutaBitmap, "Wb");
//		sizeBitmap = lseek(desc, 0, SEEK_END);
		sizeBitmap =cantidadBloques/8;
		ftruncate(desc,sizeBitmap);// el metadata no va a aumentar su tamaño
		map = mmap(NULL,sizeBitmap,PROT_READ |PROT_WRITE,MAP_SHARED,desc,0);
		bitmap = bitarray_create_with_mode(map,sizeBitmap,MSB_FIRST); //PREGUNTAR SI ESTA BIEN INICIALIZADO EL BITMAP

		while(sizeBitmap){
//			printf("valor del bit %d\n", bitarray_test_bit(bitmap,sizeBitmap));
			log_info(logger,"valor del bit %d", bitarray_test_bit(bitmap,sizeBitmap));
			sizeBitmap--;

		}



}

void atenderDAM()
{
	Paquete_DAM *paqueteDAM = inicializarPaqueteDam();

	t_config *configDAM = config_create(pathDAM);
	int transferSize = config_get_int_value(configDAM, "TRANSFER_SIZE");

	int partes,resultadoOperacion,size,flag,cantBytes;
	char* codigoEscriptorio;
	char ** lineas ;
	log_info(logger,"Atendiendo DAM...");
	while(1)
	{
		recibirPaqueteDAM(socketDAM,paqueteDAM);
		paqueteDAM->archivoAsociado.nombre = string_new();
		string_append(&paqueteDAM->archivoAsociado.nombre,paqueteDAM->archivoAsociado.direccionContenido);
		ejecutarRetardo(pathMDJ); //Se ejecuta el retardo antes de cada operacion
		log_info(logger,"tipoOperacion: %d",paqueteDAM->tipoOperacion);
		switch(paqueteDAM->tipoOperacion)
		{
			case 1: //abrir
				log_info(logger,"Entre al abrir..");
				log_info(logger,"path: %s",paqueteDAM->archivoAsociado.direccionContenido);
				flag = validarArchivo(paqueteDAM->archivoAsociado.nombre, &size); //falta poner errores 30004
				codigoEscriptorio = obtenerDatosArchivo(paqueteDAM->archivoAsociado.nombre,0,size);
				log_info(logger,"Codigo archivo: %s", codigoEscriptorio);
				send(socketDAM,&flag,sizeof(int),0);
				if(flag==1) //encontro el archivo
				{
					partes = size/transferSize;
					if(size % transferSize != 0){
						partes++;
					}
					log_info(logger,"Partes: %d", partes);
					send(socketDAM,&partes,sizeof(int),0);
//					char* aux = malloc(transferSize*sizeof(char));
					char* aux = string_new();
					int i = 0 ;
					while(partes > 0)
					{
							if(partes == 1)
							{
								int resto = size % transferSize ;
								if(resto != 0)
								{
//									char* aux2 = malloc(resto*sizeof(char));
									char* aux2 = string_new();
									send(socketDAM,&resto,sizeof(int),0);
//									memcpy(&aux2,codigoEscriptorio+ i*transferSize,resto);
									aux2 = string_substring(codigoEscriptorio,i*transferSize,resto);
									send(socketDAM,aux2,resto,0);
									free(aux2);
								}
								else
								{
									send(socketDAM,&transferSize,sizeof(int),0);
//									memcpy(&aux,codigoEscriptorio+ i*transferSize,transferSize);
									aux = string_substring(codigoEscriptorio,i*transferSize,transferSize);
									send(socketDAM,aux,transferSize,0);
								}
							}
							else
							{
//								memcpy(&aux,codigoEscriptorio+ i*transferSize,transferSize);
								aux = string_substring(codigoEscriptorio,i*transferSize,transferSize);
								send(socketDAM,aux,transferSize,0);
							}
							partes--;
							i++;
							log_info(logger,"Partes restantes..: %d", partes);
					  }
//					  free(aux);
//					send(socketDAM,codigoEscriptorio,size,0);
				}
			break;
			case 6: //flush  //falta mostrar error 30003
				recv(socketDAM,&partes,sizeof(int),0);
				char* buffer = malloc(transferSize+1);
				char* escriptorio = string_new();
				while( partes > 0)
				{
					recv(socketDAM,buffer,transferSize,0);
					partes--;
					buffer[transferSize] = '\0' ;
					string_append(&escriptorio,buffer);
					//cambiar para q funcione en caso de q el tamaño del escriptorio no sea multiplo del transfersize
				}
				resultadoOperacion = guardarDatosArchivo(paqueteDAM->archivoAsociado.nombre, 0, size, escriptorio);
				send(socketDAM,&resultadoOperacion,sizeof(int),0);
			break;
			case 8: //crear
				log_info(logger,"Creando Archivo");
				lineas = string_split(paqueteDAM->archivoAsociado.nombre, " ");
				cantBytes = atoi(lineas[1]);
				log_info(logger,"lineas: %d ", cantBytes);
				resultadoOperacion = crearArchivo(lineas[0],cantBytes);
				send(socketDAM,&resultadoOperacion,sizeof(int),0);
			break;
			case 9: //borrar
				resultadoOperacion = borrarArchivo(paqueteDAM->archivoAsociado.nombre);
				send(socketDAM,&resultadoOperacion,sizeof(int),0);
			break;

		}

	}
	config_destroy(configMDJ);
	config_destroy(configDAM);
}

int validarArchivo(char* nombreArchivo, int* size)
{
	t_config *configMDJ = config_create(pathMDJ);
	logger = log_create(logMDJ,"MDJ",1, LOG_LEVEL_INFO);
	log_info(logger,"Validando archivo..");
	log_info(logger,"path: %s",nombreArchivo);
	char* pathArchivo = string_new();
	string_append(&pathArchivo, puntoMontaje);
	string_append(&pathArchivo, "Archivos");
	string_append(&pathArchivo, nombreArchivo);
	log_info(logger,"path: %s",pathArchivo);
	FILE *archivo= fopen(pathArchivo,"r");
	if(archivo == NULL)
	{
		log_info(logger, "Error 30004: El archivo no existe");
		return 0; //archivo inexistente
	}
	else
	{
		log_info(logger, "Eeeeee");
		t_config *configArchivo = config_create(pathArchivo);  // /fifa-examples/fifa-checkpoint/Archivos/scripts/
		(*size) = config_get_int_value(configArchivo, "TAMANIO");
		log_info(logger, "oooo");
		config_destroy(configArchivo);
		config_destroy(configMDJ);
		fclose(archivo);
	}
	log_info(logger,"Archivo validado exitosamente");
	return 1; //archivo existente
}

char* obtenerDatosArchivo(char* nombreArchivo ,int offset, int size)
{
	t_config *configMDJ = config_create(pathMDJ);
	logger = log_create(logMDJ,"MDJ",1, LOG_LEVEL_INFO);
	log_info(logger,"Obteniendo datos archivo...");
	char* pathArchivo = string_new();
	string_append(&pathArchivo, puntoMontaje);
	string_append(&pathArchivo, "Archivos");
	string_append(&pathArchivo, nombreArchivo);
	t_config *configArchivo = config_create(pathArchivo);  // /fifa-examples/fifa-checkpoint/Archivos/scripts/
	char** bloques = config_get_array_value(configArchivo, "BLOQUES");

	/*
	int l = 0;
	while(bloques[l] != NULL)
	{
		log_info(logger,"Obteniendo datos archivo... %s", bloques[l]);
		l++;
	}
	*/

//	char* pathArchivo2 = string_new();
//	string_append(&pathArchivo2, puntoMontaje);
//	string_append(&pathArchivo2, "Metadata/Metadata.bin");	//fifa-examples/fifa-checkpoint/Metadata/Metadata.bin
//	t_config *configArchivo = config_create(pathArchivo2);
//	int tamanioBloque = config_get_int_value(configArchivo, "TAMANIO_BLOQUES");
//	int bloqueInicio = size/tamanioBloque;
//	int offsetBloque = size%tamanioBloque;
	int i = 0;
	char* codigoArchivo = string_new();
//	char* aux = malloc(tamanioBloque);
	log_info(logger,"Copiando info de cada bloque..");
	while(bloques[i] != NULL)
	{
		char* pathBloque = string_new();
		string_append(&pathBloque, puntoMontaje);
		string_append(&pathBloque, "Bloques/");  //    fifa-examples/fifa-checkpoint/Bloques/
		string_append(&pathBloque, bloques[i]);
		string_append(&pathBloque, ".bin");   //   fifa-examples/fifa-checkpoint/Bloques/0.bin
//		log_info(logger,"Obteniendo datos archivo... %s", pathBloque);
		FILE *archivo= fopen(pathBloque,"rb");
		fseek(archivo, 0, SEEK_END);
		int tam = ftell(archivo);
		char* aux = malloc(tam+1);
		fseek(archivo, 0, SEEK_SET);
		fread(aux,tam,1,archivo);
		aux[tam] = '\0';
		log_info(logger," datos archivo... %s", aux);
		string_append(&codigoArchivo, aux);
		free(aux);
		fclose(archivo);
		i++;
	}
	log_info(logger,"Carga de datos de archivo exitosa");
	char* codigoPedido = string_substring(codigoArchivo, offset, size);
	return codigoPedido;
}

int borrarArchivo(char* nombreArchivo)
{
//	logger =log_create(logMDJ,"MDJ",1, LOG_LEVEL_INFO);
//	t_config *configMDJ = config_create(pathMDJ);
//	char* puntoMontaje = config_get_string_value(configMDJ, "PUNTO_MONTAJE");
	int size;
	if(validarArchivo(nombreArchivo,&size) == 0)
	{
		log_info(logger,"Error 30004: Archivo Inexistente");
		return 0 ;
	}
	char* pathArchivo = string_new();
	log_info(logger,"borrando Archivo...");
	string_append(&pathArchivo, puntoMontaje);
	string_append(&pathArchivo, "Archivos");
	string_append(&pathArchivo, nombreArchivo);
	t_config *configArchivo = config_create(pathArchivo);  // /fifa-examples/fifa-checkpoint/Archivos/scripts/
	char** bloques = config_get_array_value(configArchivo, "BLOQUES");
	log_info(logger,"verifico bloques...");
//	char* pathArchivo2 = string_new();
//	string_append(&pathArchivo2, puntoMontaje);
//	string_append(&pathArchivo2, "Metadata/Metadata.bin");	//fifa-examples/fifa-checkpoint/Metadata/Metadata.bin
//
////	int cantidadBloques = config_get_int_value(configArchivo, "CANTIDAD_BLOQUES");
////	char *bitarray = string_new();
//	t_bitarray *bitmap = bitarray_create_with_mode(bitarray,cantidadBloques/8,LSB_FIRST); //PREGUNTAR SI ESTA BIEN INICIALIZADO EL BITMAP
	for(int j = 0 ; bloques[j] != NULL;j++ )
	{
		bitarray_clean_bit(bitmap,atoi(bloques[j])) ;
	}
	log_info(logger,"Antes de eliminar file...");
	int ret = remove(pathArchivo);

	if(ret == 0) {
		log_info(logger,"File deleted successfully");
	} else {
		log_info(logger,"Error 30004: Archivo no encontrado");
		return 0;
	}
	return 1 ;
	/*int i = 0;
	int ret2 = 0 ;
	char* codigoArchivo = string_new();

	while(bloques[i] != NULL)
	{
		char* pathBloque = string_new();
		string_append(&pathBloque, puntoMontaje);
		string_append(&pathBloque, "Bloques/");  //    fifa-examples/fifa-checkpoint/Bloques/
		string_append(&pathBloque, bloques[i]);
		string_append(&pathBloque, ".bin");   //   fifa-examples/fifa-checkpoint/Bloques/0.bin
		ret2 = remove(pathBloque);
	    if(ret2 == 0) {
	    	log_info(logger,"File deleted successfully");
		} else {
			log_info(logger,"Error 30004: Archivo no encontrado");
		}
		i++;
	} */


}

int crearArchivo(char* nombreArchivo,int cantBytes)
{
	logger =log_create(logMDJ,"MDJ",1, LOG_LEVEL_INFO);
	int aux ;
	log_info(logger,"Validando Archivo...");
	if(validarArchivo(nombreArchivo,&aux) == 1)
	{
		log_info(logger,"Error 50001: Archivo ya existente");
		return 0 ;
	}
//	t_config *configMDJ = config_create(pathMDJ);
//	char* puntoMontaje = config_get_string_value(configMDJ, "PUNTO_MONTAJE");
//	char* pathArchivo2 = string_new();
//	string_append(&pathArchivo2, puntoMontaje);
//	string_append(&pathArchivo2, "Metadata/Metadata.bin");	//fifa-examples/fifa-checkpoint/Metadata/Metadata.bin
//	t_config *configArchivo = config_create(pathArchivo2);
//	int tamanioBloque = config_get_int_value(configArchivo, "TAMANIO_BLOQUES");
//	int cantidadBloquesTotales = config_get_int_value(configArchivo, "CANTIDAD_BLOQUES");
	int cantidadBloquesTotales = cantidadBloques;
	log_info(logger,"Calculando cuantos bloques se necesitan...");
	int cantidadBloquesNecesarios = cantBytes/tamanioBloque;
	if(cantBytes%tamanioBloque != 0)
	{
		cantidadBloquesNecesarios++;
	}
	log_info(logger,"cantidadBloquesNecesarios: %d" , cantidadBloquesNecesarios);
//	int tamanioArchivo = cantidadBloquesNecesarios*tamanioBloque+1; SEGURO Q NO VA ESTO, DPS LO BORRAMOS
	t_list *listaBloques = list_create();
//	char *bitarray = string_new();
//	t_bitarray *bitmap = bitarray_create_with_mode(bitarray,cantidadBloquesNecesarios/8,LSB_FIRST); //PREGUNTAR SI ESTA BIEN INICIALIZADO EL BITMAP
	int cantBloquesLibres = 0 ;
	for(int i = 0 ; i < cantidadBloquesTotales;i++)
	{
		if(bitarray_test_bit(bitmap,i) == false)
		{
			cantBloquesLibres++;
		}
	}
	if(cantBloquesLibres < cantidadBloquesNecesarios)
	{
		log_info(logger,"Error 50002: Espacio Insuficiente");
		return 0 ;
	}
	log_info(logger,"Buscando bloques libres...");
	int i = 0;
	int t = 0 ;
	while(t < cantidadBloquesNecesarios)
	{
		if(bitarray_test_bit(bitmap,i) == false)
		{
			int *numeroBloque = malloc(sizeof (int));
			(*numeroBloque) = i;
			list_add(listaBloques,numeroBloque);
			bitarray_set_bit(bitmap, i);
			t++;
		}
		i++;
	}
	log_info(logger,"eee medio crear");
//	t_config *configMDJ = config_create(pathDAM);
//	char* puntoMontaje = config_get_string_value(configMDJ, "PUNTO_MONTAJE");
	char* pathArchivo = string_new();
	string_append(&pathArchivo, puntoMontaje);
	string_append(&pathArchivo, "Archivos");
	string_append(&pathArchivo, nombreArchivo);
	FILE * archivoNuevo = fopen(pathArchivo,"w");
	log_info(logger,"path fopen.. : %s ", pathArchivo);
	char* info = string_new();
	log_info(logger,"fopen crear w...");
	string_append(&info, "TAMANIO=");
	string_append(&info,string_itoa(cantBytes));
	string_append(&info, "\nBLOQUES=[");
	int m = 0;
	while(m < list_size(listaBloques))
	{
		int* numBloqueLibre = list_get(listaBloques,m);
//		char *numeroBloque = (char*) list_get(listaBloques,m);
		string_append(&info,string_itoa((*numBloqueLibre)));
		if(list_size(listaBloques) == m+1)
		{
			string_append(&info,"]");
		}
		else
		{
			string_append(&info,",");
		}
		m++;
	}
	string_append(&info, "\n");
	log_info(logger,"antes del write");
	fwrite(info,1,string_length(info)+1,archivoNuevo);
	log_info(logger,"dps del write");
	fclose(archivoNuevo);
	m = 0;
	while(m < list_size(listaBloques))
	{
		char* pathBloque = string_new();
		string_append(&pathBloque, puntoMontaje);
		string_append(&pathBloque, "Bloques/");  //    fifa-examples/fifa-checkpoint/Bloques/
		string_append(&pathBloque, list_get(listaBloques,m));
		string_append(&pathBloque, ".bin");   //   fifa-examples/fifa-checkpoint/Bloques/0.bin
		FILE *bloqueArchivo = fopen(pathBloque,"wb");
		for(int n=0 ; n < cantBytes; n++)
		{
			fseek(bloqueArchivo, 0, SEEK_END);
			fwrite("\n",1,2,bloqueArchivo);
		}
		m++;
		fclose(bloqueArchivo);
	}
	for(int j = 0 ; j< list_size(listaBloques);j++)
	{
		int* num = list_remove(listaBloques,j);
		free(num);

	}
	list_destroy(listaBloques);
	log_info(logger,"Fin de operacion crear...");
	return 1 ;
}

int guardarDatosArchivo(char* nombreArchivo, int offset, int size, char* codigoArchivo)
{
	logger =log_create(logMDJ,"MDJ",1, LOG_LEVEL_INFO);
	int aux ;
	if(validarArchivo(nombreArchivo,&aux) == 0)
	{
		log_info(logger,"Error 30004: Archivo no existe");
		return 0 ;
	}
	t_config *configMDJ = config_create(pathMDJ);
	char* puntoMontaje = config_get_string_value(configMDJ, "PUNTO_MONTAJE");
	char* pathArchivo = string_new();
	string_append(&pathArchivo, puntoMontaje);
	string_append(&pathArchivo, "Archivos");
	string_append(&pathArchivo, nombreArchivo);
	rutaArchivoActual = string_new();
	string_append(&rutaArchivoActual, pathArchivo);
	t_config *configArchivo = config_create(pathArchivo);  // /fifa-examples/fifa-checkpoint/Archivos/scripts/
	char** bloques = config_get_array_value(configArchivo, "BLOQUES");
	char* pathArchivo2 = string_new();
	string_append(&pathArchivo2, puntoMontaje);
	string_append(&pathArchivo2, "Metadata/Metadata.bin");	//fifa-examples/fifa-checkpoint/Metadata/Metadata.bin
	t_config *configArchivo2 = config_create(pathArchivo2);
//	int tamanioBloque = config_get_int_value(configArchivo, "TAMANIO_BLOQUES");
	int cantidadBloquesTotales = config_get_int_value(configArchivo2, "CANTIDAD_BLOQUES");
	int bloquesNecesarios = string_length(codigoArchivo)/tamanioBloque;
	if(string_length(codigoArchivo)%tamanioBloque != 0)
	{
		bloquesNecesarios++;
	}
	log_info(logger,"cantidadBloquesNecesarios: %d" , bloquesNecesarios);
	int i = 0;
	while(bloques[i] != NULL)
	{
		char* pathBloque = string_new();
		string_append(&pathBloque, puntoMontaje);
		string_append(&pathBloque, "Bloques/");  //    fifa-examples/fifa-checkpoint/Bloques/
//		string_append(&pathBloque, string_itoa(bloques[i]));
		string_append(&pathBloque, bloques[i]);
		string_append(&pathBloque, ".bin");   //   fifa-examples/fifa-checkpoint/Bloques/0.bin
		FILE *bloqueArchivo = fopen(pathBloque,"wb");
		fseek(bloqueArchivo, 0, SEEK_SET);
		char* aux = malloc(tamanioBloque);
		memcpy(aux,codigoArchivo+i*tamanioBloque,tamanioBloque);
		fwrite(aux,tamanioBloque,1,bloqueArchivo);
		free(aux);
		fclose(bloqueArchivo);
		i++;
	}
	log_info(logger, "Eeeeee");
	log_info(logger, "falan %d",bloquesNecesarios-i);
	if(bloquesNecesarios!= i)
	{
		int resultadoOperacion = asignarNuevosBloques(codigoArchivo,i,bloquesNecesarios-i,cantidadBloquesTotales,bloques,string_length(codigoArchivo));
		if(resultadoOperacion == -1)
		{
			log_info(logger,"30003: Espacio insuficiente en MDJ");
			return 0 ;
		}
	}
	log_info(logger,"Datos Guardados Exitosamente");
}

int asignarNuevosBloques(char *codigoArchivo,int posicion,int cantBloquesFaltantes, int cantidadBloquesTotales, char** bloques, int nuevoTamanioArchivo)
{
	log_info(logger,"Buscando nuevos bloques para terminar de guardar todo..");
	int cantBloquesLibres = 0 ;
	for(int i = 0 ; i < cantidadBloquesTotales;i++)
	{
		if(bitarray_test_bit(bitmap,i) == false)
		{
			cantBloquesLibres++;
		}
	}
	if(cantBloquesLibres < cantBloquesFaltantes)
	{
		//"Error 50002: Espacio Insuficiente"
		return -1 ;
	}
	t_list * nuevosBloques = list_create();

	for(int i = 0 ; i < cantidadBloquesTotales;i++)
	{
		if((bitarray_test_bit(bitmap,i) == false) && (cantBloquesFaltantes > 0))
		{
			bitarray_set_bit(bitmap, i);
			log_info(logger,"bloque q falta: %d ",i);
//			t_config *configMDJ = config_create(pathMDJ);
//			char* puntoMontaje = config_get_string_value(configMDJ, "PUNTO_MONTAJE");
			char* pathBloque = string_new();
			string_append(&pathBloque, puntoMontaje);
			string_append(&pathBloque, "Bloques/");  //    fifa-examples/fifa-checkpoint/Bloques/
			string_append(&pathBloque, string_itoa(i));
			string_append(&pathBloque, ".bin");   //   fifa-examples/fifa-checkpoint/Bloques/0.bin
			FILE *bloqueArchivo = fopen(pathBloque,"wb");
			char* aux = malloc(tamanioBloque+1); // NO ESTOY SEGURO SI VA EL +1
			memcpy(aux,codigoArchivo+ posicion*tamanioBloque,tamanioBloque);
			aux[tamanioBloque] = '\0';  //NO ESTOY SEGURO SI ESTA LINEA VA
			log_info(logger,"cacho q falta: %s ",aux);
			fwrite(aux,tamanioBloque,1,bloqueArchivo);
			free(aux);
			list_add(nuevosBloques,string_itoa(i));
			cantBloquesFaltantes--;
			posicion++;
			fclose(bloqueArchivo);
		}
	}
	actualizarScriptArchivo(bloques,nuevosBloques,nuevoTamanioArchivo);
	return 1 ;
}

void actualizarScriptArchivo(char** bloques,t_list* bloqueNuevo, int nuevoTamanioArchivo)
{
	log_info(logger,"Actualizando vector de bloques en archivo..");
	FILE * archivoNuevo = fopen(rutaArchivoActual,"w");
	int i =0;
	log_info(logger,"ooooooooo");
	t_list* bloqPosta = list_create();
	while(bloques[i] != NULL)
	{
		list_add(bloqPosta,bloques[i]);
		i++;
	}
	list_add_all(bloqPosta,bloqueNuevo);

	char* info = string_new();
	string_append(&info, "TAMANIO=");
//	string_append(&info,string_itoa(tamanioBloque*list_size(bloqPosta)+1));
	string_append(&info,string_itoa(nuevoTamanioArchivo+1));
	string_append(&info, "\nBLOQUES=[");
	int m = 0;
	log_info(logger,"antes del whilee");
	while(m < list_size(bloqPosta))
	{
		char *numeroBloque = (char*) list_get(bloqPosta,m);
		log_info(logger,"numBloqe..: %s",numeroBloque);
		string_append(&info,numeroBloque);
		if(list_size(bloqPosta) == m+1)
		{
			string_append(&info,"]");
		}
		else
		{
			string_append(&info,",");
		}
		m++;
	}

	log_info(logger,"emmmmmm");
	string_append(&info, "\n");
	fwrite(info,1,string_length(info)+1,archivoNuevo);   //PINCHA ACAA CUANDO TIENE QUE AGREGAR MAS DE 1 BLOQUE AL CONFIG
	log_info(logger,"yyyyyyyy");
	fclose(archivoNuevo);
}


void testeoMDJ()
{

	int resultadoOperacion;
/* 	resultadoOperacion = crearArchivo("PruebaMDJ.txt",80);
	log_info(logger,"res: %d" , resultadoOperacion);
	resultadoOperacion = crearArchivo("PruebaMDJ2.txt",280);
	log_info(logger,"res: %d" , resultadoOperacion); */
	int size =0;
	log_info(logger,"tamanio codigo:\n%d", size);
	validarArchivo("scripts/checkpoint.escriptorio", &size);
	char* codigoEscriptorio;
	log_info(logger,"tamanio codigo:\n%d", size);
	codigoEscriptorio = obtenerDatosArchivo("scripts/checkpoint.escriptorio",0,size);
	log_info(logger,"Codigo archivo:\n%s", codigoEscriptorio);

	crearArchivo("hola.txt",50);
	crearArchivo("test.txt",50);
	crearArchivo("hola2.txt",50);
	crearArchivo("test2.txt",50);
	char* pepe =string_new();
	string_append(&pepe, "hola pepe como estas jojojoj todo bien perrotes dale que ya terminamos mdj");
	int sizeNuevoCodigo = string_length(pepe);
	log_info(logger,"tamanio nuevo codigo: %d", sizeNuevoCodigo);
	guardarDatosArchivo("test.txt", 0, sizeNuevoCodigo, pepe);
//	int size =0;
	validarArchivo("test.txt", &size);
	codigoEscriptorio = obtenerDatosArchivo("test.txt",0,size);
	log_info(logger,"Codigo archivo:\n%s", codigoEscriptorio);

	pepe =string_new();
	string_append(&pepe, "hola juan probando 1223");
	sizeNuevoCodigo = string_length(pepe);
	log_info(logger,"tamanio nuevo codigo: %d", sizeNuevoCodigo);
	guardarDatosArchivo("test2.txt", 0, sizeNuevoCodigo, pepe);
//	int size =0;
	validarArchivo("test2.txt", &size);
	codigoEscriptorio = obtenerDatosArchivo("test2.txt",0,size);
	log_info(logger,"Codigo archivo:\n%s", codigoEscriptorio);

	pepe =string_new();
	string_append(&pepe, "ultima prueba ya van 6 horas haciendo esto no doy mas me qiero ir dale dale dale dael roger roger federer el mejor de la historia si pibe al fin anduvo todo el MDJ gracias DIOS");
	sizeNuevoCodigo = string_length(pepe);
	log_info(logger,"tamanio nuevo codigo: %d", sizeNuevoCodigo);
	guardarDatosArchivo("hola2.txt", 0, sizeNuevoCodigo, pepe);
//	int size =0;
	validarArchivo("hola2.txt", &size);
	codigoEscriptorio = obtenerDatosArchivo("hola2.txt",0,size);
	log_info(logger,"Codigo archivo:\n%s", codigoEscriptorio);

	borrarArchivo("hola.txt");
	crearArchivo("chauMDJ.txt",260);
	/*
//	char* codigoEscriptorio;
	crearArchivo("hola.txt",50);
	crearArchivo("test.txt",50);
	char* pepe =string_new();
	string_append(&pepe, "hola pepe como estas jojojoj todo bien perrotes dale que ya terminamos mdj");
	int sizeNuevoCodigo = string_length(pepe);
	log_info(logger,"tamanio nuevo codigo: %d", sizeNuevoCodigo);
	guardarDatosArchivo("test.txt", 0, sizeNuevoCodigo, pepe);
//	int size =0;
	validarArchivo("test.txt", &size);
	codigoEscriptorio = obtenerDatosArchivo("test.txt",0,size);
	log_info(logger,"Codigo archivo:\n%s", codigoEscriptorio);

	//pruebo borrar archivo
	borrarArchivo("hola.txt");

	crearArchivo("testeoo.txt",221);

	char* finatest =string_new();
	string_append(&finatest, "Ante un pedido de escritura MDJ almacenará en el path enviado por parámetro,.probando eee loco si anda o no");
	int sizeNuevoCodigo2 = string_length(finatest);
	log_info(logger,"tamanio nuevo codigo: %d", sizeNuevoCodigo2);
	guardarDatosArchivo("test.txt", 0, sizeNuevoCodigo2, finatest);
//	int size =0;
	validarArchivo("test.txt", &size);
	codigoEscriptorio = obtenerDatosArchivo("test.txt",0,size);
	log_info(logger,"Codigo archivo:\n%s", codigoEscriptorio);  */

//	crearArchivo("testBorrar1.txt",201);
//	crearArchivo("testBorrar2.txt",82);
//	borrarArchivo("testBorrar2.txt");
//	crearArchivo("testAFTERBorrar.txt",221);

//	int tam;
//	validarArchivo("testBorrar2.txt",&tam);

}



