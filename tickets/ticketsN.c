#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <semaphore.h>
#include <time.h>
#include <unistd.h>

#define N 5 // --> nodos
#define P 5 // --> procesos
#define MAX_ESPERA 20

int ticket = 0;
int max_ticket = 0;
int mi_id;

int id_nodos_pend[N];
int buzones_nodos[N];

bool quiero = false;

sem_t sem_ticket, sem_max_ticket, sem_mi_id,
        sem_id_nodos_pend, sem_buzones_nodos, 
        sem_quiero; 

int contador_procesos = 0, contador_procesos_espera = 0;
bool libertad = false;

sem_t sem_contador_procesos, sem_contador_procesos_espera, sem_libertad;
sem_t sem_espera_procesos;
sem_t sem_S_C_E_M;

struct msgbuf_message{
  long mtype;  //1 --> solicitud; 2 --> ACK
  int id;
  int ticket;
};

void *receptor(void *n);
void *proceso(void *n);
int max(int n1, int n2);

int main(int argc, char *argv[]){

    if(argc != 2 || atoi(argv[1]) == 0){
        printf("La forma de ejecución es: %s \"id_nodo\"\n", argv[0]);
        printf("El id del nodo no puede ser 0, [1, N]\n");
        return -1;
    }

    //INICIALIZACIÓN VARIABLES
    mi_id = atoi(argv[1]);
    int yo = mi_id;
    int i;
    int id_mi_buzon;


    //INICIALIZACIÓN BUZONES DE LOS NODOS
    for (i = 0; i < N; i++){

        buzones_nodos[i] = msgget(i + 1, IPC_CREAT | 0777);
        printf("El id del buzón del nodo %i es: %i.\n", i + 1, buzones_nodos[i]);

        if(buzones_nodos[i] == -1){

            perror("No se creó el mensaje\n");
        }
    }

    id_mi_buzon = buzones_nodos[yo - 1];

    //INICIALIZACIÓN SEMÁFOROS
    sem_init(&sem_ticket, 0, 1);
    sem_init(&sem_max_ticket, 0, 1);
    sem_init(&sem_mi_id, 0, 1);
    sem_init(&sem_buzones_nodos, 0, 1);
    sem_init(&sem_id_nodos_pend, 0, 1);
    sem_init(&sem_quiero, 0, 1);

    sem_init(&sem_contador_procesos, 0, 1);
    sem_init(&sem_contador_procesos_espera, 0, 1);
    sem_init(&sem_libertad, 0, 1);

    sem_init(&sem_espera_procesos, 0, 0);

    sem_init(&sem_S_C_E_M, 0, 1);

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
    sem_wait(&sem_mi_id);
    int yo = mi_id;
    sem_post(&sem_mi_id);
    int i;
    sem_wait(&sem_buzones_nodos);
    int id_mi_buzon = buzones_nodos[yo -1];
    sem_post(&sem_buzones_nodos);

    struct msgbuf_message msg_solicitud;
    struct msgbuf_message msg_recivido, msg_enviar;

    int espera_aleatoria = 0;
    srand(time(NULL));

    while(1){

        //ESPERA ALEATORIA PARA ENTRAR EN LA S.C.E.M
        printf("Proceso %d: Haciendo mis movidas hasta que me de la gana de entrar en la S.C\n",mi_id_proceso);
        espera_aleatoria = rand()% MAX_ESPERA;
        sleep(15);

        sem_wait(&sem_contador_procesos);
        contador_procesos++;
        sem_post(&sem_contador_procesos);
        
        sem_wait(&sem_libertad);
        if(!libertad){
            sem_post(&sem_libertad);
            sem_wait(&sem_contador_procesos);
            if(contador_procesos == 1){
                sem_post(&sem_contador_procesos);
                sem_wait(&sem_quiero);
                quiero = true;
                sem_post(&sem_quiero);
                sem_wait(&sem_ticket);
                sem_wait(&sem_max_ticket);
                ticket = max_ticket + 1;
                sem_post(&sem_max_ticket);
                msg_solicitud.ticket = ticket;
                sem_post(&sem_ticket);
                msg_solicitud.id = yo;
                msg_solicitud.mtype = 1;

                //ENVIO PETICIONES
                for(i = 0; i < N; i++){
                    
                    if(yo - 1 == i){

                        printf("Proceso %d: No me voy a enviar un mensaje a mi mismo\n", mi_id_proceso);
                        continue;

                    }else{

                        sem_wait(&sem_buzones_nodos);
                        if(msgsnd(buzones_nodos[i], &msg_solicitud, sizeof(msg_solicitud), 0) == -1){

                            sem_post(&sem_buzones_nodos);
                            printf("\n\tProceso %d: ERROR: Hubo un error enviando el mensaje al nodo: %i.\n",mi_id_proceso, i);
                        }else{
                            
                            sem_post(&sem_buzones_nodos);
                        }
                    }
                }
                printf("Proceso %d: Hemos enviado todos los tickets\n", mi_id_proceso);

                //ENVIO PETICIONES
                for(i = 0; i < N; i++){
                    
                    if(yo - 1 == i){

                        printf("Proceso %d: No me voy a recibir un mensaje de mi mismo\n", mi_id_proceso);
                        continue;

                    }else{

                        if(msgrcv(id_mi_buzon, &msg_recivido, sizeof(msg_recivido), 2, 0) == -1){

                            printf("\n\tProceso %d: ERROR: Hubo un error enviando el mensaje al nodo: %i.\n",mi_id_proceso, i);
                        }else{
                            
                            printf("Proceso %d: Mensaje del nodo %i recibido con EXITO. \n",mi_id_proceso, msg_recivido.id);
                        }
                    }
                }
                printf("Proceso %d: Hemos recibido todas las respuestas\n", mi_id_proceso);
                sem_wait(&sem_libertad);
                libertad = true;
                sem_post(&sem_libertad);

                sem_wait(&sem_contador_procesos_espera);
                if(contador_procesos_espera > 0){
                    sem_post(&sem_espera_procesos);
                }
                sem_post(&sem_contador_procesos_espera);

            }else{//parguelitas
                printf("Proceso %d: Soy un parguelita\n", mi_id_proceso);
                sem_post(&sem_contador_procesos);
                sem_wait(&sem_contador_procesos_espera);
                contador_procesos_espera++;
                sem_post(&sem_contador_procesos_espera);
                sem_wait(&sem_espera_procesos);
                sem_wait(&sem_contador_procesos_espera);
                contador_procesos_espera--;
                if(contador_procesos_espera > 0){
                    sem_post(&sem_espera_procesos);
                }
                sem_post(&sem_contador_procesos_espera);

                printf("Proceso %d: Ya no soy un parguelita\n", mi_id_proceso);
            }

        }else{
            sem_post(&sem_libertad);
        }

        sem_wait(&sem_S_C_E_M);

        printf("\n\t\tProceso %d: ESTAMOS DENTRO DE LA SECCIÓN CRÍTICA\n\n", mi_id_proceso);

        sleep(2);

        printf("\n\t\tProceso %d: VOY A SALIR DE LA SECCIÓN CRÍTICA\n\n", mi_id_proceso);

        sem_wait(&sem_contador_procesos);
        contador_procesos--;
        if(contador_procesos == 0){
            
            sem_wait(&sem_quiero);
            quiero = false;
            sem_post(&sem_quiero);
            sem_wait(&sem_libertad);
            libertad = false;
            sem_post(&sem_libertad);

            for(i = 0; i < N; i++){

                sem_wait(&sem_id_nodos_pend);
                if(id_nodos_pend[i] > 0){

                    sem_post(&sem_id_nodos_pend);
                    printf("Proceso %d: Le tengo que enviar un ACK al nodo %i\n",mi_id_proceso, i + 1);
                    msg_enviar.mtype = 2;
                    msg_enviar.id = yo;
                    sem_wait(&sem_buzones_nodos);
                    if(msgsnd(buzones_nodos[i], &msg_enviar, sizeof(msg_enviar), 0) == -1){

                        sem_post(&sem_buzones_nodos);
                        printf("\n\tProceso %d: ERROR 2: Hubo un error enviando el mensaje al nodo: %i.\n",mi_id_proceso, i + 1);
                    }else{
                        
                        sem_post(&sem_buzones_nodos);
                        printf("Proceso %d: Mensaje enviado con EXITO. \n", mi_id_proceso);
                    }
                    sem_wait(&sem_id_nodos_pend);
                    id_nodos_pend[i] = 0;
                    sem_post(&sem_id_nodos_pend);

                }else{

                    sem_post(&sem_id_nodos_pend);
                }
            }
           
        }
        sem_post(&sem_contador_procesos);
        sem_post(&sem_S_C_E_M);

    }

}

