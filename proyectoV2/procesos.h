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

#define EVITAR_RETECION_EM 1 // variable para limitar la ejecución de procesos en nodo, y asi evitar la retención de exclusión mutua.

struct msgbuf_mensaje{

  long msg_type; // TIPO 1 --> SOLICITUD      //TIPO 2 --> TESTIGO    //TIPO 3 --> ENVIAR TESTIGO CONSULTAS    //TIPO 4 --> RECIBIR TESTIGO CONSULTAS
  int id;                                                               
  int peticion;
  int prioridad;
  int atendidas[N][P];
  int id_nodo_master;
};

typedef struct{ // MEMOMORIA COMPARTIDA POR LOS PROCESOS DE UN NODO.

  //Variables para consultas concurrentes
  int dentro_C;
  bool nodo_master, testigos_recogidos;
  int id_nodo_master;
  int nodos_con_consultas[N];

  sem_t sem_dentro_C, sem_nodo_master, sem_nodos_con_consultas,
        sem_testigos_recogidos, sem_id_nodo_master;

  // VARIABLES GLOBALES
  bool testigo;
  bool dentro;
  bool turno_PA, turno_RA, turno_C, turno;


  int atendidas[N][P], peticiones[N][P];
  int buzones_nodos[N];

  int prioridad_maxima, prioridad_max_otro_nodo;

  // SEMÁFOROS GLOBALES
  sem_t sem_testigo, sem_atendidas, sem_buzones_nodos, sem_mi_peticion,
      sem_peticiones, sem_prioridad_maxima,
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

  sem_wait(&(me->sem_prioridad_max_otro_nodo));
  sem_wait(&(me->sem_prioridad_maxima));
  #ifdef __DEBUG
  printf("\tDEBUG --> maxima mi nodo %d, otro nodo %d.\n",me->prioridad_maxima, me->prioridad_max_otro_nodo);
  #endif
  sem_post(&(me->sem_prioridad_max_otro_nodo));
  sem_post(&(me->sem_prioridad_maxima));
}

void send_testigo(int mi_id, memoria *me){ // MODIFICAR PARA LA NUEVA SITUACIÓN

  int i = 0, j = 0;
  int id_buscar = mi_id;
  int id_buscar_prima;
  bool encontrado = false;
  struct msgbuf_mensaje msg_testigo;
  msg_testigo.msg_type = (long)2;
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
  
  sem_wait(&(me->sem_atendidas));
  sem_wait(&(me->sem_peticiones));
  sem_wait(&me->sem_contador_anul_pagos_pendientes);
  if(me->contador_anul_pagos_pendientes == 0){
    me->atendidas[mi_id - 1][PAGOS_ANUL - 1] = me->peticiones[mi_id - 1][PAGOS_ANUL - 1];
  }
  sem_post(&(me->sem_contador_anul_pagos_pendientes));
  sem_wait(&me->sem_contador_reservas_admin_pendientes);
  if(me->contador_reservas_admin_pendientes == 0){
    me->atendidas[mi_id - 1][ADMIN_RESER - 1] = me->peticiones[mi_id - 1][ADMIN_RESER - 1];
  }
  sem_post(&(me->sem_contador_reservas_admin_pendientes));
  sem_wait(&me->sem_contador_consultas_pendientes);
  if(me->contador_consultas_pendientes == 0){
    me->atendidas[mi_id - 1][CONSULTAS - 1] = me->peticiones[mi_id - 1][CONSULTAS - 1];
  }
  sem_post(&(me->sem_contador_consultas_pendientes));
  sem_post(&(me->sem_atendidas));
  sem_post(&(me->sem_peticiones));
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
          sem_wait(&(me->sem_nodo_master));
          me->nodo_master = false;
          sem_post(&(me->sem_nodo_master));
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
        sem_post(&me->sem_atendidas);
      }
    }

    sem_wait(&me->sem_testigo);
    me->testigo = false;
    sem_post(&me->sem_testigo);
    // ENVIANDO TESTIGO
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

void send_copias_testigos(int mi_id, memoria *me){//enviar copias del testigo
                                                  //el nodo master es el que usa esta función.
  int i, j;
  struct msgbuf_mensaje msg_testigo;
  msg_testigo.msg_type = (long)3;
  msg_testigo.id = mi_id;
  msg_testigo.id_nodo_master = mi_id;
  for (i = 0; i < N; i++){
    for (j = 0; j < P; j++){
      sem_wait(&me->sem_atendidas);
      msg_testigo.atendidas[i][j] = me->atendidas[i][j];
      sem_post(&me->sem_atendidas);
    }
  }
  sem_wait(&(me->sem_atendidas));
  sem_wait(&(me->sem_peticiones));
  for(i = 0; i < N; i ++){
    if(me->atendidas[i][CONSULTAS - 1] < me->peticiones[i][CONSULTAS - 1]){
      me->atendidas[i][CONSULTAS - 1] = me->peticiones[i][CONSULTAS - 1];
      sem_wait(&(me->sem_buzones_nodos));
      if (msgsnd(me->buzones_nodos[i], &msg_testigo, sizeof(msg_testigo), 0)){
        printf("PROCESO ENVIO TESTIGO FALSO: \n\n\tERROR: Hubo un error al enviar el testigo.\n");
      }
      sem_post(&me->sem_buzones_nodos);
    }
  }
  sem_post(&(me->sem_atendidas));
  sem_post(&(me->sem_peticiones));
}

