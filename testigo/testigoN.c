#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <semaphore.h>
#include <time.h>
#include <unistd.h>

#define N 5
#define P 5
#define MAX_ESPERA 20

struct msgbuf_solicitud{
  int id;
  int peticion;
};
struct msgbuf_testigo{
  long id;
  int atendidas[N];
};

//VARIABLES GLOBALES
bool testigo = false;
bool dentro = false;
int mi_id;
int atendidas[N] = {}, peticiones[N] = {};
int buzones_nodos[N];
int buzon_testigo;

//SEMÁFOROS GLOBALES
sem_t sem_testigo, sem_dentro, sem_mi_id, sem_atendidas, 
      sem_peticiones, sem_buzones_nodos, sem_buzon_testigo;

//VARIABLES PROCESOS
int mi_peticion = 0;
struct msgbuf_testigo msg_testigo;
struct msgbuf_solicitud msg_solicitud;
int contador_procesos = 0, contador_espera = 0;

//SEMÁFOROS PROCESOS
sem_t sem_mi_peticion, sem_msg_testigo, sem_msg_solicitud,
        sem_espera_procesos, sem_contador_procesos, sem_contador_espera;

sem_t sem_S_C_E_M;

//DECLARACIÓN FUNCIONES
void *proceso(void *n);
void send_testigo();
void *receptor(void *n);
int max(int n1, int n2);



