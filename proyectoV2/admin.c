#include "procesos.h"

int main(int argc, char *argv[]){

    if (argc != 2){
        printf("La forma correcta de ejecución es: %s \"id_nodo\"\n", argv[0]);
        return -1;
    }

    int mi_id = atoi(argv[1]);
    //int i;
    memoria *me = NULL;
    int memoria_id;
    // inicialización memoria compartida
    memoria_id = shmget(mi_id, sizeof(memoria), 0);
    me = shmat(memoria_id, NULL, 0);
    #ifdef __DEBUG
    printf("El id de la memoria compartida es: %i\n", memoria_id);
    #endif

    #ifdef __PRINT_PROCESO
    printf("ADMIN --> Hola\n"); 
    #endif
    sem_wait(&(me->sem_contador_reservas_admin_pendientes));
    me->contador_reservas_admin_pendientes = me->contador_reservas_admin_pendientes + 1;
    sem_wait(&(me->sem_testigo));
    sem_wait(&(me->sem_turno_RA));
    sem_wait(&(me->sem_turno));
    sem_wait(&(me->sem_contador_procesos_max_SC));
    
    if ((!me->testigo && (me-> contador_reservas_admin_pendientes == 1)) || 
         (me->testigo && me->turno_RA && (me->contador_reservas_admin_pendientes + me->contador_procesos_max_SC - EVITAR_RETECION_EM) == 1)
         || (me->testigo && (me-> contador_reservas_admin_pendientes == 1) && !me->turno_RA && me->turno)){ 
        //RAMA DE PEDIR EL TESTIGO
        sem_post(&(me->sem_testigo));
        sem_post(&(me->sem_turno_RA));
        sem_post(&(me->sem_contador_procesos_max_SC));
        sem_post(&(me->sem_turno));
        sem_post(&(me->sem_contador_reservas_admin_pendientes));
        sem_wait(&(me->sem_turno_C));
        me->turno_C = false;
        sem_post(&(me->sem_turno_C));
        set_prioridad_max(me);
        #ifdef __PRINT_PROCESO
        printf("ADMIN --> Tengo que pedir el testigo\n");
        #endif
    
        //Enviamos peticiones
        send_peticiones(me, mi_id, ADMIN_RESER);
        // ACABAMOS CON EL ENVIO DE PETICIONES AHORA ME TOCA ESPERAR.
        printf("ESpero\n");
        sem_wait(&(me->sem_reser_admin_pend));
        printf("salgo\n");
    }else{ // NO TENGO QUE PEDIR EL TESTIGO
        sem_post(&(me->sem_testigo));
        sem_post(&(me->sem_turno_RA));
        sem_post(&(me->sem_turno));
        sem_post(&(me->sem_contador_procesos_max_SC));
        sem_post(&(me->sem_contador_reservas_admin_pendientes));
        #ifdef __PRINT_PROCESO
        printf("ADMIN --> no tengo que pedir el testigo.\n");
        #endif
        sem_wait(&(me->sem_dentro));
        sem_wait(&(me->sem_testigo));
        if ((me->dentro) || !(me->testigo)){ // SI HAY ALGUIEN DENTRO O NO TENGO EL TESTIGO, ESPERO
            sem_post(&(me->sem_dentro));
            sem_post(&(me->sem_testigo));
            #ifdef __PRINT_PROCESO
            printf("ADMIN --> tengo que esperar porque no tengo permiso.\n");
            #endif
            sem_wait(&(me->sem_reser_admin_pend));
        }else{ // SI NO HAY NADIE DENTRO
            sem_post(&(me->sem_dentro));
            sem_post(&(me->sem_testigo));
            sem_wait(&(me->sem_prioridad_maxima));
            me->prioridad_maxima = ADMIN_RESER;
            sem_post(&(me->sem_prioridad_maxima));
            sem_wait(&(me->sem_turno_RA));
            me->turno_RA = true;
            sem_post(&(me->sem_turno_RA));
            sem_wait(&(me->sem_turno));
            me->turno = true;
            sem_post(&(me->sem_turno));
        }
    }
    // SECCIÓN CRÍTICA DE EXCLUSIÓN MUTUA BABY
    #ifdef __PRINT_PROCESO
    printf("ADMIN --> VOY A LA SCEM .\n");
    #endif
    sem_wait(&(me->sem_contador_reservas_admin_pendientes));
    me->contador_reservas_admin_pendientes = me->contador_reservas_admin_pendientes - 1;
    sem_post(&(me->sem_contador_reservas_admin_pendientes));
    sem_wait(&(me->sem_dentro));
    me->dentro = true;
    sem_post(&(me->sem_dentro));
    sem_wait(&(me->sem_contador_procesos_max_SC));
    me->contador_procesos_max_SC = me->contador_procesos_max_SC + 1;
    sem_post(&(me->sem_contador_procesos_max_SC));
    sleep(SLEEP); // tiempo que se queda en la S.C
    #ifdef __PRINT_PROCESO
    printf("ADMIN --> salgo de la SCEM.\n");
    #endif
    
    set_prioridad_max(me);

    sem_wait(&(me->sem_prioridad_max_otro_nodo));
    sem_wait(&(me->sem_prioridad_maxima));
    if(me->prioridad_max_otro_nodo > me->prioridad_maxima){//prioridad máxima en otro nodo
        sem_post(&(me->sem_prioridad_max_otro_nodo));
        sem_post(&(me->sem_prioridad_maxima));
        sem_wait(&(me->sem_turno_RA));
        me->turno_RA = false;
        sem_post(&(me->sem_turno_RA));
        sem_wait(&(me->sem_turno));
        me->turno = false;
        sem_post(&(me->sem_turno));
        sem_wait(&(me->sem_dentro));
        me->dentro = false;
        sem_post(&(me->sem_dentro));
        sem_wait(&(me->sem_prioridad_max_otro_nodo));
        if(me->prioridad_max_otro_nodo == CONSULTAS){
            sem_post(&(me->sem_prioridad_max_otro_nodo));
            send_copias_testigos(mi_id, me);
        }else{
            sem_post(&(me->sem_prioridad_max_otro_nodo));
            send_testigo(mi_id, me);
        }
    }else{
        if((me->prioridad_max_otro_nodo == me->prioridad_maxima) && me->prioridad_max_otro_nodo != 0){
            sem_post(&(me->sem_prioridad_max_otro_nodo));
            sem_post(&(me->sem_prioridad_maxima));
            sem_wait(&(me->sem_contador_procesos_max_SC));
            sem_wait(&(me->sem_contador_reservas_admin_pendientes));
            sem_wait(&(me->sem_prioridad_max_otro_nodo));
            printf("Contador MAX: %d\n", me->contador_procesos_max_SC);
            if (me->contador_procesos_max_SC >= EVITAR_RETECION_EM || (me->contador_reservas_admin_pendientes == 0 && me->prioridad_max_otro_nodo != 0)){
                #ifdef __PRINT_PROCESO
                printf("ADMIN --> Quiero evitar la exclusión mutua o ya no hay procesos de esta prioridad en mi nodo.\n");
                #endif
                sem_post(&(me->sem_prioridad_max_otro_nodo));
                sem_post(&(me->sem_contador_procesos_max_SC));
                sem_post(&(me->sem_contador_reservas_admin_pendientes));
                sem_wait(&(me->sem_turno_RA));
                me->turno_RA = false;
                sem_post(&(me->sem_turno_RA));
                sem_wait(&(me->sem_turno));
                me->turno = false;
                sem_post(&(me->sem_turno));
                send_testigo(mi_id, me);
                sem_wait(&(me->sem_dentro));
                me->dentro = false;
                sem_post(&(me->sem_dentro));
            }else{
                sem_post(&(me->sem_prioridad_max_otro_nodo));
                sem_post(&(me->sem_contador_procesos_max_SC));
                sem_post(&(me->sem_contador_reservas_admin_pendientes));
                sem_post(&(me->sem_reser_admin_pend));
            }
        }else{
            sem_post(&(me->sem_prioridad_max_otro_nodo));
            if(me->prioridad_maxima != 0){
                if (me->prioridad_maxima == PAGOS_ANUL){ // La prioridad mas alta de mi nodo es pagos_anul
            
                    #ifdef __PRINT_PROCESO
                    printf("ADMIN --> le doy paso a mi compañero.\n");
                    #endif
                    sem_post(&(me->sem_prioridad_maxima));
                    sem_wait(&(me->sem_atendidas));
                    sem_wait(&(me->sem_peticiones));
                    me->atendidas[mi_id - 1][PAGOS_ANUL - 1] = me->peticiones[mi_id - 1][PAGOS_ANUL - 1];
                    #ifdef __DEBUG
                    printf("\tDEBUG --> atendidas %d, peticiones %d.\n",me->atendidas[mi_id - 1][PAGOS_ANUL - 1], me->peticiones[mi_id - 1][PAGOS_ANUL - 1]);
                    #endif
                    sem_post(&(me->sem_atendidas));
                    sem_post(&(me->sem_peticiones));
                    sem_wait(&(me->sem_contador_procesos_max_SC));
                    me->contador_procesos_max_SC = 0;
                    sem_post(&(me->sem_contador_procesos_max_SC));
                    sem_wait(&(me->sem_turno_RA));
                    me->turno_RA = false;
                    sem_post(&(me->sem_turno_RA));
                    sem_wait(&(me->sem_turno_PA));
                    me->turno_PA = true;
                    sem_post(&(me->sem_turno_PA));
                    
                    sem_post(&me->sem_anul_pagos_pend);
                }else{
                    if (me->prioridad_maxima == ADMIN_RESER){ // La prioridad mas alta de mi nodo es reservas_admin
                    
                        #ifdef __PRINT_PROCESO
                        printf("ADMIN --> le doy paso a admin_res.\n");
                        #endif
                        sem_post(&(me->sem_prioridad_maxima));
                        
                        
                        sem_post(&(me->sem_reser_admin_pend));
                    }else{ // La prioridad mas alta de mi nodo es consultas
                    
                        #ifdef __PRINT_PROCESO
                        printf("ADMIN --> le doy paso a consultas.\n");
                        #endif
                        sem_post(&(me->sem_prioridad_maxima));
                        sem_wait(&(me->sem_contador_procesos_max_SC));
                        me->contador_procesos_max_SC = 0;
                        sem_post(&(me->sem_contador_procesos_max_SC));
                        sem_wait(&(me->sem_turno_RA));
                        me->turno_RA = false;
                        sem_post(&(me->sem_turno_RA));
                        sem_wait(&(me->sem_turno_C));
                        me->turno_PA = true;
                        sem_post(&(me->sem_turno_C));
                        sem_wait(&(me->sem_atendidas));
                        sem_wait(&(me->sem_peticiones));
                        me->atendidas[mi_id - 1][CONSULTAS - 1] = me->peticiones[mi_id - 1][CONSULTAS - 1];
                        #ifdef __DEBUG
                        printf("\tDEBUG --> atendidas %d, peticiones %d.\n",me->atendidas[mi_id - 1][CONSULTAS - 1], me->peticiones[mi_id - 1][CONSULTAS - 1]);
                        #endif
                        sem_post(&(me->sem_atendidas));
                        sem_post(&(me->sem_peticiones));
                        sem_wait(&(me->sem_nodo_master));
                        me->nodo_master = true;
                        sem_post(&(me->sem_nodo_master));
                        int i;
                        sem_wait(&(me->sem_contador_consultas_pendientes));
                        for(i = 0; i < me->contador_consultas_pendientes; i++){
                            printf("consultas pend = %d\n", me->contador_consultas_pendientes);
                            sem_post(&(me->sem_consult_pend));
                        }
                        sem_post(&(me->sem_contador_consultas_pendientes));
                    }
                }
            }else{
                sem_post(&(me->sem_prioridad_maxima));
                sem_wait(&(me->sem_atendidas));
                sem_wait(&(me->sem_peticiones));
                me->atendidas[mi_id - 1][ADMIN_RESER - 1] = me->peticiones[mi_id - 1][ADMIN_RESER - 1];
                sem_post(&(me->sem_atendidas));
                sem_post(&(me->sem_peticiones));
                sem_wait(&(me->sem_turno_RA));
                me->turno_RA = false;
                sem_post(&(me->sem_turno_RA));
                sem_wait(&(me->sem_turno));
                me->turno = false;
                sem_post(&(me->sem_turno));
            }
        }
    }
    
    return 0;
}
