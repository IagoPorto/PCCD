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
    int i;
    memoria *me;
    int memoria_id;
    // inicialización memoria compartida
    memoria_id = shmget(mi_id, sizeof(memoria), 0666 | IPC_CREAT);
    me = shmat(memoria_id, NULL, 0);
    // inicialización de variables memoria compartida
    if (mi_id == 1)
    {
        me->testigo = true;
        me->tengo_que_pedir_testigo = false;
    }
    else
    {
        me->testigo = false;
        me->tengo_que_pedir_testigo = true;
    }
    me->permiso_para_SCEM_anulpagos = false;
    me->permiso_para_SCEM_cons = false;
    me->permiso_para_SCEM_resadmin = false;
    me->mi_peticion = 0;
    me->tengo_que_enviar_testigo = false;
    me->contador_anul_pagos_pendientes = 0;
    me->contador_consultas_pendientes = 0;
    me->contador_procesos_SC = 0;
    me->contador_reservas_admin_pendientes = 0;
    me->prioridad_maxima = 0;
    // inicialización de semáforos
    // inicialización semáforos de paso.
    sem_init(&(me->sem_anul_pagos_pend), 0, 0);
    sem_init(&(me->sem_reser_admin_pend), 0, 0);
    sem_init(&(me->sem_consult_pend), 0, 0);
    // inicialización semáforos exclusión mutua
    sem_init(&(me->sem_consult_pend), 0, 1);
    sem_init(&(me->sem_anul_pagos_pend), 0, 1);
    sem_init(&(me->sem_reser_admin_pend), 0, 1);
    sem_init(&(me->sem_contador_procesos_SC), 0, 1);
    sem_init(&(me->sem_contador_anul_pagos_pendientes), 0, 1);
    sem_init(&(me->sem_contador_reservas_admin_pendientes), 0, 1);
    sem_init(&(me->sem_contador_consultas_pendientes), 0, 1);
    sem_init(&(me->sem_mi_peticion), 0, 1);
    sem_init(&(me->sem_testigo), 0, 1);
    sem_init(&(me->sem_tengo_que_enviar_testigo), 0, 1);
    sem_init(&(me->sem_tengo_que_pedir_testigo), 0, 1);
    sem_init(&(me->sem_permiso_para_SCEM_anulpagos), 0, 1);
    sem_init(&(me->sem_permiso_para_SCEM_cons), 0, 1);
    sem_init(&(me->sem_permiso_para_SCEM_resadmin), 0, 1);
    sem_init(&(me->sem_prioridad_maxima), 0, 1);
    sem_init(&(me->sem_contador_procesos_max_SC), 0, 1);

    sem_init(&(me->sem_SCEM), 0, 1);
    // INICIO RX!!!!!!!!!!!!!!!!!
    struct msgbuf_mensaje mensaje_rx;
    int i;
    bool quedan_solicitudes_sin_atender = false;
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
            printf("RECEPTOR: He recibido una petición del nodo: %d\n", mensaje_rx.id);
#endif
            sem_wait(&(me->sem_peticiones));
            me->peticiones[mensaje_rx.id - 1][0] = max(me->peticiones[mensaje_rx.id - 1][0], mensaje_rx.peticion);
            me->peticiones[mensaje_rx.id - 1][1] = mensaje_rx.prioridad;
            sem_post(&(me->sem_peticiones));
            printf("\n");

            // SI TENGO EL TESTIGO Y NO LO ESTOY USANDO. LE ENVÍO EL TESTIGO AL SIGUIENTE NODO
            sem_wait(&(me->sem_testigo));
            if (me->testigo)
            {
                sem_post(&(me->sem_testigo));
                sem_wait(&(me->sem_prioridad_maxima));
                if (me->prioridad_maxima <= mensaje_rx.prioridad && me->prioridad_maxima != 0)
                {
#ifdef __PRINT_RX
                    printf("RECEPTOR: TENEMOS LA MISMO O YO TENGO INFERIOR\n");
#endif
                    // Si es igual o mayor la prioridad del otro nodo, cortamos el grifo y el ultimo envia el testigo.
                    sem_post(&(me->sem_prioridad_maxima));
                    sem_wait(&(me->sem_tengo_que_enviar_testigo));
                    me->tengo_que_enviar_testigo = true;
                    sem_post(&(me->sem_tengo_que_enviar_testigo));
                    sem_wait(&(me->sem_tengo_que_pedir_testigo));
                    me->tengo_que_pedir_testigo = true;
                    sem_post(&(me->sem_tengo_que_pedir_testigo));
                    sem_wait(&(me->sem_prioridad_maxima));
                    switch (me->prioridad_maxima)
                    {
                    case PAGOS_ANUL:
                        sem_post(&(me->sem_prioridad_maxima));
                        sem_wait(&(me->sem_permiso_para_SCEM_anulpagos));
                        me->permiso_para_SCEM_anulpagos = false;
                        sem_post(&(me->sem_permiso_para_SCEM_anulpagos));
                        break;
                    case ADMIN_RESER:
                        sem_post(&(me->sem_prioridad_maxima));
                        sem_wait(&(me->sem_permiso_para_SCEM_resadmin));
                        me->permiso_para_SCEM_resadmin = false;
                        sem_post(&(me->sem_permiso_para_SCEM_resadmin));
                        break;
                    case CONSULTAS:
                        sem_post(&(me->sem_prioridad_maxima));
                        sem_wait(&(me->sem_permiso_para_SCEM_cons));
                        me->permiso_para_SCEM_cons = false;
                        sem_post(&(me->sem_permiso_para_SCEM_cons));
                        break;
                    }
                }
                else
                {
                    if (me->prioridad_maxima == 0)
                    {
                        sem_post(&(me->sem_prioridad_maxima));
                        send_testigo(mi_id);
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