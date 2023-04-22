#include "procesos.h"


//DECLARACIÓN FUNCIONES
void *proceso(void *n);
void send_testigo();
void *receptor(void *n);
int max(int n1, int n2);



int main(int argc, char *argv[]){

    if (argc != 2 || atoi(argv[1]) == 0){
        printf("La forma de ejecutar el programa es: %s id_nodo\n", argv[0]);
        printf("El id del nodo no puede ser 0, [1, N]\n");
        return -1;
    }

    int i = 0;
    mi_id = atoi(argv[1]);

    //INICIALIZACIÓN TESTIGO
    if(mi_id == 1){
        testigo = true;
        tengo_que_pedir_testigo = false;
        permiso_para_S_C_E_M = true;
    }

    //INICIALIZACIÓN SEMÁFOROS
    sem_init(&sem_testigo, 0, 1);
    sem_init(&sem_mi_id, 0, 1);
    sem_init(&sem_atendidas, 0, 1);
    sem_init(&sem_peticiones, 0, 1);

    sem_init(&sem_mi_peticion, 0, 1);
    sem_init(&sem_contador_procesos, 0, 1);
    sem_init(&sem_contador_procesos_SC, 0, 1);
    sem_init(&sem_contador_espera, 0, 1);
    sem_init(&sem_buzones_nodos, 0, 1);
    sem_init(&sem_tengo_que_pedir_testigo, 0, 1);
    sem_init(&sem_permiso_para_S_C_E_M, 0, 1);

    sem_init(&sem_espera_procesos, 0, 0);

    sem_init(&sem_S_C_E_M, 0, 1);

    //INICIALIZACIÓN BUZONES DE LOS NODOS.
    for (i = 0; i < N; i++){

        buzones_nodos[i] = msgget(i + 1, IPC_CREAT | 0777);
        printf("El id del buzón del nodo %i es: %i.\n", i + 1, buzones_nodos[i]);

        if(buzones_nodos[i] == -1){

            perror("No se creó el mensaje\n");
        }
    }

    //INICIALIZACIÓN HILO RECEPTOR
    pthread_t hilo;
    int id_hilo = pthread_create(&hilo, NULL, receptor, NULL);//Hilo destinado a la recepción de mensajes.
    if(id_hilo != 0){
        printf("No se ha podido crear el hilo.\n");
        return -1;
    }

    //INICIALIZACIÓN PROCESOS DEL NODO
    pthread_t hilo_procesos[P];
    int ids_hilos_procesos[P];

    int procesos[P];
    int *punt_procesos = &procesos[0];

    for(i = 0; i < P; i++){
        procesos[i] = i + 1;
        ids_hilos_procesos[i] = pthread_create(&hilo_procesos[i], NULL, proceso, (punt_procesos + i));
        if(ids_hilos_procesos[i] != 0){
            printf("No se ha podido crear el hilo.\n");
            return -1;
        }
    }

    while(1);

    return 0;
}

