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
    printf("CONSULTAS --> Hola\n"); 
    #endif
    sleep(3);
    sem_wait(&(me->sem_contador_consultas_pendientes));
    me->contador_consultas_pendientes = me->contador_consultas_pendientes + 1;
    sem_wait(&(me->sem_testigo));
    sem_wait(&(me->sem_turno_C));
    sem_wait(&(me->sem_turno));
    
    if ((!me->testigo && (me-> contador_consultas_pendientes == 1)) ||
         (me->testigo && (me-> contador_consultas_pendientes == 1) && !me->turno_C && me->turno)){ 
        //RAMA DE PEDIR EL TESTIGO
        sem_post(&(me->sem_testigo));
        sem_post(&(me->sem_turno_C));
        sem_post(&(me->sem_turno));
        sem_post(&(me->sem_contador_consultas_pendientes));
        set_prioridad_max(me);
        #ifdef __PRINT_PROCESO
        printf("CONSULTAS --> Tengo que pedir el testigo\n");
        #endif
    
        //Enviamos peticiones
        send_peticiones(me, mi_id, CONSULTAS);
        // ACABAMOS CON EL ENVIO DE PETICIONES AHORA ME TOCA ESPERAR.
        printf("Espero\n");
        sem_wait(&(me->sem_consult_pend));
        
        
    }else{ // NO TENGO QUE PEDIR EL TESTIGO
        sem_post(&(me->sem_testigo));
        sem_post(&(me->sem_turno_C));
        sem_post(&(me->sem_turno));
        sem_post(&(me->sem_contador_consultas_pendientes));
        #ifdef __PRINT_PROCESO
        printf("CONSULTAS --> no tengo que pedir el testigo.\n");
        #endif
        sem_wait(&(me->sem_testigo));
        sem_wait(&(me->sem_turno_C));
        if ((!(me->testigo) && !me->turno_C) || (!me->turno_C)){ // SI HAY ALGUIEN DENTRO O NO TENGO EL TESTIGO, ESPERO
            sem_post(&(me->sem_testigo));
            sem_post(&(me->sem_turno_C));
            sem_wait(&(me->sem_contador_consultas_pendientes));
            if(me->contador_consultas_pendientes == 1){
                sem_post(&(me->sem_contador_consultas_pendientes));
                set_prioridad_max(me);
                sem_wait(&(me->sem_prioridad_maxima));
                sem_wait(&(me->sem_testigo));
                if(me->testigo && (me->prioridad_maxima == CONSULTAS)){
                    sem_post(&(me->sem_prioridad_maxima));
                    sem_post(&(me->sem_testigo));
                    sem_wait(&(me->sem_turno_C));
                    me->turno_C = true;
                    sem_post(&(me->sem_turno_C));
                    sem_wait(&(me->sem_turno));
                    me->turno = true;
                    sem_post(&(me->sem_turno));
                    sem_wait(&(me->sem_dentro));
                    me->dentro = true;
                    sem_post(&(me->sem_dentro));
                    sem_wait(&(me->sem_nodos_con_consultas));
                    me->nodos_con_consultas[mi_id - 1] = 1;
                    sem_post(&(me->sem_nodos_con_consultas));
                }else{
                    sem_post(&(me->sem_prioridad_maxima));
                    sem_post(&(me->sem_testigo));
                    #ifdef __PRINT_PROCESO
                    printf("CONSULTAS --> tengo que esperar porque no tengo permiso.\n");
                    #endif
                    sem_wait(&(me->sem_consult_pend));
                }
            }else{
                sem_post(&(me->sem_contador_consultas_pendientes));
                #ifdef __PRINT_PROCESO
                printf("CONSULTAS --> tengo que esperar porque no tengo permiso.\n");
                #endif
                sem_wait(&(me->sem_consult_pend));
            }
            
            
        }else{ // SI NO HAY NADIE DENTRO
            sem_post(&(me->sem_testigo));
            sem_post(&(me->sem_turno_C));
        }
    }
    // SECCIÓN CRÍTICA DE CONCURRENCIA ENTRE CONSULTAS BABY
    #ifdef __PRINT_PROCESO
    printf("CONSULTAS --> VOY A LA SC CONCURRENTE DE CONSULTAS .\n");
    #endif
    sem_wait(&(me->sem_dentro_C));
    me->dentro_C = me->dentro_C + 1;
    sem_post(&(me->sem_dentro_C));
    sleep(SLEEP); // tiempo que se queda en la S.C
    #ifdef __PRINT_PROCESO
    printf("CONSULTAS --> salgo de la SCEM.\n");
    #endif
    sem_wait(&(me->sem_contador_consultas_pendientes));
    me->contador_consultas_pendientes = me->contador_consultas_pendientes - 1;
    sem_post(&(me->sem_contador_consultas_pendientes));
    sem_wait(&(me->sem_dentro_C));
    me->dentro_C = me->dentro_C - 1;
    sem_post(&(me->sem_dentro_C));
    
    sem_wait(&(me->sem_turno_C));
    sem_wait(&(me->sem_contador_consultas_pendientes));
    sem_wait(&(me->sem_dentro_C));
    if((!me->turno_C && (me->dentro_C == 0)) || (me->contador_consultas_pendientes == 0)){
        sem_post(&(me->sem_turno_C));
        sem_post(&(me->sem_contador_consultas_pendientes));
        sem_post(&(me->sem_dentro_C));
        #ifdef __PRINT_PROCESO
        printf("CONSULTAS --> soy el último y envio el testigo.\n");
        #endif
        
        sem_wait(&(me->sem_dentro));
        me->dentro = false;
        sem_post(&(me->sem_dentro));
        sem_wait(&(me->sem_turno_C));
        me->turno_C = false;
        sem_post(&(me->sem_turno_C));
        sem_wait(&(me->sem_turno));
        me->turno = false;
        sem_post(&(me->sem_turno));
        send_testigo_consultas(mi_id, me);
        #ifdef __PRINT_PROCESO
        printf("CONSULTAS --> Chao.\n");
        #endif
    }else{
        sem_post(&(me->sem_turno_C));
        sem_post(&(me->sem_contador_consultas_pendientes));
        sem_post(&(me->sem_dentro_C));
        #ifdef __PRINT_PROCESO
        printf("CONSULTAS --> Chao.\n");
        #endif
        

    }
    return 0;
}