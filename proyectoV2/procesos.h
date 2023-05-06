#ifndef __PROCESO_H
#define __PROCESO_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <semaphore.h>
#include <sys/types.h>
#include <time.h>
#include <sys/shm.h>
#include <unistd.h>

#define __PRINT_RX        // comentar en caso de no querer mensajes del proceso receptor
#define __PRINT_PROCESO   // comentar en caso de no querer mensajes de los procesos escritores del nodo.
#define __PRINT_CONSULTAS // comentar en caso deno querer mensajes de los procesos consultas.
#define __DEBUG

#define N 4 // --> nodos
#define P 3 // --> prioridades
#define MAX_ESPERA 10
#define SLEEP 3

#define PAGOS_ANUL 3
#define ADMIN_RESER 2
#define CONSULTAS 1

#define EVITAR_RETECION_EM 2 // variable para limitar la ejecución de procesos en nodo, y asi evitar la retención de exclusión mutua.

struct msgbuf_mensaje{

  long msg_type; // TIPO 1 --> SOLICITUD      //TIPO 2 --> TESTIGO    //TIPO 3 --> TESTIGO CONSULTAS??????
  int id;
  int peticion;
  int prioridad;
  int atendidas[N][P];
  int id_nodo_master;
};

typedef struct{ // MEMOMORIA COMPARTIDA POR LOS PROCESOS DE UN NODO.

  // VARIABLES GLOBALES
  bool testigo;
  bool tengo_que_pedir_testigo;
  bool tengo_que_enviar_testigo;
  bool dentro;

  bool turno_PA, turno_RA, turno_C, turno;

  int atendidas[N][P], peticiones[N][P];
  int buzones_nodos[N];

  int prioridad_maxima, prioridad_max_otro_nodo;

  // SEMÁFOROS GLOBALES
  sem_t sem_testigo, sem_atendidas, sem_buzones_nodos, sem_mi_peticion,
      sem_peticiones, sem_tengo_que_pedir_testigo,
      sem_tengo_que_enviar_testigo, sem_prioridad_maxima,
      sem_prioridad_max_otro_nodo, sem_dentro,
      sem_turno_PA, sem_turno_RA, sem_turno_C, sem_turno;

  // VARIABLES PROCESOS
  int mi_peticion;
  int contador_anul_pagos_pendientes, contador_reservas_admin_pendientes, contador_consultas_pendientes;
  int contador_procesos_max_SC; // contador_procesos_max_SC sirve para evitar la retencion de exclusión mutua.

  // SEMÁFOROS PROCESOS
  sem_t sem_contador_procesos_max_SC;
  sem_t sem_contador_anul_pagos_pendientes, sem_contador_reservas_admin_pendientes, sem_contador_consultas_pendientes;
  sem_t sem_anul_pagos_pend, sem_reser_admin_pend, sem_consult_pend;
} memoria;

void send_testigo(int mi_id, memoria *me){ // MODIFICAR PARA LA NUEVA SITUACIÓN

  int i = 0, j = 0;
  int id_buscar = mi_id;
  int id_buscar_prima;
  bool encontrado = false;
  struct msgbuf_mensaje msg_testigo;
  msg_testigo.msg_type = 2;
  msg_testigo.id = mi_id;
  if (id_buscar + 1 > N){
    id_buscar = 1;
  }else{

    id_buscar++;
  }
  id_buscar_prima = id_buscar;
  sem_wait(&(me->sem_contador_procesos_max_SC));
  me->contador_procesos_max_SC = 0;
  sem_post(&(me->sem_contador_procesos_max_SC));

  // COMPROBACIÓN DE SI HAY ALGUIEN ESPERANDO
  for (j = P - 1; j > -1; j--){
    id_buscar = id_buscar_prima;
    for (i = 0; i < N; i++) {

      // ANILLO LÓGICO
      if (id_buscar > N) {
        id_buscar = 1;
      }
      if (id_buscar != mi_id){

        // SI HAY MAS PETICIONES QUE ATENDIDAS, ESTÁ ESPERANDO EL TESTIGO
        sem_wait(&me->sem_peticiones);
        sem_wait(&me->sem_atendidas);
        if ((me->peticiones[id_buscar - 1][j] > me->atendidas[id_buscar - 1][j])){
          #ifdef __DEBUG
          printf("\nDEBUG: Nodo: %d; con prioridad: %d; las peticiones son: %d; las atendidas son: %d\n\n", id_buscar, j + 1, me->peticiones[id_buscar - 1][j], me->atendidas[id_buscar - 1][j]);
          #endif
          sem_post(&me->sem_atendidas);
          sem_post(&me->sem_peticiones);
          encontrado = true;
          break;
        }else{
          sem_post(&me->sem_peticiones);
          sem_post(&me->sem_atendidas);
        }
      }
      id_buscar++;
    }
    if(encontrado){
      break;
    }
  }
  if (encontrado){
    #ifdef __DEBUG
    printf("DEBUG: nodo destinatarios encontrado: id = %d\n", id_buscar);
    #endif

    // CREANDO EL MENSAJE PARA EL TESTIGO
    msg_testigo.id = id_buscar;
    for (i = 0; i < N; i++){
      for (j = 0; j < P; j++){
        sem_wait(&me->sem_atendidas);
        msg_testigo.atendidas[i][j] = me->atendidas[i][j];
        printf("Atendidas: %d, testigo: %d\n",me->atendidas[i][j],msg_testigo.atendidas[i][j]);
        sem_post(&me->sem_atendidas);
      }
    }

    sem_wait(&me->sem_testigo);
    me->testigo = false;
    sem_post(&me->sem_testigo);
    // ENVIANDO TESTIGO
    sem_wait(&(me->sem_tengo_que_pedir_testigo));
    me->tengo_que_pedir_testigo = true;
    sem_post(&(me->sem_tengo_que_pedir_testigo));
    sem_wait(&(me->sem_tengo_que_enviar_testigo));
    me->tengo_que_enviar_testigo = false;
    sem_post(&(me->sem_tengo_que_enviar_testigo));
    sem_wait(&me->sem_buzones_nodos);
    if (msgsnd(me->buzones_nodos[id_buscar - 1], &msg_testigo, sizeof(msg_testigo), 0)){
      printf("PROCESO ENVIO: \n\n\tERROR: Hubo un error al enviar el testigo.\n");
    }
    sem_post(&me->sem_buzones_nodos);

    printf("PROCESO ENVIO: \n\t\t TESTIGO ENVIADO\n");
  }else{
    #ifdef __DEBUG
    printf("DEBUG: NO HAY NODO DISPONIBLE.\n");
    #endif
  }
  return;
}

