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
    sem_init(&(me->sem_mi_peticion), 0, 1);
    sem_init(&(me->sem_testigo), 0, 1);
    sem_init(&(me->sem_tengo_que_enviar_testigo), 0, 1);
    sem_init(&(me->sem_tengo_que_pedir_testigo), 0, 1);
    sem_init(&(me->sem_permiso_para_SCEM_anulpagos), 0, 1);
    sem_init(&(me->sem_permiso_para_SCEM_cons), 0, 1);
    sem_init(&(me->sem_permiso_para_SCEM_resadmin), 0, 1);

    // INICIO RX!!!!!!!!!!!!!!!!!
    struct msgbuf_mensaje mensaje_rx;
    int i;
    bool quedan_solicitudes_sin_atender = false;
    sem_wait(&sem_mi_id);
    int mi_id_receptor = mi_id - 1;
    sem_post(&sem_mi_id);
    sem_wait(&sem_buzones_nodos);
    int id_de_mi_buzon = buzones_nodos[mi_id_receptor];
    sem_post(&sem_buzones_nodos);

    while (true)
    {

        // RECIBIMOS PETICIÓN
        if (msgrcv(id_de_mi_buzon, &mensaje_rx, sizeof(mensaje_rx), 0, 0) == -1)
        {
            printf("Proceso Rx: ERROR: Hubo un error al recibir un mensaje en el RECEPTOR.\n");
        }

        // ACTUALIZO EL VALOR DE PETICIONES CON LA QUE ME ACABA DE LLEGAR
        if (mensaje_rx.msg_type == 1)
        { // EL mensaje es una petición.
            printf("Proceso RX: He recibido una petición del nodo: %d\n", mensaje_rx.id);
            sem_wait(&sem_peticiones);
            peticiones[mensaje_rx.id - 1] = max(peticiones[mensaje_rx.id - 1], mensaje_rx.peticion);
            sem_post(&sem_peticiones);
            printf("\n");

            // SI TENGO EL TESTIGO Y NO ESTOY EN LA S.C. LE ENVÍO EL TESTIGO AL SIGUIENTE NODO
            sem_wait(&sem_testigo);
            sem_wait(&sem_contador_procesos_SC);
            sem_wait(&sem_peticiones);
            sem_wait(&sem_atendidas);
            if (testigo && !(contador_procesos_SC > 0) && (peticiones[mensaje_rx.id - 1] > atendidas[mensaje_rx.id - 1]))
            {
                sem_post(&sem_peticiones);
                sem_post(&sem_atendidas);
                sem_post(&sem_testigo);
                sem_post(&sem_contador_procesos_SC);
                // ENVIAMOS EL TESTIGO
                printf("Proceso RX: PREPARANDO ENVIO\n");
                send_testigo();
            }
            else
            {
                sem_post(&sem_peticiones);
                sem_post(&sem_atendidas);
                sem_post(&sem_testigo);
                sem_post(&sem_contador_procesos_SC);
            }
            sem_wait(&sem_testigo);
            if (testigo)
            {
                sem_post(&sem_testigo);
                sem_wait(&sem_tengo_que_pedir_testigo);
                tengo_que_pedir_testigo = true;
                sem_post(&sem_tengo_que_pedir_testigo);
                sem_wait(&sem_permiso_para_S_C_E_M);
                permiso_para_S_C_E_M = false;
                sem_post(&sem_permiso_para_S_C_E_M);
            }
            else
            {
                sem_post(&sem_testigo);
            }
        }
        else
        { // El mensaje es el testigo
            printf("Proceso RX: He recibido el testigo del nodo: %d\n", mensaje_rx.id);
            sem_post(&sem_espera_procesos);
            for (i = 0; i < N; i++)
            {
                sem_wait(&sem_atendidas);
                atendidas[i] = mensaje_rx.atendidas[i];
                sem_wait(&sem_peticiones);
                if (atendidas[i] < peticiones[i])
                {
                    quedan_solicitudes_sin_atender = true;
                }
                sem_post(&sem_peticiones);
                sem_post(&sem_atendidas);
            }
            sem_wait(&sem_tengo_que_pedir_testigo);
            sem_wait(&sem_permiso_para_S_C_E_M);
            if (quedan_solicitudes_sin_atender)
            {
                permiso_para_S_C_E_M = false;
                tengo_que_pedir_testigo = true;
            }
            else
            {
                permiso_para_S_C_E_M = true;
                tengo_que_pedir_testigo = false;
            }
            sem_post(&sem_tengo_que_pedir_testigo);
            sem_post(&sem_permiso_para_S_C_E_M);
            sem_wait(&sem_testigo);
            testigo = true;
            sem_post(&sem_testigo);
        }
    }

    return 0;
}