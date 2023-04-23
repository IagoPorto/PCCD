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
        if (me->tengo_que_pedir_testigo)
        { // Rama de pedir testigo

#ifdef __PRINT_PROCESO
            printf("PAGOS --> Tengo que pedir el testigo");
#endif

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
        }
        else // NO TENGO QUE PEDIR EL TESTIGO
        {
            sem_wait(&(me->sem_testigo));
            if (me->testigo)
            {
                sem_post(&(me->sem_testigo));

                ////////////////////////////////////////////////////////////////////
                ///////////////////////////////////////////////////////////////////
                ////////////////////////////////////////////////////////////////////
                // FALTAN COSAS
                ///////////////////////////////////////////////////7
                /////////////////////////////////////////////////////////////////
            }
            else
            {
                sem_post(&(me->sem_testigo));
            }
            sem_post(&(me->sem_tengo_que_pedir_testigo));
            sem_wait(&(me->sem_permiso_para_SCEM_anulpagos));
            if (!(me->permiso_para_SCEM_anulpagos))
            { // SI NO TENGO PERMISO Y NO TENGO QUE PEDIR TESTIGO, ESPERO
#ifdef __PRINT_PROCESO
                print("PAGOS --> tengo que esperar porque no tengo permiso.\n");
#endif
                sem_post(&(me->sem_permiso_para_SCEM_anulpagos));
                sem_wait(&(me->sem_contador_anul_pagos_pendientes));
                me->contador_anul_pagos_pendientes++;
                sem_post(&(me->sem_contador_anul_pagos_pendientes));
                sem_wait(&(me->sem_anul_pagos_pend));
                // FALTAN COSAS HASTA AQUÍ
                ////////////////////////////////////////////////////////////////////
                ///////////////////////////////////////////////////////////////////
                ////////////////////////////////////////////////////////////////////
            }
            else
            { // SI TENGO PERMISO VOY
                sem_post(&(me->sem_permiso_para_SCEM_anulpagos));
            }
        }
        // SECCIÓN CRÍTICA DE EXCLUSIÓN MUTUA BABY
#ifdef __PRINT_PROCESO
        print("PAGOS --> VOY A LA SCEM BABY.\n");
#endif

        sem_wait(&(me->sem_contador_procesos_SC));
        me->contador_procesos_SC++;
        sem_post(&(me->sem_contador_procesos_SC));
        sem_wait(&(me->sem_SCEM));
        sleep(SLEEP);
        sem_post(&(me->sem_SCEM));
        sem_wait(&(me->sem_contador_procesos_SC));
        me->contador_procesos_SC--;
        if (me->contador_procesos_SC == 0)
        {
            sem_post(&(me->sem_contador_procesos_SC));
            send_testigo(mi_id);
            /////////////////////////////////////////////////////////////////////////
            ////////////////////////////////////////////////////////////////////////
            // FALTAN COSAS
        }
        else
        {
            sem_post(&(me->sem_contador_procesos_SC));
        }
        sem_post(&(me->sem_contador_procesos_SC));
        sem_wait(&(me->sem_tengo_que_enviar_testigo));
        if (me->tengo_que_enviar_testigo)
        {
            sem_post(&(me->sem_tengo_que_enviar_testigo));
        }
        else
        {
            sem_post(&(me->sem_tengo_que_enviar_testigo));
        }
    }

    return 0;
}