void *receptor(void *n){

    struct msgbuf_message msg_peticion;
    struct msgbuf_message msg_respuesta;
    msg_peticion.mtype = 1;
    msg_respuesta.mtype = 2;
    sem_wait(&sem_mi_id);
    int mi_id_receptor = mi_id - 1;
    sem_post(&sem_mi_id);
    msg_respuesta.id = mi_id_receptor + 1;
    sem_wait(&sem_buzones_nodos);
    int id_de_mi_buzon = buzones_nodos[mi_id_receptor];
    sem_post(&sem_buzones_nodos);

    while(true){

        //RECIBIMOS PETICIÓN
        if(msgrcv(id_de_mi_buzon, &msg_peticion, sizeof(msg_peticion), 1, 0) == -1){
            printf("Proceso RX: ERROR: Hubo un error al recibir un mensaje en el RECEPTOR.\n");
        }
        printf("Proceso RX: He recibido un mensaje del nodo: %d\n", msg_peticion.id);
        
        sem_wait(&sem_max_ticket);
        max_ticket = max(max_ticket, msg_peticion.ticket);
        sem_post(&sem_max_ticket);

        sem_wait(&sem_quiero);
        sem_wait(&sem_ticket);
        if(!quiero || msg_peticion.ticket < ticket || (msg_peticion.ticket == ticket  && msg_peticion.id < mi_id_receptor + 1)){
            sem_post(&sem_quiero);
            sem_post(&sem_ticket);
            msg_respuesta.id = mi_id_receptor + 1;
            msg_respuesta.mtype = 2;
            sem_wait(&sem_buzones_nodos);
            if(msgsnd(buzones_nodos[msg_peticion.id - 1], &msg_respuesta, sizeof(msg_respuesta), 0) == -1){

                sem_post(&sem_buzones_nodos);
                printf("\n\tProceso RX: ERROR 3: Hubo un error enviando el mensaje al nodo: %i.\n", msg_peticion.id);
            }else{
                
                sem_post(&sem_buzones_nodos);
                printf("Proceso RX: Mensaje enviado con EXITO. \n");
            }
        }else{
            sem_post(&sem_quiero);
            sem_post(&sem_ticket);
            sem_wait(&sem_id_nodos_pend);
            id_nodos_pend[msg_peticion.id - 1] = id_nodos_pend[msg_peticion.id - 1] + 1;
            printf("Proceso RX: El id del nodo pendiente es %i\n", msg_peticion.id);
            sem_post(&sem_id_nodos_pend);
        }

    }

}

int max(int n1, int n2){
  if(n1 > n2) return n1;
  else return n2;
}