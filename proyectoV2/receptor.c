#include "procesos.h"

memoria *me;

int main(int argc, char *argv[]){

    if (argc != 2){
        printf("La forma correcta de ejecución es: %s \"id_nodo\"\n", argv[0]);
        return -1;
    }
    // INICIALIZACIÓN DE VARIABLES, MEMORIA COMPARTIDA Y SEMÁFOROS
    //  inicialización variables
    int mi_id = atoi(argv[1]);
    int i, j;
    memoria *me;
    int memoria_id;
    // inicialización memoria compartida
    memoria_id = shmget(mi_id, sizeof(memoria), 0666 | IPC_CREAT);
    me = shmat(memoria_id, NULL, 0);
    #ifdef __DEBUG
    printf("El id de la memoria compartida es: %i\n", memoria_id);
    #endif

    // inicialización de variables memoria compartida
    if (mi_id == 1){
        #ifdef __PRINT_RX
        printf("Soy el nodo 1\n");
        #endif
        me->testigo = true;
        me->nodo_master = true;
    }else{
        me->testigo = false;
        me->nodo_master = false;
    }

    for (i = 0; i < N; i++){
        for (j = 0; j < P; j++){
            me->atendidas[i][j] = 0;
            me->peticiones[i][j] = 0;
        }
        me->nodos_con_consultas[i] = 0;
    }
    me->testigos_recogidos = false;
    me->id_nodo_master = 0;
    me->dentro_C = 0;
    me->dentro = false;
    me->mi_peticion = 0;
    me->contador_anul_pagos_pendientes = 0;
    me->contador_consultas_pendientes = 0;
    me->contador_procesos_max_SC = 0;
    me->contador_reservas_admin_pendientes = 0;
    me->prioridad_max_otro_nodo = 0;
    me->prioridad_maxima = 0;
    me->turno_PA = false;
    me->turno_RA = false;
    me->turno_C = false;
    me->turno = false;
    // inicialización de semáforos
    // inicialización de semáforos para consultas concurrentes
    sem_init(&(me->sem_dentro_C), 1, 1);
    sem_init(&(me->sem_id_nodo_master), 1, 1);
    sem_init(&(me->sem_nodo_master), 1, 1);
    sem_init(&(me->sem_nodos_con_consultas), 1, 1);
    sem_init(&(me->sem_testigos_recogidos), 1, 1);
    // inicialización semáforos de paso.
    sem_init(&(me->sem_anul_pagos_pend), 1, 0);
    sem_init(&(me->sem_reser_admin_pend), 1, 0);
    sem_init(&(me->sem_consult_pend), 1, 0);
    // inicialización semáforos exclusión mutua
    sem_init(&me->sem_contador_anul_pagos_pendientes, 1, 1);
    sem_init(&(me->sem_contador_reservas_admin_pendientes), 1, 1);
    sem_init(&(me->sem_contador_consultas_pendientes), 1, 1);
    sem_init(&(me->sem_mi_peticion), 1, 1);
    sem_init(&(me->sem_testigo), 1, 1);
    sem_init(&(me->sem_prioridad_maxima), 1, 1);
    sem_init(&(me->sem_contador_procesos_max_SC), 1, 1);
    sem_init(&(me->sem_prioridad_max_otro_nodo), 1, 1);
    sem_init(&(me->sem_dentro), 1, 1);
    sem_init(&(me->sem_atendidas), 1, 1);
    sem_init(&(me->sem_peticiones), 1, 1);
    sem_init(&(me->sem_buzones_nodos), 1, 1);
    sem_init(&(me->sem_turno_PA), 1, 1);
    sem_init(&(me->sem_turno_RA), 1, 1);
    sem_init(&(me->sem_turno_C), 1, 1);
    sem_init(&(me->sem_turno), 1, 1);
    // INICIALIZACIÓN DE BUZONES NODOS
    //  INICIALIZACIÓN BUZONES DE LOS NODOS.
    for (i = 0; i < N; i++){

        me->buzones_nodos[i] = msgget(i + 1, IPC_CREAT | 0777);
        #ifdef __DEBUG
        printf("El id del buzón del nodo %i es: %i.\n", i + 1, me->buzones_nodos[i]);
        #endif

        if (me->buzones_nodos[i] == -1){
            perror("No se creó el mensaje\n");
        }
    }
    // INICIO RX!!!!!!!!!!!!!!!!!
    struct msgbuf_mensaje mensaje_rx;
    sem_wait(&(me->sem_buzones_nodos));
    int id_de_mi_buzon = me->buzones_nodos[mi_id - 1];
    sem_post(&(me->sem_buzones_nodos));

    while (true){
        // RECIBIMOS PETICIÓN
        if (msgrcv(id_de_mi_buzon, &mensaje_rx, sizeof(mensaje_rx), 0, 0) == -1){
            printf("Proceso Rx: ERROR: Hubo un error al recibir un mensaje en el RECEPTOR.\n");
            return -1;
        }

        // ACTUALIZO EL VALOR DE PETICIONES CON LA QUE ME ACABA DE LLEGAR
        switch (mensaje_rx.msg_type){
            case (long)1: // EL mensaje es una petición.
                #ifdef __PRINT_RX
                printf("RECEPTOR: He recibido una petición (%d) del nodo: %d, con prioridad: %d\n", mensaje_rx.peticion, mensaje_rx.id, mensaje_rx.prioridad);
                #endif
                sem_wait(&(me->sem_peticiones));
                me->peticiones[mensaje_rx.id - 1][mensaje_rx.prioridad - 1] = max(me->peticiones[mensaje_rx.id - 1][mensaje_rx.prioridad - 1], mensaje_rx.peticion);
                sem_post(&(me->sem_peticiones));
                sem_wait(&(me->sem_prioridad_max_otro_nodo));
                me->prioridad_max_otro_nodo = max(me->prioridad_max_otro_nodo, mensaje_rx.prioridad);
                #ifdef __DEBUG
                printf("La prioridad máxima de otro nodo es: %i\n", me->prioridad_max_otro_nodo);
                #endif
                sem_post(&(me->sem_prioridad_max_otro_nodo));
                printf("\n");
                if(mensaje_rx.prioridad != CONSULTAS){
                    sem_wait(&(me->sem_testigo));
                    if (me->testigo){
                        sem_post(&(me->sem_testigo));
                        sem_wait(&(me->sem_prioridad_maxima));
                        if (me->prioridad_maxima < mensaje_rx.prioridad && me->prioridad_maxima != 0) {
                            sem_post(&(me->sem_prioridad_maxima));
                            #ifdef __PRINT_RX
                            printf("RECEPTOR: EL NODO %i TIENE UNA PRIORIDAD MAS ALTA\n", mensaje_rx.id);
                            #endif
                        }else{
                            if (me->prioridad_maxima == 0){
                                sem_post(&(me->sem_prioridad_maxima));
                                send_testigo(mi_id, me);
                            }else{
                                sem_post(&(me->sem_prioridad_maxima));
                            }
                        }
                    }else{
                        sem_post(&(me->sem_testigo));
                    }
                }else{
                    sem_wait(&(me->sem_prioridad_max_otro_nodo));
                    sem_wait(&(me->sem_prioridad_maxima));
                    sem_wait(&(me->sem_testigo));
                    if(me->testigo && ( me->prioridad_max_otro_nodo == CONSULTAS && (me->prioridad_maxima == CONSULTAS || me->prioridad_maxima == 0))){
                        sem_post(&(me->sem_prioridad_max_otro_nodo));
                        sem_post(&(me->sem_prioridad_maxima));
                        sem_post(&(me->sem_testigo));
                        send_copias_testigos(mi_id, me);
                    }else{
                        sem_post(&(me->sem_prioridad_max_otro_nodo));
                        sem_post(&(me->sem_prioridad_maxima));
                        sem_post(&(me->sem_testigo));
                    }
                }
                break;

            case (long)2:// El mensaje es el testigo
                #ifdef __PRINT_RX
                printf("RECEPTOR: He recibido el testigo del nodo: %d\n", mensaje_rx.id);
                #endif
                sem_wait(&(me->sem_prioridad_max_otro_nodo));
                me->prioridad_max_otro_nodo = 0;
                sem_post(&(me->sem_prioridad_max_otro_nodo));
                sem_wait(&(me->sem_atendidas));
                sem_wait(&(me->sem_peticiones));
                // actualizar vector de atendidas y saber cual es la prioridad máxima de otro nodo
                for (i = 0; i < N; i++){
                    if(mi_id != i + 1){
                        for (j = 0; j < P; j++){
                            me->atendidas[i][j] = mensaje_rx.atendidas[i][j];
                        
                            if (me->atendidas[i][j] < me->peticiones[i][j]){
                                sem_wait(&(me->sem_prioridad_max_otro_nodo));
                                me->prioridad_max_otro_nodo = max(me->prioridad_max_otro_nodo, j + 1);
                                printf("La prio max del otro nodo es: %d\n", me->prioridad_max_otro_nodo);
                                printf("atendidas: %d; peticiones: %d\n", me->atendidas[i][j], me->peticiones[i][j]);
                                sem_post(&(me->sem_prioridad_max_otro_nodo));
                            }
                        }
                    }
                }
                for(i = 0; i < N; i++){
                    printf("NODO %d\n", i + 1);
                    printf("\tPeticiones: %d, %d, %d\n", me->peticiones[i][0], me->peticiones[i][1], me->peticiones[i][2]);
                    printf("\tAtendidas : %d, %d, %d\n", me->atendidas[i][0], me->atendidas[i][1], me->atendidas[i][2]);
                }
                sem_post(&(me->sem_atendidas));
                sem_post(&(me->sem_peticiones));
                set_prioridad_max(me);
                sem_wait(&(me->sem_prioridad_max_otro_nodo));
                sem_wait(&(me->sem_prioridad_maxima));
                
                if(me->prioridad_max_otro_nodo > me->prioridad_maxima){
                    sem_post(&(me->sem_prioridad_max_otro_nodo));
                    sem_post(&(me->sem_prioridad_maxima));
                    send_testigo(mi_id, me);
                }else{
                    sem_post(&(me->sem_prioridad_max_otro_nodo));
                    sem_post(&(me->sem_prioridad_maxima));
                    sem_wait(&(me->sem_nodo_master));
                    me->nodo_master = true;
                    sem_post(&(me->sem_nodo_master));
                    sem_wait(&(me->sem_prioridad_max_otro_nodo));
                    #ifdef __DEBUG
                    printf("!!!DEBUG: La prioridad máxima de otro nodo es: %i\n", me->prioridad_max_otro_nodo);
                    #endif
                    sem_post(&(me->sem_prioridad_max_otro_nodo));

                    sem_wait(&(me->sem_contador_anul_pagos_pendientes));
                    if (me->contador_anul_pagos_pendientes > 0){
                        sem_post(&(me->sem_contador_anul_pagos_pendientes));
                        sem_wait(&(me->sem_atendidas));
                        sem_wait(&(me->sem_peticiones));
                        me->atendidas[mi_id - 1][PAGOS_ANUL -1] = me->peticiones[mi_id - 1][PAGOS_ANUL -1];
                        sem_post(&(me->sem_atendidas));
                        sem_post(&(me->sem_peticiones));
                        sem_wait(&(me->sem_contador_procesos_max_SC));
                        sem_wait(&(me->sem_contador_anul_pagos_pendientes));
                        if((me->contador_anul_pagos_pendientes + me->contador_procesos_max_SC - EVITAR_RETECION_EM) > 0){
                            sem_post(&(me->sem_contador_procesos_max_SC));
                            sem_post(&(me->sem_contador_anul_pagos_pendientes));
                            send_peticiones(me, mi_id, PAGOS_ANUL);
                        }else{
                            sem_post(&(me->sem_contador_procesos_max_SC));
                            sem_post(&(me->sem_contador_anul_pagos_pendientes));
                        }
                        sem_wait(&(me->sem_turno_PA));
                        me->turno_PA = true;
                        sem_post(&(me->sem_turno_PA));
                        sem_wait(&(me->sem_turno));
                        me->turno = true;
                        sem_post(&(me->sem_turno));
                        sem_post(&(me->sem_anul_pagos_pend));
                    }else{
                        sem_post(&(me->sem_contador_anul_pagos_pendientes));
                        sem_wait(&(me->sem_contador_reservas_admin_pendientes));
                        if (me->contador_reservas_admin_pendientes > 0){
                            sem_post(&(me->sem_contador_reservas_admin_pendientes));
                            sem_wait(&(me->sem_atendidas));
                            sem_wait(&(me->sem_peticiones));
                            me->atendidas[mi_id - 1][ADMIN_RESER - 1] = me->peticiones[mi_id - 1][ADMIN_RESER - 1];
                            #ifdef __DEBUG
                            printf("\tDEBUG --> atendidas %d, peticiones %d.\n",me->atendidas[mi_id - 1][ADMIN_RESER - 1], me->peticiones[mi_id - 1][ADMIN_RESER - 1]);
                            #endif
                            sem_post(&(me->sem_atendidas));
                            sem_post(&(me->sem_peticiones));
                            sem_wait(&(me->sem_contador_procesos_max_SC));
                            sem_wait(&(me->sem_contador_reservas_admin_pendientes));
                            if((me->contador_reservas_admin_pendientes + me->contador_procesos_max_SC - EVITAR_RETECION_EM) > 0){
                                sem_post(&(me->sem_contador_procesos_max_SC));
                                sem_post(&(me->sem_contador_reservas_admin_pendientes));
                                send_peticiones(me, mi_id, ADMIN_RESER);
                            }else{
                                sem_post(&(me->sem_contador_procesos_max_SC));
                                sem_post(&(me->sem_contador_reservas_admin_pendientes));
                            }
                            sem_wait(&(me->sem_turno_RA));
                            me->turno_RA = true;
                            sem_post(&(me->sem_turno_RA));
                            sem_wait(&(me->sem_turno));
                            me->turno = true;
                            sem_post(&(me->sem_turno));
                            sem_post(&(me->sem_reser_admin_pend));
                        }else{
                            sem_post(&(me->sem_contador_reservas_admin_pendientes));
                            sem_wait(&(me->sem_atendidas));
                            sem_wait(&(me->sem_peticiones));
                            me->atendidas[mi_id - 1][CONSULTAS - 1] = me->peticiones[mi_id - 1][CONSULTAS - 1];
                            #ifdef __DEBUG
                            printf("\tDEBUG --> atendidas %d, peticiones %d.\n",me->atendidas[mi_id - 1][CONSULTAS - 1], me->peticiones[mi_id - 1][CONSULTAS - 1]);
                            #endif
                            sem_post(&(me->sem_atendidas));
                            sem_post(&(me->sem_peticiones));
                            sem_wait(&(me->sem_turno_C));
                            me->turno_C = true;
                            sem_post(&(me->sem_turno_C));
                            sem_wait(&(me->sem_turno));
                            me->turno = true;
                            sem_post(&(me->sem_turno));
                            sem_post(&(me->sem_consult_pend));
                        }
                    }
                    
                }

                
                sem_wait(&(me->sem_testigo));
                me->testigo = true;
                sem_post(&(me->sem_testigo));

                break;
            case (long)3://Recibimos el testigo para consultas
                #ifdef __PRINT_RX
                printf("RECEPTOR: He recibido el testigo FALSO CONSULTAS del nodo: %d\n", mensaje_rx.id);
                #endif
                //Si nos llega el testigo pero hay mas prioridad lo devolvemos
                //si no, turno de consultas a 1
                sem_wait(&(me->sem_prioridad_max_otro_nodo));
                me->prioridad_max_otro_nodo = 0;
                sem_post(&(me->sem_prioridad_max_otro_nodo));
                sem_wait(&(me->sem_atendidas));
                sem_wait(&(me->sem_peticiones));
                // actualizar vector de atendidas y saber cual es la prioridad máxima de otro nodo
                for (i = 0; i < N; i++){
                    if(mi_id != i + 1){
                        for (j = 0; j < P; j++){
                            me->atendidas[i][j] = mensaje_rx.atendidas[i][j];
                        
                            if (me->atendidas[i][j] < me->peticiones[i][j]){
                                sem_wait(&(me->sem_prioridad_max_otro_nodo));
                                me->prioridad_max_otro_nodo = max(me->prioridad_max_otro_nodo, j + 1);
                                printf("La prio max del otro nodo es: %d\n", me->prioridad_max_otro_nodo);
                                printf("atendidas: %d; peticiones: %d\n", me->atendidas[i][j], me->peticiones[i][j]);
                                sem_post(&(me->sem_prioridad_max_otro_nodo));
                            }
                        }
                    }
                }
                for(i = 0; i < N; i++){
                    printf("NODO %d\n", i + 1);
                    printf("\tPeticiones: %d, %d, %d\n", me->peticiones[i][0], me->peticiones[i][1], me->peticiones[i][2]);
                    printf("\tAtendidas : %d, %d, %d\n", me->atendidas[i][0], me->atendidas[i][1], me->atendidas[i][2]);
                }
                sem_post(&(me->sem_atendidas));
                sem_post(&(me->sem_peticiones));
                set_prioridad_max(me);
                sem_wait(&(me->sem_prioridad_max_otro_nodo));
                sem_wait(&(me->sem_prioridad_maxima));
                if(me->prioridad_max_otro_nodo > CONSULTAS || me->prioridad_maxima > CONSULTAS){
                    sem_post(&(me->sem_prioridad_max_otro_nodo));
                    sem_post(&(me->sem_prioridad_maxima));
                    struct msgbuf_mensaje msg_testigo;
                    msg_testigo.msg_type = (long)4;
                    msg_testigo.id = mi_id;
                    msg_testigo.id_nodo_master = mensaje_rx.id_nodo_master;
                    sem_wait(&me->sem_buzones_nodos);
                    if (msgsnd(me->buzones_nodos[msg_testigo.id_nodo_master - 1], &msg_testigo, sizeof(msg_testigo), 0)){
                    printf("PROCESO ENVIO TESTIGO FALSO: \n\n\tERROR: Hubo un error al enviar el testigo.\n");
                    }
                    sem_post(&me->sem_buzones_nodos);
                }else{
                    sem_post(&(me->sem_prioridad_max_otro_nodo));
                    sem_post(&(me->sem_prioridad_maxima));
                    printf("Voy a dar paso a las consultas\n");
                    sem_wait(&(me->sem_turno_C));
                    me->turno_C = true;
                    sem_post(&(me->sem_turno_C));
                    sem_wait(&(me->sem_atendidas));
                    sem_wait(&(me->sem_peticiones));
                    me->atendidas[mi_id - 1][CONSULTAS - 1] = me->peticiones[mi_id - 1][CONSULTAS - 1];
                    sem_post(&(me->sem_atendidas));
                    sem_post(&(me->sem_peticiones));
                    sem_wait(&(me->sem_id_nodo_master));
                    me->id_nodo_master = mensaje_rx.id_nodo_master;
                    sem_post(&(me->sem_id_nodo_master));
                    sem_wait(&(me->sem_contador_consultas_pendientes));
                    for(i = 0; i < me->contador_consultas_pendientes; i++){
                        printf("consultas pend = %d\n", me->contador_consultas_pendientes);
                        sem_post(&(me->sem_consult_pend));
                    }
                    sem_post(&(me->sem_contador_consultas_pendientes));
                }
                
                break;
            case (long)4:
                #ifdef __PRINT_RX
                printf("RECEPTOR: He recibido el testigo CONSULTAS de un nodo NO master del nodo: %d\n", mensaje_rx.id);
                #endif
                sem_wait(&(me->sem_nodos_con_consultas));
                me->nodos_con_consultas[mensaje_rx.id - 1] = 0;
                sem_post(&(me->sem_nodos_con_consultas));
                sem_wait(&(me->sem_testigos_recogidos));
                me->testigos_recogidos = true;
                sem_wait(&(me->sem_nodos_con_consultas));
                me->nodos_con_consultas[mi_id - 1] = 0;
                for(i = 0; i < N; i++){
                if(me->nodos_con_consultas[i] == 0){
                    me->testigos_recogidos = false;
                    break;
                }
                }
                sem_post(&(me->sem_nodos_con_consultas));
                if(me->testigos_recogidos){
                sem_post(&(me->sem_testigos_recogidos));
                send_testigo_consultas_master(mi_id, me);
                }else{
                sem_post(&(me->sem_testigos_recogidos));
                }
                break;
        }
        
    }

    return 0;
}