#include "procesos.h"

memoria *me;

int main(int argc, char *argv[])
{

    if (argc != 2)
    {
        printf("La forma correcta de ejecución es: %s \"id_nodo\"\n", argv[0]);
        return -1;
    }

    int mi_id = atoi(argv[1]);
    int i;
    memoria *me;
    int memoria_id;
    // inicialización memoria compartida
    memoria_id = shmget(mi_id, sizeof(memoria), 0666 | IPC_CREAT);
    me = shmat(memoria_id, NULL, 0);

    while (1)
    {

#ifdef __PRINT_PROCESO
        printf("PAGOS --> Hola\n");
#endif

        sem_wait(&(me->sem_tengo_que_pedir_testigo));
        sem_wait(&(me->sem_prioridad_maxima));
        if (me->tengo_que_pedir_testigo || me->prioridad_maxima < PAGOS_ANUL)
        { // Rama de pedir testigo

#ifdef __PRINT_PROCESO
            printf("PAGOS --> Tengo que pedir el testigo");
#endif
            me->prioridad_maxima = PAGOS_ANUL;
            sem_post(&(me->sem_prioridad_maxima));
            me->tengo_que_pedir_testigo = false;
            sem_post(&(me->sem_tengo_que_pedir_testigo));

            struct msgbuf_mensaje solicitud;
            sem_wait(&(me->sem_mi_peticion));
            me->mi_peticion++;
            solicitud.peticion = me->mi_peticion;
            sem_post(&(me->sem_mi_peticion));
            solicitud.msg_type = (long)1;
            solicitud.id = mi_id;
            solicitud.prioridad = PAGOS_ANUL;
            sem_wait(&(me->sem_peticiones));
            me->peticiones[mi_id - 1][0] = solicitud.peticion;
            me->peticiones[mi_id - 1][1] = solicitud.prioridad;
            sem_post(&(me->sem_peticiones));

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
                        printf("PAGOS:\n\tERROR: Hubo un error enviando el mensaje al nodo: %i.\n", i);
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
            sem_post(&(me->sem_tengo_que_pedir_testigo));
            sem_post(&(me->sem_prioridad_maxima));
            sem_wait(&(me->sem_dentro));
            sem_wait(&(me->sem_testigo));
            if ((me->dentro) || !(me->testigo))
            { // SI HAY ALGUIEN DENTRO Y NO TENGO QUE PEDIR TESTIGO Y NO TENGO EL TESTIGO, ESPERO
#ifdef __PRINT_PROCESO
                print("PAGOS --> tengo que esperar porque no tengo permiso.\n");
#endif
                sem_post(&(me->sem_dentro));
                sem_post(&(me->sem_testigo));
                sem_wait(&(me->sem_contador_anul_pagos_pendientes));
                me->contador_anul_pagos_pendientes++;
                sem_post(&(me->sem_contador_anul_pagos_pendientes));
                sem_wait(&(me->sem_anul_pagos_pend));
                sem_wait(&(me->sem_contador_anul_pagos_pendientes));
                me->contador_anul_pagos_pendientes--;
                sem_post(&(me->sem_contador_anul_pagos_pendientes));
            }
            else
            { // SI NO HAY NADIE DENTRO
                sem_post(&(me->dentro));
                sem_post(&(me->sem_testigo));
            }
        }
        // SECCIÓN CRÍTICA DE EXCLUSIÓN MUTUA BABY
#ifdef __PRINT_PROCESO
        print("PAGOS --> VOY A LA SCEM BABY.\n");
#endif
        sem_wait(&(me->sem_dentro));
        me->dentro = true;
        sem_post(&(me->sem_dentro));
        sem_wait(&(me->sem_contador_procesos_max_SC));
        me->contador_procesos_max_SC++;
        sem_post(&(me->sem_contador_procesos_max_SC));
        sleep(SLEEP); // tiempo que se queda en la S.C
        set_prioridad_max();

        sem_wait(&(me->sem_tengo_que_enviar_testigo));
        if (me->tengo_que_enviar_testigo)
        { // Prioridad maxima en otro nodo
            sem_post(&(me->sem_tengo_que_enviar_testigo));
            send_testigo(mi_id);
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
                sem_post(&(me->sem_prioridad_maxima));
                sem_post(&(me->sem_prioridad_max_otro_nodo));
                sem_wait(&(me->sem_contador_anul_pagos_pendientes));
                if (me->contador_anul_pagos_pendientes > 0) // La prioridad mas alta de mi nodo es pagos_anul
                {
                    sem_post(&(me->sem_contador_anul_pagos_pendientes));
                    sem_post(&(me->sem_anul_pagos_pend));
                }
                else
                {
                    sem_post(&(me->sem_contador_anul_pagos_pendientes));
                    sem_wait(&(me->sem_contador_reservas_admin_pendientes));
                    if (me->contador_reservas_admin_pendientes > 0) // La prioridad mas alta de mi nodo es reservas_admin
                    {
                        sem_post(&(me->sem_contador_reservas_admin_pendientes));
                        sem_post(&(me->sem_reser_admin_pend));
                        sem_wait(&(me->sem_prioridad_maxima));
                        me->prioridad_maxima = ADMIN_RESER;
                        sem_post(&(me->sem_prioridad_maxima));
                        sem_wait(&(me->sem_contador_procesos_max_SC));
                        me->contador_procesos_max_SC = 0;
                        sem_post(&(me->sem_contador_procesos_max_SC));
                    }
                    else // La prioridad mas alta de mi nodo es consultas
                    {
                        sem_post(&(me->sem_contador_reservas_admin_pendientes));
                        sem_wait(&(me->sem_prioridad_maxima));
                        me->prioridad_maxima = CONSULTAS;
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
                sem_post(&(me->sem_prioridad_maxima));
                sem_post(&(me->sem_prioridad_max_otro_nodo));
                sem_wait(&(me->sem_contador_procesos_max_SC));
                sem_wait(&(me->sem_contador_anul_pagos_pendientes));
                if (me->contador_procesos_max_SC >= EVITAR_RETECION_EM || me->contador_anul_pagos_pendientes == 0)
                {
                    sem_post(&(me->sem_contador_procesos_max_SC));
                    sem_post(&(me->sem_contador_anul_pagos_pendientes));
                    send_testigo(mi_id);
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
                    sem_post(&(me->sem_contador_anul_pagos_pendientes));
                }
            }
        } // Si no hay nadie me voy
    }

    return 0;
}