void *proceso(void *n){

    int mi_id_proceso = *((int *)n);
    int i = 0;
    sem_wait(&sem_mi_id);
    long yo = mi_id;
    sem_post(&sem_mi_id);

    int espera_aleatoria = 0;
    srand(time(NULL));

    while(1){

        //ESPERA ALEATORIA PARA ENTRAR EN LA S.C.E.M
        //printf("Proceso %d: Esperando hasta querer entrar en la S.C.E.M.\n", mi_id_proceso);
        espera_aleatoria = rand()% MAX_ESPERA;
        sleep(espera_aleatoria);
        sem_wait(&sem_contador_procesos);
        contador_procesos++;
        sem_post(&sem_contador_procesos);
        sem_wait(&sem_tengo_que_pedir_testigo);
        if(tengo_que_pedir_testigo){//tengo que pedir el testigo bien por ser el primero bien, porque vamos a tener que enviar el testigo antes de que me toque.
            tengo_que_pedir_testigo = false;
            sem_post(&sem_tengo_que_pedir_testigo);

            printf("Proceso %d: Voy a pedir el testigo.\n", mi_id_proceso);
            sem_wait(&sem_mi_peticion);
            mi_peticion++;
            solicitud.peticion = mi_peticion;
            sem_post(&sem_mi_peticion);
            solicitud.id = yo;
            solicitud.msg_type = 1;
            //ENVIO PETICIONES
            for(i = 0; i < N; i++){
                
                if(yo - 1 == i){
                    continue;

                }else{

                    sem_wait(&sem_buzones_nodos);
                    //sem_wait(&sem_msg_solicitud);

                    if(msgsnd(buzones_nodos[i], &solicitud, sizeof(solicitud), 0) == -1){

                        //sem_post(&sem_msg_solicitud);
                        sem_post(&sem_buzones_nodos);
                        printf("Proceso %d:\n\tERROR: Hubo un error enviando el mensaje al nodo: %i.\n",mi_id_proceso, i);
                    }else{

                        //sem_post(&sem_msg_solicitud);
                        sem_post(&sem_buzones_nodos);
                    }
                }
            }
            //Espero sentadito
            sem_wait(&sem_contador_espera);
            contador_espera++;
            sem_post(&sem_contador_espera);
            sem_wait(&sem_espera_procesos);
            sem_wait(&sem_contador_espera);
            contador_espera--;
            if(contador_espera > 0){
                sem_post(&sem_contador_espera);
                sem_post(&sem_espera_procesos);
            }else{
                sem_post(&sem_contador_espera);
            }

        }else{
            sem_post(&sem_tengo_que_pedir_testigo);

            sem_wait(&sem_permiso_para_S_C_E_M);
            if(!permiso_para_S_C_E_M){
                sem_post(&sem_permiso_para_S_C_E_M);
                //Espero sentadito
                sem_wait(&sem_contador_espera);
                contador_espera++;
                sem_post(&sem_contador_espera);
                sem_wait(&sem_espera_procesos);
                sem_wait(&sem_contador_espera);
                contador_espera--;
                if(contador_espera > 0){
                    sem_post(&sem_contador_espera);
                    sem_post(&sem_espera_procesos);
                }else{
                    sem_post(&sem_contador_espera);
                }
            }else{
                sem_post(&sem_permiso_para_S_C_E_M);
            }
        }

        sem_wait(&sem_contador_procesos_SC);
        contador_procesos_SC++;
        sem_post(&sem_contador_procesos_SC);
        //AQUÍ CONFLUYEN LOS PARGUELAS, CON LOS QUE YA TIENEN EL TESTIGO JUNTO CON EL HÉROE QUE LO CONSIGUIÓ 
        sem_wait(&sem_S_C_E_M);
          /////////////////////////////////////////
         //ENTRAMOS DENTRO DE LA SECCIÓN CRÍTICA//
        /////////////////////////////////////////
        printf("Proceso %d:\n\t\tESTOY DENTRO DE LA SECCIÓN CRÍTICA\n\n", mi_id_proceso);
        //estamos el tiempo que nos de la real gana, porque nos hemos ganado el testigo, yeah baby (aunque no un tiempo infinito)
        //espera_aleatoria = rand()% MAX_ESPERA;
        sleep(SLEEP);
        sem_wait(&sem_contador_procesos);
        contador_procesos--;
        sem_post(&sem_contador_procesos);

        sem_post(&sem_S_C_E_M);

        //Salimos de la SCEM
        sem_wait(&sem_contador_procesos_SC);
        contador_procesos_SC--;
        if(contador_procesos_SC == 0){
            sem_post(&sem_contador_procesos_SC);
            //ACTUALIZAMOS VECTOR ANTENDIDAS Y VARIABLE DENTRO
            sem_wait(&sem_atendidas);
            atendidas[yo - 1] = atendidas[yo - 1] + 1;
            sem_post(&sem_atendidas);

            //ENVIAMOS EL TESTIGO
            send_testigo();
        }else{
            sem_post(&sem_contador_procesos_SC);
        }
    }

}