int main(int argc, char *argv[]){

    if (argc < 2){
        printf("La forma de ejecutar el programa es: %s num_id_nodo\n", argv[0]);
        return -1;
    }

    int i = 0;
    mi_id = atoi(argv[1]);
    msg_solicitud.id = mi_id;

    //INICIALIZACIÓN TESTIGO
    if(mi_id == 1){
        testigo = true;
    }

    //INICIALIZACIÓN SEMÁFOROS
    sem_init(&sem_testigo, 0, 1);
    sem_init(&sem_dentro, 0, 1);
    sem_init(&sem_mi_id, 0, 1);
    sem_init(&sem_atendidas, 0, 1);
    sem_init(&sem_peticiones, 0, 1);
    sem_init(&sem_buzones_nodos, 0, 1);
    sem_init(&sem_buzon_testigo, 0, 1);

    sem_init(&sem_mi_peticion, 0, 1);
    sem_init(&sem_msg_testigo, 0, 1);//No es seguro
    //sem_init(&sem_msg_solicitud, 0, 1);//No es seguro
    sem_init(&sem_contador_procesos, 0, 1);
    sem_init(&sem_contador_espera, 0, 1);

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

    //INICIALIZACIÓN BUZÓN TESTIGO
    buzon_testigo = msgget(N + 1, IPC_CREAT | 0777);
    printf("La cola del TESTIGO tiene id: %i.\n", buzon_testigo);

    if(buzon_testigo == -1){
        perror("No se creó el mensaje\n");
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
        printf("Proceso %d: Haciendo mis movidas hasta que me de la gana de entrar en la S.C\n",mi_id_proceso);
        espera_aleatoria = rand()% MAX_ESPERA;
        sleep(espera_aleatoria);

        sem_wait(&sem_contador_procesos);
        contador_procesos++;
        sem_post(&sem_contador_procesos);

        sem_wait(&sem_testigo);
        if(!testigo){

            sem_post(&sem_testigo);

            sem_wait(&sem_contador_procesos);
            if(contador_procesos == 1){

                sem_post(&sem_contador_procesos);
                printf("Proceso %d: Voy a pedir el testigo.\n", mi_id_proceso);
                sem_wait(&sem_mi_peticion);
                mi_peticion++;
                //sem_wait(&sem_msg_solicitud);
                msg_solicitud.peticion = mi_peticion;
                msg_solicitud.id = yo;
                sem_post(&sem_mi_peticion);
                //sem_post(&sem_msg_solicitud);
                printf("\n\t\tPROCESO %d: EL MENSAJE TIENE ID = %d Y LA PETICIÓN = %d\n", mi_id_proceso, msg_solicitud.id, msg_solicitud.peticion);
                //ENVIO PETICIONES
                for(i = 0; i < N; i++){
                    
                    if(yo - 1 == i){

                        printf("Proceso %d: No me voy a enviar un mensaje a mi mismo\n", mi_id_proceso);
                        continue;

                    }else{

                        printf("Proceso %d: Enviando solicitud al nodo: %d\n", mi_id_proceso, i + 1);
                        sem_wait(&sem_buzones_nodos);
                        //sem_wait(&sem_msg_solicitud);

                        if(msgsnd(buzones_nodos[i], &msg_solicitud, sizeof(msg_solicitud), 0) == -1){

                            //sem_post(&sem_msg_solicitud);
                            sem_post(&sem_buzones_nodos);
                            printf("Proceso %d:\n\tERROR: Hubo un error enviando el mensaje al nodo: %i.\n",mi_id_proceso, i);
                        }else{

                            //sem_post(&sem_msg_solicitud);
                            sem_post(&sem_buzones_nodos);
                            printf("Proceso %d: Mensaje enviado con EXITO. \n", mi_id_proceso);
                        }
                    }
                }

                //RECIBIMOS EL TESTIGO
                sem_wait(&sem_buzon_testigo);
                if(msgrcv(buzon_testigo, &msg_testigo, sizeof(msg_testigo), yo, 0) == -1){

                    sem_post(&sem_buzon_testigo);
                    printf("Proceso %d: \n\n\tERROR: Hubo un error al recibir el testigo\n", mi_id_proceso);
                }
                sem_post(&sem_buzon_testigo);

                sem_wait(&sem_testigo);
                testigo = true;
                sem_post(&sem_testigo);
                printf("Proceso %d: He recibido el testigo.\n", mi_id_proceso);

                //SI HAY PARGUELAS ESPERANDO LE DOY PASO A UNO
                sem_wait(&sem_contador_espera);
                if(contador_espera > 0){
                    sem_post(&sem_contador_espera);
                    sem_post(&sem_espera_procesos);
                }else{
                    sem_post(&sem_contador_espera);
                }

                //ACTUALIZAMOS VECTOR ATENDIDAS
                for(i = 0; i < N; i++){
                    sem_wait(&sem_atendidas);
                    atendidas[i] = msg_testigo.atendidas[i];
                    sem_post(&sem_atendidas);
                }

            }else{//Si no soy el primero y no tenemos el testigo, espero como un parguela.

                printf("Proceso %d: Soy un parguelita, me toca esperar\n", mi_id_proceso);
                sem_post(&sem_contador_procesos);
                sem_wait(&sem_contador_espera);
                contador_espera++;
                sem_post(&sem_contador_espera);

                //AQUÍ ESPERAN COMO PARGUELAS
                sem_wait(&sem_espera_procesos);
                //AQUÍ DEJAN DE SER PARGUELAS
                sem_wait(&sem_contador_espera);
                contador_espera--;
                if(contador_espera > 0){
                    sem_post(&sem_contador_espera);
                    sem_post(&sem_espera_procesos);
                }else{
                    sem_post(&sem_contador_espera);
                }
            }

        }else{
            sem_post(&sem_testigo);
            
        }
        printf("Proceso %d: Estoy esperando para entrar en la S.C.E.M\n", mi_id_proceso);
        //AQUÍ CONFLUYEN LOS PARGUELAS, CON LOS QUE YA TIENEN EL TESTIGO JUNTO CON EL HÉROE QUE LO CONSIGUIÓ 
        sem_wait(&sem_S_C_E_M);

        //ENTRAMOS DENTRO DE LA SECCIÓN CRÍTICA
        printf("Proceso %d: \nEs mi TURNO.\n", mi_id_proceso);
        sem_wait(&sem_dentro);
        dentro = true;
        sem_post(&sem_dentro);
        printf("Proceso %d:\n\t\tESTOY DENTRO DE LA SECCIÓN CRÍTICA\n", mi_id_proceso);
        //estamos el tiempo que nos de la real gana, porque nos hemos ganado el testigo, yeah baby (aunque no un tiempo infinito)
        //espera_aleatoria = rand()% MAX_ESPERA;
        sleep(1);

        sem_wait(&sem_contador_procesos);
        contador_procesos--;

        if(contador_procesos == 0){

            //ACTUALIZAMOS VECTOR ANTENDIDAS Y VARIABLE DENTRO
            sem_wait(&sem_atendidas);
            atendidas[yo - 1] = mi_peticion;
            sem_post(&sem_atendidas);

            sem_wait(&sem_dentro);
            dentro = false;
            sem_post(&sem_dentro);

            //ENVIAMOS EL TESTIGO
            send_testigo();
            sem_post(&sem_contador_procesos);

        }else{
            sem_post(&sem_contador_procesos);
            
        }

        sem_post(&sem_S_C_E_M);
    }

}

