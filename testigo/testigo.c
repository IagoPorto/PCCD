#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <semaphore.h>

#define N 5

struct msgbuf_solicitud{
  int id;
  int peticion;
};
struct msgbuf_testigo{
  int id;
  int atendidas[N];
};

bool testigo = false;
bool dentro = false;
int mi_id;
int atendidas[N], peticiones[N];
int buzones_nodos[N];
int buzon_testigo;

sem_t sem_testigo, sem_dentro, sem_mi_id, sem_atendidas, 
      sem_peticiones, sem_buzones_nodos, sem_buzon_testigo;

//DECLARACIÓN FUNCIONES
void send_testigo();
void *receptor(void *n);
int max(int n1, int n2);

int main(int argc, char *argv[]){

    if (argc < 2){
        printf("La forma de ejecutar el programa es: %s num_nodos\n", argv[0]);
        return -1;
    }

    //VARIABLES
    mi_id = atoi(argv[1]);
    int i = 0;
    int mi_peticion = 0;
    struct msgbuf_testigo msg_testigo;
    struct msgbuf_solicitud msg_solicitud;
    msg_solicitud.id = mi_id;

    //INICIALIZACIÓN TESTIGO
    if(mi_id == 0){
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

    //INICIALIZACIÓN DE VECTOR ATENDIDAS, PETICIONES Y BUZONES DE LOS NODOS.
    for (i; i < N; i++){

        atendidas[i] = 0;
        peticiones[i] = 0;
        buzones_nodos[i] = msgget(i + 1, IPC_CREAT | 0777);
        printf("El id del buzón del nodo %i es: %i.\n", i, buzones_nodos[i]);

        if(buzones_nodos[i] == -1){

            perror("No se creó el mensaje\n");
        }
    }

    //INICIALIZACIÓN BUZÓN TESTIGO
    buzon_testigo = msgget(100, IPC_CREAT | 0777);
    printf("La cola del testigo tiene id: %i.\n", buzon_testigo);

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


    while(1){

        printf("Haciendo mis movidas hasta que me de la gana de entrar en la S.C\n");
        getchar();

        sem_wait(&sem_testigo);
        if(!testigo){

            sem_post(&sem_testigo);
            printf("Voy a pedir el testigo.\n");
            mi_peticion++;
            msg_solicitud.peticion = mi_peticion;

            //ENVIO PETICIONES
            i = 0;
            for(i; i < N; i++){
                
                printf("Enviando solicitudes...");
                sem_wait(&sem_mi_id);
                if(mi_id == i){

                    sem_post(&sem_mi_id);
                    printf("No me voy a enviar un mensaje a mi mismo\n");
                    continue;

                }else{

                    sem_post(&sem_mi_id);
                    sem_wait(&sem_buzones_nodos);
                    if(msgsnd(buzones_nodos[i], &msg_solicitud, sizeof(msg_solicitud), 0) == -1){

                        sem_post(&sem_buzones_nodos);
                        printf("\nERROR: Hubo un error enviando el mensaje al nodo: %i.\n", i);
                    }else{
                        
                        sem_post(&sem_buzones_nodos);
                        printf("Mensaje enviado con exito. \n");
                    }
                }
            }

            //RECIBIMOS EL TESTIGO
            sem_wait(&sem_buzon_testigo);
            if(msgrcv(buzon_testigo, &msg_testigo, sizeof(msg_testigo), mi_id, 0) == -1){

                sem_post(&sem_buzon_testigo);
                printf("\n\n\tERROR: Hubo un error al recibir el testigo\n");
                return -1;
            }
            sem_post(&sem_buzon_testigo);

            sem_wait(&sem_testigo);
            testigo = true;
            sem_post(&sem_testigo);
            printf("He recibido el testigo.\n");

            //ACTUALIZAMOS VECTOR ATENDIDAS
            i= 0;
            for(i; i < N; i++){
                sem_wait(&sem_atendidas);
                atendidas[i] = msg_testigo.atendidas[i];
                sem_post(&sem_atendidas);
            }

        }else{
            sem_post(&sem_testigo);
        }

        //ENTRAMOS DENTRO DE LA SECCIÓN CRÍTICA
        printf("Es mi TURNO.\n");
        sem_wait(&sem_dentro);
        dentro = true;
        sem_post(&sem_dentro);
        printf("\n\t\tESTOY DENTRO DE LA SECCIÓN CRÍTICA\n");
        //estamos el tiempo que nos de la real gana, porque nos hemos ganado el testigo, yeah baby (aunque no un tiempo infinito)
        getchar();

        //ACTUALIZAMOS VECTOR ANTENDIDAS Y VARIABLE DENTRO
        sem_wait(&sem_atendidas);
        atendidas[mi_id] = mi_peticion;
        sem_post(&sem_atendidas);

        sem_wait(&sem_dentro);
        dentro = false;
        sem_post(&sem_dentro);

        //ENVIAMOS EL TESTIGO
        send_testigo();

    }

    return 0;
}

void *receptor(void *n){

    struct msgbuf_solicitud msg_peticion;

    while(true){

        //RECIBIMOS PETICIÓN
        sem_wait(&sem_buzones_nodos);
        if(msgrcv(buzones_nodos[mi_id], &msg_peticion, sizeof(msg_peticion), 0, 0) == -1){
            sem_post(&sem_buzones_nodos);
            printf("ERROR: Hubo un error al recibir un mensaje con el hilo.\n");
        }
        sem_post(&sem_buzones_nodos);

        //ACTUALIZO EL VALOR DE PETICIONES CON LA QUE ME ACABA DE LLEGAR
        sem_wait(&sem_peticiones);
        peticiones[msg_peticion.id] = max(peticiones[msg_peticion.id], msg_peticion.peticion);

        //SI TENGO EL TESTIGO Y NO ESTOY EN LA S.C. LE ENVÍO EL TESTIGO AL SIGUIENTE NODO
        sem_wait(&sem_testigo);
        sem_wait(&sem_dentro);
        sem_wait(&sem_atendidas);
        if(testigo && !dentro && (peticiones[msg_peticion.id] > atendidas[msg_peticion.id])){
            sem_post(&sem_peticiones);
            sem_post(&sem_atendidas);
            sem_post(&sem_testigo);
            sem_post(&sem_dentro);
            //ENVIAMOS EL TESTIGO
            send_testigo();
        }
        sem_post(&sem_peticiones);
        sem_post(&sem_atendidas);
        sem_post(&sem_testigo);
        sem_post(&sem_dentro);

    }

}

void send_testigo(){

    int i = 0;
    int id_buscar = mi_id;
    bool encontrado = false;
    struct msgbuf_testigo msg_testigo;

    //COMPROBACIÓN DE SI HAY ALGUIEN ESPERANDO
    for(i; i < N; i++){

        //ANILLO LÓGICO
        sem_wait(&sem_mi_id);
        if(mi_id + i + 1 == N){
            sem_post(&sem_mi_id);
            id_buscar = 0;
        }else{
            sem_post(&sem_mi_id);
            id_buscar = mi_id + i + 1;
        }

        //SI HAY MAS PETICIONES QUE ATENDIDAS, ESTÁ ESPERANDO EL TESTIGO
        sem_wait(&sem_peticiones);
        if(peticiones[id_buscar] > atendidas[id_buscar]){
            sem_post(&sem_peticiones);
            encontrado = true;
            break;
        }
        sem_post(&sem_peticiones);
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
        sem_wait(&sem_buzon_testigo);
        if(msgsnd(buzon_testigo, &msg_testigo, sizeof(msg_testigo), 0)){
            sem_post(&sem_buzon_testigo);
            printf("\n\n\tERROR: Hubo un error al enviar el testigo.\n");
        }
        sem_post(&sem_buzon_testigo);

        printf("\n\t TESTIGO ENVIADO\n");
    }

}

int max(int n1, int n2){

  printf("vector peticiones: %i, mensaje: %i\n", n1, n2);
  if(n1 > n2) return n1;
  else return n2;
}