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
#include <sys/shm.h>
#include <unistd.h>

#define N 5 // --> nodos
#define P 5 // --> procesos
#define MAX_ESPERA 10
#define SLEEP 3

#define PAGOS_ANUL 1
#define ADMIN_RESER 2
#define CONSULTAS 3

#define __PRINT_RX        // comentar en caso de no querer mensajes del proceso receptor
#define __PRINT_PROCESO   // comentar en caso de no querer mensajes de los procesos escritores del nodo.
#define __PRINT_CONSULTAS // comentar en caso deno querer mensajes de los procesos consultas.
#define __DEBUG

struct msgbuf_mensaje
{

  long msg_type; // TIPO 1 --> SOLICITUD      //TIPO 2 --> TESTIGO
  int id;
  int peticion;
  int prioridad;
  int atendidas[N][2];
};

typedef struct // MEMOMORIA COMPARTIDA POR LOS PROCESOS DE UN NODO.
{
  // VARIABLES GLOBALES
  bool testigo;
  bool permiso_para_SCEM_anulpagos;
  bool permiso_para_SCEM_resadmin;
  bool permiso_para_SCEM_cons;
  bool tengo_que_pedir_testigo;
  bool tengo_que_enviar_testigo;
  int atendidas[N][2], peticiones[N][2];
  int buzones_nodos[N];

  // SEMÁFOROS GLOBALES
  sem_t sem_testigo, sem_atendidas, sem_buzones_nodos, sem_mi_peticion,
      sem_peticiones, sem_tengo_que_pedir_testigo, sem_permiso_para_SCEM_anulpagos,
      sem_permiso_para_SCEM_resadmin, sem_permiso_para_SCEM_cons, sem_tengo_que_enviar_testigo;

  // VARIABLES PROCESOS
  int mi_peticion;
  int contador_anul_pagos_pendientes, contador_reservas_admin_pendientes, contador_consultas_pendientes;
  int contador_procesos_SC;

  // SEMÁFOROS PROCESOS
  sem_t sem_contador_procesos_SC;
  sem_t sem_contador_anul_pagos_pendientes, sem_contador_reservas_admin_pendientes, sem_contador_consultas_pendientes;
  sem_t sem_anul_pagos_pend, sem_reser_admin_pend, sem_consult_pend;

  sem_t sem_SCEM; // semáforo exclusión mutua para todos los procesos de tipo escritor
} memoria;

void send_testigo(int id)
{

  int i = 0;
  sem_wait(&sem_mi_id);
  int id_buscar = mi_id;
  int yo = mi_id;
  sem_post(&sem_mi_id);
  bool encontrado = false;
  struct msgbuf_mensaje msg_testigo;
  msg_testigo.msg_type = 2;
  msg_testigo.id = yo;
  if (id_buscar + i + 1 > N)
  {
    id_buscar = 1;
  }
  else
  {

    id_buscar++;
  }

  // COMPROBACIÓN DE SI HAY ALGUIEN ESPERANDO
  for (i; i < N; i++)
  {

    // ANILLO LÓGICO
    if (id_buscar > N)
    {
      id_buscar = 1;
    }
    if (id_buscar != yo)
    {

      // SI HAY MAS PETICIONES QUE ATENDIDAS, ESTÁ ESPERANDO EL TESTIGO
      sem_wait(&sem_peticiones);
      sem_wait(&sem_atendidas);
      if (peticiones[id_buscar - 1] > atendidas[id_buscar - 1])
      {
        sem_post(&sem_peticiones);
        sem_post(&sem_atendidas);
        encontrado = true;
        printf("PROCESO ENVIO: El id al que le vamos a enviar el testigo es: %d\n", id_buscar);
        break;
      }
      else
      {
        sem_post(&sem_peticiones);
        sem_post(&sem_atendidas);
      }
    }
    id_buscar++;
  }

  if (encontrado)
  {

    // CREANDO EL MENSAJE PARA EL TESTIGO
    msg_testigo.id = id_buscar;
    i = 0;
    for (i; i < N; i++)
    {
      sem_wait(&sem_atendidas);
      msg_testigo.atendidas[i] = atendidas[i];
      sem_post(&sem_atendidas);
    }

    sem_wait(&sem_testigo);
    testigo = false;
    sem_post(&sem_testigo);
    // ENVIANDO TESTIGO
    sem_wait(&sem_buzones_nodos);
    if (msgsnd(buzones_nodos[id_buscar - 1], &msg_testigo, sizeof(msg_testigo), 0))
    {
      printf("PROCESO ENVIO: \n\n\tERROR: Hubo un error al enviar el testigo.\n");
    }
    sem_post(&sem_buzones_nodos);

    printf("PROCESO ENVIO: \n\t\t TESTIGO ENVIADO\n");
  }
}

int max(int n1, int n2)
{
  if (n1 > n2)
    return n1;
  else
    return n2;
}

#endif