void *receptor(void *n){

    struct msgbuf_mensaje mensaje_rx;
    int i;
    bool quedan_solicitudes_sin_atender = false;
    sem_wait(&sem_mi_id);
    int mi_id_receptor = mi_id - 1;
    sem_post(&sem_mi_id);
    sem_wait(&sem_buzones_nodos);
    int id_de_mi_buzon = buzones_nodos[mi_id_receptor];
    sem_post(&sem_buzones_nodos);

    while(true){

        //RECIBIMOS PETICIÓN
        if(msgrcv(id_de_mi_buzon, &mensaje_rx, sizeof(mensaje_rx), 0, 0) == -1){
            printf("Proceso Rx: ERROR: Hubo un error al recibir un mensaje en el RECEPTOR.\n");
        }
        
        //ACTUALIZO EL VALOR DE PETICIONES CON LA QUE ME ACABA DE LLEGAR
        if(mensaje_rx.msg_type == 1){//EL mensaje es una petición.
            printf("Proceso RX: He recibido una petición del nodo: %d\n", mensaje_rx.id);
            sem_wait(&sem_peticiones);
            peticiones[mensaje_rx.id - 1] = max(peticiones[mensaje_rx.id - 1], mensaje_rx.peticion);
            sem_post(&sem_peticiones);
            printf("\n");

            //SI TENGO EL TESTIGO Y NO ESTOY EN LA S.C. LE ENVÍO EL TESTIGO AL SIGUIENTE NODO
            sem_wait(&sem_testigo);
            sem_wait(&sem_contador_procesos_SC);
            sem_wait(&sem_peticiones);
            sem_wait(&sem_atendidas);
            if(testigo && !(contador_procesos_SC > 0) && (peticiones[mensaje_rx.id - 1] > atendidas[mensaje_rx.id -1])){
                sem_post(&sem_peticiones);
                sem_post(&sem_atendidas);
                sem_post(&sem_testigo);
                sem_post(&sem_contador_procesos_SC);
                //ENVIAMOS EL TESTIGO
                printf("Proceso RX: PREPARANDO ENVIO\n");
                send_testigo();
            }else{
                sem_post(&sem_peticiones);
                sem_post(&sem_atendidas);
                sem_post(&sem_testigo);
                sem_post(&sem_contador_procesos_SC);
            }
            sem_wait(&sem_testigo);
            if(testigo){
                sem_post(&sem_testigo);
                sem_wait(&sem_tengo_que_pedir_testigo);
                tengo_que_pedir_testigo = true;
                sem_post(&sem_tengo_que_pedir_testigo);
                sem_wait(&sem_permiso_para_S_C_E_M);
                permiso_para_S_C_E_M = false;
                sem_post(&sem_permiso_para_S_C_E_M);
            }else{
                sem_post(&sem_testigo);
            }

        }else{//El mensaje es el testigo
            printf("Proceso RX: He recibido el testigo del nodo: %d\n", mensaje_rx.id);
            sem_post(&sem_espera_procesos);
            for(i = 0; i < N; i++){
                sem_wait(&sem_atendidas);
                atendidas[i] = mensaje_rx.atendidas[i];
                sem_wait(&sem_peticiones);
                if(atendidas[i] < peticiones[i]){
                    quedan_solicitudes_sin_atender = true;
                }
                sem_post(&sem_peticiones);
                sem_post(&sem_atendidas);

            }
            sem_wait(&sem_tengo_que_pedir_testigo);         
            sem_wait(&sem_permiso_para_S_C_E_M);
            if(quedan_solicitudes_sin_atender){
                permiso_para_S_C_E_M = false;
                tengo_que_pedir_testigo = true;
            }else{
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

}

void send_testigo(){

    int i = 0;
    sem_wait(&sem_mi_id);
    int id_buscar = mi_id;
    int yo = mi_id;
    sem_post(&sem_mi_id);
    bool encontrado = false;
    struct msgbuf_mensaje msg_testigo;
    msg_testigo.msg_type = 2;
    msg_testigo.id = yo;
    if(id_buscar + i + 1 > N){
            id_buscar = 1;
    }else{
        
        id_buscar++;
    }

    //COMPROBACIÓN DE SI HAY ALGUIEN ESPERANDO
    for(i; i < N; i++){

        //ANILLO LÓGICO
        if(id_buscar > N){
            id_buscar = 1;
        }
        if(id_buscar != yo){

            //SI HAY MAS PETICIONES QUE ATENDIDAS, ESTÁ ESPERANDO EL TESTIGO
            sem_wait(&sem_peticiones);
            sem_wait(&sem_atendidas);
            if(peticiones[id_buscar - 1] > atendidas[id_buscar - 1]){
                sem_post(&sem_peticiones);
                sem_post(&sem_atendidas);
                encontrado = true;
                printf("PROCESO ENVIO: El id al que le vamos a enviar el testigo es: %d\n", id_buscar);
                break;
            }else{
                sem_post(&sem_peticiones);
                sem_post(&sem_atendidas);
            }
        }
        id_buscar++;
    }

    if(encontrado){

        //CREANDO EL MENSAJE PARA EL TESTIGO
        msg_testigo.id = id_buscar;
        i = 0;
        for(i; i < N; i++){
            sem_wait(&sem_atendidas);
            msg_testigo.atendidas[i] = atendidas[i];
            sem_post(&sem_atendidas);
        }

        sem_wait(&sem_testigo);
        testigo = false;
        sem_post(&sem_testigo);
        //ENVIANDO TESTIGO
        sem_wait(&sem_buzones_nodos);
        if(msgsnd(buzones_nodos[id_buscar - 1], &msg_testigo, sizeof(msg_testigo), 0)){
            printf("PROCESO ENVIO: \n\n\tERROR: Hubo un error al enviar el testigo.\n");
        }
        sem_post(&sem_buzones_nodos);

        printf("PROCESO ENVIO: \n\t\t TESTIGO ENVIADO\n");
    }

}

int max(int n1, int n2){
  if(n1 > n2) return n1;
  else return n2;
}