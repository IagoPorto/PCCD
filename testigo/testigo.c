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
  long id;
  int atendidas[N];
};

bool testigo = false;
bool dentro = false;
int mi_id;
int atendidas[N] = {}, peticiones[N] = {};
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
    long yo = mi_id;
    int i = 0;
    int mi_peticion = 0;
    struct msgbuf_testigo msg_testigo;
    struct msgbuf_solicitud msg_solicitud;
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
            for(i = 0; i < N; i++){
                
                if(yo - 1 == i){

                    printf("No me voy a enviar un mensaje a mi mismo\n");
                    continue;

                }else{

                    printf("Enviando solicitud al nodo: %d\n", i + 1);
                    sem_wait(&sem_buzones_nodos);
                    if(msgsnd(buzones_nodos[i], &msg_solicitud, sizeof(msg_solicitud), 0) == -1){

                        sem_post(&sem_buzones_nodos);
                        printf("\n\tERROR: Hubo un error enviando el mensaje al nodo: %i.\n", i);
                    }else{
                        
                        sem_post(&sem_buzones_nodos);
                        printf("Mensaje enviado con EXITO. \n");
                    }
                }
            }

            //RECIBIMOS EL TESTIGO
            sem_wait(&sem_buzon_testigo);
            if(msgrcv(buzon_testigo, &msg_testigo, sizeof(msg_testigo), yo, 0) == -1){

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
            for(i = 0; i < N; i++){
                sem_wait(&sem_atendidas);
                atendidas[i] = msg_testigo.atendidas[i];
                sem_post(&sem_atendidas);
            }

        }else{
            sem_post(&sem_testigo);
        }

        //ENTRAMOS DENTRO DE LA SECCIÓN CRÍTICA
        printf("\nEs mi TURNO.\n");
        sem_wait(&sem_dentro);
        dentro = true;
        sem_post(&sem_dentro);
        printf("\n\t\tESTOY DENTRO DE LA SECCIÓN CRÍTICA\n");
        //estamos el tiempo que nos de la real gana, porque nos hemos ganado el testigo, yeah baby (aunque no un tiempo infinito)
        getchar();

        //ACTUALIZAMOS VECTOR ANTENDIDAS Y VARIABLE DENTRO
        sem_wait(&sem_atendidas);
        sem_wait(&sem_mi_id);
        atendidas[mi_id - 1] = mi_peticion;
        sem_post(&sem_atendidas);
        sem_post(&sem_mi_id);

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
    sem_wait(&sem_mi_id);
    int mi_id_receptor = mi_id - 1;
    sem_post(&sem_mi_id);
    sem_wait(&sem_buzones_nodos);
    int id_de_mi_buzon = buzones_nodos[mi_id_receptor];
    sem_post(&sem_buzones_nodos);

    while(true){

        //RECIBIMOS PETICIÓN
        if(msgrcv(id_de_mi_buzon, &msg_peticion, sizeof(msg_peticion), 0, 0) == -1){
            printf("ERROR: Hubo un error al recibir un mensaje en el RECEPTOR.\n");
        }
        printf("He recibido un mensaje del nodo: %d\n", msg_peticion.id);
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
            printf("PREPARANDO ENVIO\n");
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
                printf("El id al que le vamos a enviar el testigo es: %d\n", id_buscar);
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
        printf("PAQUETE CALIENTE: Lo espera el nodo %ld\n", msg_testigo.id);
        //ENVIANDO TESTIGO
        sem_wait(&sem_buzon_testigo);
        if(msgsnd(buzon_testigo, &msg_testigo, sizeof(msg_testigo), 0)){
            printf("\n\n\tERROR: Hubo un error al enviar el testigo.\n");
        }
        sem_post(&sem_buzon_testigo);

        printf("\n\t\t TESTIGO ENVIADO\n");
    }

}

int max(int n1, int n2){
  if(n1 > n2) return n1;
  else return n2;
}