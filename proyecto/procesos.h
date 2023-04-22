#ifndef __PROCESO_H
#define __PROCESO_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <semaphore.h>
#include <time.h>
#include <unistd.h>

#define N 5 // --> nodos
#define P 5 // --> procesos
#define MAX_ESPERA 10
#define SLEEP 3
#define __PRINT_RX        // comentar en caso de no querer mensajes del proceso receptor
#define __PRINT_PROCESO   // comentar en caso de no querer mensajes de los procesos escritores del nodo.
#define __PRINT_CONSULTAS // comentar en caso deno querer mensajes de los procesos consultas.
#define __DEBUG

struct msgbuf_mensaje
{

  long msg_type;
  int id;
  int peticion;
  int prioridad;
  int atendidas[N][2];
};

typedef struct
{
  // VARIABLES GLOBALES
  bool testigo;
  bool permiso_para_S_C_E_M;
  bool tengo_que_pedir_testigo;
  int atendidas[N][2], peticiones[N][2];
  int buzones_nodos[N];

  // SEMÁFOROS GLOBALES
  sem_t sem_testigo, sem_atendidas, sem_buzones_nodos, sem_mi_peticion,
      sem_peticiones, sem_tengo_que_pedir_testigo, sem_permiso_para_S_C_E_M;

  // VARIABLES PROCESOS
  int mi_peticion;
  struct msgbuf_mensaje solicitud;
  int contador_anul_pagos_pendientes, contador_reservas_admin_pendientes, contador_consultas_pendientes;
  int contador_procesos_SC;

  // SEMÁFOROS PROCESOS
  sem_t sem_mi_peticion, sem_contador_procesos_SC;
  sem_t sem_contador_anul_pagos_pendientes, sem_contador_reservas_admin_pendientes, sem_contador_consultas_pendientes;

  sem_t sem_SCEM; // semáforo exclusión mutua para todos los procesos de tipo escritor
} memoria;

#endif