void set_prioridad_max(memoria *me){
  sem_wait(&(me->sem_contador_anul_pagos_pendientes));
  if (me->contador_anul_pagos_pendientes > 0){
    sem_post(&(me->sem_contador_anul_pagos_pendientes));
    sem_wait(&(me->sem_prioridad_maxima));
    me->prioridad_maxima = PAGOS_ANUL;
    sem_post(&(me->sem_prioridad_maxima));
  }else{
    sem_post(&(me->sem_contador_anul_pagos_pendientes));
    sem_wait(&(me->sem_contador_reservas_admin_pendientes));
    if (me->contador_reservas_admin_pendientes > 0){
      sem_post(&(me->sem_contador_reservas_admin_pendientes));
      sem_wait(&(me->sem_prioridad_maxima));
      me->prioridad_maxima = ADMIN_RESER;
      sem_post(&(me->sem_prioridad_maxima));
    }else{
      sem_post(&(me->sem_contador_reservas_admin_pendientes));
      sem_wait(&(me->sem_contador_consultas_pendientes));
      if (me->contador_consultas_pendientes > 0){
        sem_post(&(me->sem_contador_consultas_pendientes));
        sem_wait(&(me->sem_prioridad_maxima));
        me->prioridad_maxima = CONSULTAS;
        sem_post(&(me->sem_prioridad_maxima));
      }else{
        sem_post(&(me->sem_contador_consultas_pendientes));
        sem_wait(&(me->sem_prioridad_maxima));
        me->prioridad_maxima = 0;
        sem_post(&(me->sem_prioridad_maxima));
      }
    }
  }
  sem_wait(&(me->sem_prioridad_maxima));
  sem_wait(&(me->sem_prioridad_max_otro_nodo));
  if(me->prioridad_max_otro_nodo > me->prioridad_maxima){
    sem_wait(&(me->sem_tengo_que_enviar_testigo));
    me->tengo_que_enviar_testigo = true;
    
    sem_post(&(me->sem_tengo_que_enviar_testigo));
  }
  sem_post(&(me->sem_prioridad_maxima));
  sem_post(&(me->sem_prioridad_max_otro_nodo));

  sem_wait(&(me->sem_prioridad_max_otro_nodo));
  sem_wait(&(me->sem_prioridad_maxima));
  #ifdef __DEBUG
  printf("\tDEBUG --> maxima mi nodo %d, otro nodo %d.\n",me->prioridad_maxima, me->prioridad_max_otro_nodo);
  #endif
  sem_post(&(me->sem_prioridad_max_otro_nodo));
  sem_post(&(me->sem_prioridad_maxima));
}

void send_peticiones(memoria *me, int mi_id, int prioridad){
  int i;
  struct msgbuf_mensaje solicitud;
  sem_wait(&(me->sem_mi_peticion));
  me->mi_peticion = me->mi_peticion + 1;
  sem_wait(&(me->sem_peticiones));
  me->peticiones[mi_id - 1][prioridad - 1] = me->mi_peticion;
  sem_post(&(me->sem_peticiones));
  solicitud.peticion = me->mi_peticion;
  sem_post(&(me->sem_mi_peticion));
  solicitud.msg_type = (long)1;
  solicitud.id = mi_id;
  solicitud.prioridad = prioridad;

  #ifdef __DEBUG
  printf("El mensaje es de tipo: %ld, con peticion: %i, con id: %i y prioridad: %i\n",
          solicitud.msg_type, solicitud.peticion, solicitud.id, solicitud.prioridad);
  #endif

  // ENVIO PETICIONES
    for (i = 0; i < N; i++){
        if (mi_id - 1 == i){
            continue;
        }else{
            sem_wait(&(me->sem_buzones_nodos));
            // sem_wait(&sem_msg_solicitud);
            if (msgsnd(me->buzones_nodos[i], &solicitud, sizeof(solicitud), 0) == -1){
                // sem_post(&sem_msg_solicitud);
                sem_post(&(me->sem_buzones_nodos));
                #ifdef __DEBUG
                printf("PAGOS:\n\tERROR: Hubo un error enviando el mensaje al nodo: %i.\n", i);
                #endif
            }else{
                sem_post(&(me->sem_buzones_nodos));
            }
        }
    }
}

int max(int n1, int n2){
  if (n1 > n2)return n1;
  else return n2;
}

#endif