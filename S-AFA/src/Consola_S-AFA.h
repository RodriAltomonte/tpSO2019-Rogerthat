/*
 * Consola_S-AFA.h
 *
 *  Created on: 11 sep. 2018
 *      Author: utnso
 */

#ifndef CONSOLA_S_AFA_H_
#define CONSOLA_S_AFA_H_

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <commons/string.h>
#include <commons/log.h>

void consola();
int recorrerCentinela(char** centinelas,int liberar);
bool noTieneDTBasociado(void* cpuConectada);
int cantidadDeCentinelas(char** centinelas);

#endif /* CONSOLA_S_AFA_H_ */
