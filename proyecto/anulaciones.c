#include "procesos.h"

int main(int argc, char *argv[])
{

    if (argc != 2)
    {
        printf("La forma correcta de ejecución es: %s \"id_nodo\"\n", argv[0]);
        return -1;
    }

    int mi_id = atoi(argv[1]);
    int i;
    memoria *me = NULL;
    int memoria_id;
    // inicialización memoria compartida
    memoria_id = shmget(mi_id, sizeof(memoria), 0);
    me = shmat(memoria_id, NULL, 0);
#ifdef __DEBUG
    printf("El id de la memoria compartida es: %i\n", memoria_id);
#endif

#ifdef __PRINT_PROCESO
    printf("ANULACIONES --> Hola\n");
#endif

    sem_wait(&(me->sem_tengo_que_pedir_testigo));
    sem_wait(&(me->sem_prioridad_maxima));
    me->prioridad_maxima = PAGOS_ANUL;
    sem_wait(&(me->sem_testigo));
    if (me->tengo_que_pedir_testigo && !me->testigo)
    { // Rama de pedir testigo
        sem_post(&(me->sem_testigo));
#ifdef __PRINT_PROCESO
        printf("ANULACIONES --> Tengo que pedir el testigo\n");
#endif

        sem_post(&(me->sem_prioridad_maxima));
        me->tengo_que_pedir_testigo = false;
        sem_post(&(me->sem_tengo_que_pedir_testigo));
        struct msgbuf_mensaje solicitud;
        sem_wait(&(me->sem_mi_peticion));
        me->mi_peticion = me->mi_peticion + 1;
        sem_wait(&(me->sem_peticiones));
        me->peticiones[mi_id - 1][PAGOS_ANUL - 1] = me->mi_peticion;
        sem_post(&(me->sem_peticiones));
        solicitud.peticion = me->mi_peticion;
        sem_post(&(me->sem_mi_peticion));
        solicitud.msg_type = (long)1;
        solicitud.id = mi_id;
        solicitud.prioridad = PAGOS_ANUL;

#ifdef __DEBUG
        printf("El mensaje es de tipo: %ld, con peticion: %i, con id: %i y prioridad: %i\n",
               solicitud.msg_type, solicitud.peticion, solicitud.id, solicitud.prioridad);
#endif

        // ENVIO PETICIONES
        for (i = 0; i < N; i++)
        {

            if (mi_id - 1 == i)
            {
                continue;
            }
            else
            {

                sem_wait(&(me->sem_buzones_nodos));
                // sem_wait(&sem_msg_solicitud);
                if (msgsnd(me->buzones_nodos[i], &solicitud, sizeof(solicitud), 0) == -1)
                {
                    // sem_post(&sem_msg_solicitud);
                    sem_post(&(me->sem_buzones_nodos));
#ifdef __DEBUG
                    printf("ANULACIONES:\n\tERROR: Hubo un error enviando el mensaje al nodo: %i.\n", i);
#endif
                }
                else
                {
                    sem_post(&(me->sem_buzones_nodos));
                }
            }
        }
        // ACABAMOS CON EL ENVIO DE PETICIONES AHORA ME TOCA ESPERAR.
        sem_wait(&(me->sem_contador_anul_pagos_pendientes));
        me->contador_anul_pagos_pendientes++;
        sem_post(&(me->sem_contador_anul_pagos_pendientes));
        sem_wait(&(me->sem_anul_pagos_pend));
        sem_wait(&(me->sem_contador_anul_pagos_pendientes));
        me->contador_anul_pagos_pendientes--;
        sem_post(&(me->sem_contador_anul_pagos_pendientes));
    }
    else // NO TENGO QUE PEDIR EL TESTIGO
    {
#ifdef __PRINT_PROCESO
        printf("ANULACIONES --> no tengo que pedir el testigo.\n");
#endif
        sem_post(&(me->sem_tengo_que_pedir_testigo));
        sem_post(&(me->sem_prioridad_maxima));
        sem_wait(&(me->sem_dentro));
        if ((me->dentro) || !(me->testigo))
        { // SI HAY ALGUIEN DENTRO O NO TENGO EL TESTIGO, ESPERO
#ifdef __PRINT_PROCESO
            printf("ANULACIONES --> tengo que esperar porque no tengo permiso.\n");
#endif
            sem_post(&(me->sem_dentro));
            sem_post(&(me->sem_testigo));
            sem_wait(&(me->sem_contador_anul_pagos_pendientes));
            me->contador_anul_pagos_pendientes++;
            sem_post(&(me->sem_contador_anul_pagos_pendientes));
#ifdef __DEBUG
            printf("ANULACIONES --> Me quedo aquí.\n");
#endif
            sem_wait(&(me->sem_anul_pagos_pend));
#ifdef __DEBUG
            printf("ANULACIONES --> Me voy de aquí.\n");
#endif
            sem_wait(&(me->sem_contador_anul_pagos_pendientes));
            me->contador_anul_pagos_pendientes--;
            sem_post(&(me->sem_contador_anul_pagos_pendientes));
        }
        else
        { // SI NO HAY NADIE DENTRO
            sem_post(&(me->sem_dentro));
            sem_post(&(me->sem_testigo));
        }
    }
    // SECCIÓN CRÍTICA DE EXCLUSIÓN MUTUA BABY
#ifdef __PRINT_PROCESO
    printf("ANULACIONES --> VOY A LA SCEM .\n");
#endif
    sem_wait(&(me->sem_dentro));
    me->dentro = true;
    sem_post(&(me->sem_dentro));
    sem_wait(&(me->sem_contador_procesos_max_SC));
    me->contador_procesos_max_SC++;
    sem_post(&(me->sem_contador_procesos_max_SC));
    sleep(SLEEP); // tiempo que se queda en la S.C
#ifdef __PRINT_PROCESO
    printf("ANULACIONES --> salgo de la SCEM.\n");
#endif
    set_prioridad_max(me);

    sem_wait(&(me->sem_tengo_que_enviar_testigo));
    if (me->tengo_que_enviar_testigo)
    { // Prioridad maxima en otro nodo
#ifdef __PRINT_PROCESO
        printf("ANULACIONES --> La prioridad maxima está en otro nodo, tengo que enviar el testigo.\n");
#endif
        me->tengo_que_enviar_testigo = false;
        sem_post(&(me->sem_tengo_que_enviar_testigo));
        sem_wait(&(me->sem_contador_anul_pagos_pendientes));
        if (me->contador_anul_pagos_pendientes > 0)
        {
            sem_post(&(me->sem_contador_anul_pagos_pendientes));
            sem_wait(&(me->sem_atendidas));
            sem_wait(&(me->sem_peticiones));
            me->atendidas[mi_id - 1][PAGOS_ANUL - 1] = me->peticiones[mi_id - 1][PAGOS_ANUL - 1];
            sem_post(&(me->sem_atendidas));
            sem_post(&(me->sem_peticiones));
        }
        else
        {
            sem_post(&(me->sem_contador_anul_pagos_pendientes));
        }
        send_testigo(mi_id, me);
        sem_wait(&(me->sem_dentro));
        me->dentro = false;
        sem_post(&(me->sem_dentro));
    }
    else
    {
        sem_post(&(me->sem_tengo_que_enviar_testigo));
        sem_wait(&(me->sem_prioridad_max_otro_nodo));
        sem_wait(&(me->sem_prioridad_maxima));
        if (me->prioridad_max_otro_nodo < me->prioridad_maxima)
        { // Prioridad maxima en mi nodo
#ifdef __PRINT_PROCESO
            printf("ANULACIONES --> la prioridad maxima está en mi nodo.\n");
#endif
            sem_post(&(me->sem_prioridad_maxima));
            sem_post(&(me->sem_prioridad_max_otro_nodo));
            sem_wait(&(me->sem_prioridad_maxima));
#ifdef __DEBUG
            printf("DEBUG: La prioridad max de mi nodo es %i y PAGOS_ANUL es %i.\n", me->prioridad_maxima, PAGOS_ANUL);
#endif
            if (me->prioridad_maxima == PAGOS_ANUL) // La prioridad mas alta de mi nodo es pagos_anul
            {
#ifdef __PRINT_PROCESO
                printf("ANULACIONES --> le doy paso a mi compañero.\n");
#endif
                sem_post(&(me->sem_prioridad_maxima));
                sem_post(&me->sem_anul_pagos_pend);
            }
            else
            {
                if (me->prioridad_maxima == ADMIN_RESER) // La prioridad mas alta de mi nodo es reservas_admin
                {
#ifdef __PRINT_PROCESO
                    printf("ANULACIONES --> le doy paso a admin_res.\n");
#endif
                    sem_post(&(me->sem_prioridad_maxima));
                    sem_post(&(me->sem_reser_admin_pend));
                    sem_wait(&(me->sem_contador_procesos_max_SC));
                    me->contador_procesos_max_SC = 0;
                    sem_post(&(me->sem_contador_procesos_max_SC));
                }
                else // La prioridad mas alta de mi nodo es consultas
                {
#ifdef __PRINT_PROCESO
                    printf("ANULACIONES --> le doy paso a consultas.\n");
#endif
                    sem_post(&(me->sem_prioridad_maxima));
                    sem_wait(&(me->sem_contador_procesos_max_SC));
                    me->contador_procesos_max_SC = 0;
                    sem_post(&(me->sem_contador_procesos_max_SC));
                    // FALTA PONER EL CASO DE CONSULTAS
                }
            }
        }
        else
        { // misma prioridad mi nodo y otro nodo
#ifdef __PRINT_PROCESO
            printf("ANULACIONES --> Mi nodo y otro nodo tenemos la misma prioridad.\n");
#endif
            sem_post(&(me->sem_prioridad_maxima));
            sem_post(&(me->sem_prioridad_max_otro_nodo));
            sem_wait(&(me->sem_contador_procesos_max_SC));
            sem_wait(&(me->sem_contador_anul_pagos_pendientes));
            if (me->contador_procesos_max_SC >= EVITAR_RETECION_EM || me->contador_anul_pagos_pendientes == 0)
            {
#ifdef __PRINT_PROCESO
                printf("ANULACIONES --> Quiero evitar la exclusión mutua o ya no procesos de esta prioridad en mi nodo.\n");
#endif
                sem_post(&(me->sem_contador_procesos_max_SC));
                sem_post(&(me->sem_contador_anul_pagos_pendientes));
                sem_wait(&(me->sem_contador_anul_pagos_pendientes));
                if (me->contador_anul_pagos_pendientes > 0)
                {
                    sem_post(&(me->sem_contador_anul_pagos_pendientes));
                    sem_wait(&(me->sem_atendidas));
                    sem_wait(&(me->sem_peticiones));
                    me->atendidas[mi_id - 1][PAGOS_ANUL - 1] = me->peticiones[mi_id - 1][PAGOS_ANUL - 1];
                    sem_post(&(me->sem_atendidas));
                    sem_post(&(me->sem_peticiones));
                }
                else
                {
                    sem_post(&(me->sem_contador_anul_pagos_pendientes));
                }
                send_testigo(mi_id, me);
                sem_wait(&(me->sem_dentro));
                me->dentro = false;
                sem_post(&(me->sem_dentro));
                sem_wait(&(me->sem_contador_procesos_max_SC));
                me->contador_procesos_max_SC = 0;
                sem_post(&(me->sem_contador_procesos_max_SC));
            }
            else
            {
                sem_post(&(me->sem_contador_procesos_max_SC));
                sem_post(&(me->sem_contador_anul_pagos_pendientes));
                sem_post(&(me->sem_anul_pagos_pend));
            }
        }
    } // Si no hay nadie me voy

    return 0;
}