void send_testigo_consultas_master(int mi_id, memoria *me){//enviar el testigo a quien tenga lectores o dar paso a un lector
  set_prioridad_max(me);                                   //después del turno de consultas
                                                            // paso de consultas a lector, esta función la usa el nodo_master
  sem_wait(&(me->sem_prioridad_max_otro_nodo));
  sem_wait(&(me->sem_prioridad_maxima));
  if(me->prioridad_max_otro_nodo > me->prioridad_maxima){
    printf("envio el testigo a quien corresponda\n");
    sem_post(&(me->sem_prioridad_max_otro_nodo));
    sem_post(&(me->sem_prioridad_maxima));
    send_testigo(mi_id, me);
  }else{
    sem_post(&(me->sem_prioridad_max_otro_nodo));
    if(me->prioridad_maxima == PAGOS_ANUL){
      sem_post(&(me->sem_prioridad_maxima));
      printf("es el turno de pagos\n");
      sem_wait(&(me->sem_turno));
      me->turno = true;
      sem_post(&(me->sem_turno));
      sem_wait(&(me->sem_turno_PA));
      me->turno_PA = true;
      sem_post(&(me->sem_turno_PA));
      sem_post(&(me->sem_anul_pagos_pend));
    }else{
      if(me->prioridad_maxima == ADMIN_RESER){
        sem_post(&(me->sem_prioridad_maxima));
        printf("es el turno de admin\n");
        sem_wait(&(me->sem_turno));
        me->turno = true;
        sem_post(&(me->sem_turno));
        sem_wait(&(me->sem_turno_RA));
        me->turno_RA = true;
        sem_post(&(me->sem_turno_RA));
        sem_post(&(me->sem_reser_admin_pend));
      }else{
        sem_post(&(me->sem_prioridad_maxima));
        printf("no es el turno de nadie\n");
      }
    }
  }
}

void send_testigo_consultas(int mi_id, memoria *me){//enviar la copia del testigo 
                                                    //esta función es usada por el nodo master y
                                                    //por los nodos con consultas cuando ya no tienen consultas
                                                    //o cuando les toca devolver el testigo porque hay procesos
                                                    //escritores

  int i;
  sem_wait(&(me->sem_nodo_master));
  if(me->nodo_master){//SOY EL NODO MASTER
    
    sem_post(&(me->sem_nodo_master));
    #ifdef __DEBUG
    printf("DEBUG: SOY EL NODO MASTER.\n");
    #endif
    sem_wait(&(me->sem_testigos_recogidos));
    me->testigos_recogidos = true;
    sem_post(&(me->sem_testigos_recogidos));
    sem_wait(&(me->sem_nodos_con_consultas));
    me->nodos_con_consultas[mi_id - 1] = 0;
    sem_post(&(me->sem_nodos_con_consultas));
    for(i = 0; i < N; i++){
      sem_wait(&(me->sem_nodos_con_consultas));
      if(me->nodos_con_consultas[i] == 1){
        sem_post(&(me->sem_nodos_con_consultas));
        sem_wait(&(me->sem_testigos_recogidos));
        me->testigos_recogidos = false;
        sem_post(&(me->sem_testigos_recogidos));
        break;
      }else{
        sem_post(&(me->sem_nodos_con_consultas));
      }
    }
    sem_wait(&(me->sem_testigos_recogidos));
    if(me->testigos_recogidos){
      me->testigos_recogidos = false;
      sem_post(&(me->sem_testigos_recogidos));
      send_testigo_consultas_master(mi_id, me);
    }else{
      sem_post(&(me->sem_testigos_recogidos));
    }
  }else{//NO SOY EL NODO MASTER
    sem_post(&(me->sem_nodo_master));
    #ifdef __DEBUG
    printf("DEBUG: NO SOY EL NODO MASTER.\n");
    #endif
    struct msgbuf_mensaje msg_testigo;
    msg_testigo.msg_type = (long)4;
    msg_testigo.id = mi_id;
    sem_wait(&(me->sem_id_nodo_master));
    msg_testigo.id_nodo_master = me->id_nodo_master;
    sem_post(&(me->sem_id_nodo_master));
    sem_wait(&me->sem_buzones_nodos);
    if (msgsnd(me->buzones_nodos[msg_testigo.id_nodo_master - 1], &msg_testigo, sizeof(msg_testigo), 0)){
      printf("PROCESO ENVIO TESTIGO FALSO: \n\n\tERROR: Hubo un error al enviar el testigo.\n");
    }
    sem_post(&me->sem_buzones_nodos);
  }
  sem_wait(&(me->sem_atendidas));
  sem_post(&(me->sem_peticiones));
  me->atendidas[mi_id - 1][CONSULTAS - 1] = me->peticiones[mi_id - 1][CONSULTAS - 1];
  sem_post(&(me->sem_atendidas));
  sem_post(&(me->sem_peticiones));
  sem_wait(&(me->sem_contador_consultas_pendientes));
  if(me->contador_consultas_pendientes > 0){
    sem_post(&(me->sem_contador_consultas_pendientes));
    send_peticiones(me, mi_id, CONSULTAS);
  }else{
    sem_post(&(me->sem_contador_consultas_pendientes));
  }
  return;
}



int max(int n1, int n2){
  if (n1 > n2)return n1;
  else return n2;
}

#endif