void *receptor(void *n){

    struct msgbuf_solicitud msg_peticion;
    sem_wait(&sem_mi_id);
    int mi_id_receptor = mi_id - 1;
    sem_post(&sem_mi_id);
    sem_wait(&sem_buzones_nodos);
    int id_de_mi_buzon = buzones_nodos[mi_id_receptor];
    sem_post(&sem_buzones_nodos);

    while(true){

        //RECIBIMOS PETICIÓN
        if(msgrcv(id_de_mi_buzon, &msg_peticion, sizeof(msg_peticion), 0, 0) == -1){
            printf("Proceso Rx: ERROR: Hubo un error al recibir un mensaje en el RECEPTOR.\n");
        }
        printf("Proceso Rx: He recibido un mensaje del nodo: %d\n", msg_peticion.id);
        //ACTUALIZO EL VALOR DE PETICIONES CON LA QUE ME ACABA DE LLEGAR
        sem_wait(&sem_peticiones);
        peticiones[msg_peticion.id - 1] = max(peticiones[msg_peticion.id - 1], msg_peticion.peticion);
        printf("\n");
        sem_post(&sem_peticiones);

        //SI TENGO EL TESTIGO Y NO ESTOY EN LA S.C. LE ENVÍO EL TESTIGO AL SIGUIENTE NODO
        sem_wait(&sem_testigo);
        sem_wait(&sem_dentro);
        sem_wait(&sem_peticiones);
        sem_wait(&sem_atendidas);
        if(testigo && !dentro && (peticiones[msg_peticion.id - 1] > atendidas[msg_peticion.id -1])){
            sem_post(&sem_peticiones);
            sem_post(&sem_atendidas);
            sem_post(&sem_testigo);
            sem_post(&sem_dentro);
            //ENVIAMOS EL TESTIGO
            printf("Proceso Rx: PREPARANDO ENVIO\n");
            send_testigo();
        }else{
            sem_post(&sem_peticiones);
            sem_post(&sem_atendidas);
            sem_post(&sem_testigo);
            sem_post(&sem_dentro);
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
    struct msgbuf_testigo msg_testigo;
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
                printf("Proceso envio: El id al que le vamos a enviar el testigo es: %d\n", id_buscar);
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
        printf("Proceso envio: PAQUETE CALIENTE: Lo espera el nodo %ld\n", msg_testigo.id);
        //ENVIANDO TESTIGO
        sem_wait(&sem_buzon_testigo);
        if(msgsnd(buzon_testigo, &msg_testigo, sizeof(msg_testigo), 0)){
            printf("Proceso envio: \n\n\tERROR: Hubo un error al enviar el testigo.\n");
        }
        sem_post(&sem_buzon_testigo);

        printf("Proceso envio: \n\t\t TESTIGO ENVIADO\n");
    }

}

int max(int n1, int n2){
  if(n1 > n2) return n1;
  else return n2;
}