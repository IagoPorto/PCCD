

#include "procesos.h"

memoria *me;

int main(int argc, char *argv[])
{

    if (argc != 2)
    {
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
    if (mi_id == 1)
    {
#ifdef __PRINT_RX
        printf("Soy el nodo 1\n");
#endif
        me->testigo = true;
        me->tengo_que_pedir_testigo = false;
    }
    else
    {
        me->testigo = false;
        me->tengo_que_pedir_testigo = true;
    }

    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N_PRIO; j++)
        {
            me->atendidas[i][j] = 0;
            me->peticiones[i][j] = 0;
        }
    }

    me->dentro = false;
    me->mi_peticion = 0;
    me->tengo_que_enviar_testigo = false;
    me->contador_anul_pagos_pendientes = 0;
    me->contador_consultas_pendientes = 0;
    me->contador_procesos_max_SC = 0;
    me->contador_reservas_admin_pendientes = 0;
    me->prioridad_max_otro_nodo = 0;
    me->prioridad_maxima = 0;
    // inicialización de semáforos
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
    sem_init(&(me->sem_tengo_que_enviar_testigo), 1, 1);
    sem_init(&(me->sem_tengo_que_pedir_testigo), 1, 1);
    sem_init(&(me->sem_prioridad_maxima), 1, 1);
    sem_init(&(me->sem_contador_procesos_max_SC), 1, 1);
    sem_init(&(me->sem_prioridad_max_otro_nodo), 1, 1);
    sem_init(&(me->sem_dentro), 1, 1);
    sem_init(&(me->sem_atendidas), 1, 1);
    sem_init(&(me->sem_peticiones), 1, 1);
    sem_init(&(me->sem_buzones_nodos), 1, 1);

    // INICIALIZACIÓN DE BUZONES NODOS
    //  INICIALIZACIÓN BUZONES DE LOS NODOS.
    for (i = 0; i < N; i++)
    {

        me->buzones_nodos[i] = msgget(i + 1, IPC_CREAT | 0777);
#ifdef __DEBUG
        printf("El id del buzón del nodo %i es: %i.\n", i + 1, me->buzones_nodos[i]);
#endif

        if (me->buzones_nodos[i] == -1)
        {

            perror("No se creó el mensaje\n");
        }
    }
    // INICIO RX!!!!!!!!!!!!!!!!!
    struct msgbuf_mensaje mensaje_rx;
    sem_wait(&(me->sem_buzones_nodos));
    int id_de_mi_buzon = me->buzones_nodos[mi_id - 1];
    sem_post(&(me->sem_buzones_nodos));

    while (true)
    {

        // RECIBIMOS PETICIÓN
        if (msgrcv(id_de_mi_buzon, &mensaje_rx, sizeof(mensaje_rx), 0, 0) == -1)
        {
            printf("Proceso Rx: ERROR: Hubo un error al recibir un mensaje en el RECEPTOR.\n");
        }

        // ACTUALIZO EL VALOR DE PETICIONES CON LA QUE ME ACABA DE LLEGAR
        switch (mensaje_rx.msg_type)
        {
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
            printf("La prioridad máxima de otro nodo es: %i", me->prioridad_max_otro_nodo);
#endif
            sem_post(&(me->sem_prioridad_max_otro_nodo));
            printf("\n");

            sem_wait(&(me->sem_testigo));
            if (me->testigo)
            {
                sem_post(&(me->sem_testigo));
                sem_wait(&(me->sem_prioridad_maxima));
                if (me->prioridad_maxima < mensaje_rx.prioridad && me->prioridad_maxima != 0)
                {
                    sem_post(&(me->sem_prioridad_maxima));
#ifdef __PRINT_RX
                    printf("RECEPTOR: EL NODO %i TIENE UNA PRIORIDAD MAS ALTA\n", mensaje_rx.id);
#endif
                    sem_wait(&(me->sem_tengo_que_pedir_testigo));
                    me->tengo_que_pedir_testigo = true;
                    sem_post(&(me->sem_tengo_que_pedir_testigo));
                    sem_wait(&(me->sem_tengo_que_enviar_testigo));
                    me->tengo_que_enviar_testigo = true;
                    sem_post(&(me->sem_tengo_que_enviar_testigo));
                }
                else
                {
                    if (me->prioridad_maxima == 0)
                    {
                        sem_post(&(me->sem_prioridad_maxima));
                        send_testigo(mi_id, me);
                        sem_wait(&(me->sem_tengo_que_pedir_testigo));
                        me->tengo_que_pedir_testigo = true;
                        sem_post(&(me->sem_tengo_que_pedir_testigo));
                        sem_wait(&(me->sem_tengo_que_enviar_testigo));
                        me->tengo_que_enviar_testigo = false;
                        sem_post(&(me->sem_tengo_que_enviar_testigo));
                    }
                    else
                    {
                        sem_post(&(me->sem_prioridad_maxima));
                    }
                }
            }
            else
            {
                sem_post(&(me->sem_testigo));
            }
            break;

        case (long)2:
// El mensaje es el testigo
#ifdef __PRINT_RX
            printf("RECEPTOR: He recibido el testigo del nodo: %d\n", mensaje_rx.id);
#endif
            sem_wait(&(me->sem_prioridad_max_otro_nodo));
            me->prioridad_max_otro_nodo = 0;
            sem_post(&(me->sem_prioridad_max_otro_nodo));
            sem_wait(&(me->sem_atendidas));
            sem_wait(&(me->sem_peticiones));
            // actualizar vector de atendidas y saber cual es la prioridad máxima de otro nodo
            for (i = 0; i < N; i++)
            {
                for (j = 0; j < N_PRIO; j++)
                {
                    me->atendidas[i][j] = mensaje_rx.atendidas[i][j];
                }
                if (me->atendidas[i][j] < me->peticiones[i][j])
                {
                    sem_wait(&(me->sem_prioridad_max_otro_nodo));
                    me->prioridad_max_otro_nodo = max(me->prioridad_max_otro_nodo, j);
                    sem_post(&(me->sem_prioridad_max_otro_nodo));
                }
            }
            sem_post(&(me->sem_atendidas));
            sem_post(&(me->sem_peticiones));

            sem_wait(&(me->sem_contador_anul_pagos_pendientes));
            if (me->contador_anul_pagos_pendientes > 0)
            {
                sem_post(&(me->sem_contador_anul_pagos_pendientes));
                sem_post(&(me->sem_anul_pagos_pend));
            }
            else
            {
                sem_post(&(me->sem_contador_anul_pagos_pendientes));
                sem_wait(&(me->sem_contador_reservas_admin_pendientes));
                if (me->contador_reservas_admin_pendientes > 0)
                {
                    sem_post(&(me->sem_contador_reservas_admin_pendientes));
                    sem_post(&(me->sem_reser_admin_pend));
                }
                else
                {
                    sem_post(&(me->sem_contador_reservas_admin_pendientes));
                    sem_post(&(me->sem_consult_pend));
                }
            }
            sem_wait(&(me->sem_testigo));
            me->testigo = true;
            sem_post(&(me->sem_testigo));

            break;
        case (long)3:
#ifdef __PRINT_RX
            printf("RECEPTOR: He recibido el testigo CONSULTAS del nodo: %d\n", mensaje_rx.id);
#endif
            break;
        }
    }

    return 0;
}