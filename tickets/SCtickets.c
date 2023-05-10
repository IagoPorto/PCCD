
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <pthread.h>
#include <math.h>
#include <time.h>
#define nodos 5

typedef struct
{
    int type;
    float valor;
    int id_orixe;
} ticket;

float valorTicket, minTicket;
int quero_sc=0;
int miID,  pendentes=0; 
int colaNodos [nodos-1];
sem_t xenericoSC;

float max(float n1, float n2){
    if(n1>n2 || n1==n2){
        return n1;
    }
    if(n2>n1){
        return n2;
    }
}

void* fioReceptor(){
    int erro;
    int permisos=0;
    ticket ticketIN, ticketOUT;


while (1) {


    erro=msgrcv(miID, &ticketIN, sizeof(ticketIN.valor)+ sizeof(ticketIN.id_orixe),0,0);
    if (erro==-1){
        printf("Erro o recibir o mensaxe\n");

    }
        //Supoño que estou esperando confirmacións

    else if(ticketIN.type==2){   //request

     permisos ++;
     printf("Permisos= %i\n",permisos);

     if(permisos==(nodos-1)){
        sem_post(&xenericoSC);
        printf("¡POST!\n");
        permisos=0;

     }

     printf("Recibin todos os permisos\n");
    }

    else if(quero_sc==1 && (permisos < nodos-1)){  // && permisos > 0)){
        printf("[DEBUG] Quero_sc= %i  // permisos = %i\n",quero_sc,permisos);
        colaNodos[pendentes] = ticketIN.id_orixe;
        // colaNodos[ticketIN.id_orixe]=pendentes;
        pendentes++;
        //rematar
    }


            //semaforo para quero_sc?
  else if(quero_sc==0 || valorTicket >ticketIN.valor || (valorTicket >ticketIN.valor && miID>ticketIN.id_orixe)){ //se meu ticket e mais grande que un que me chegou, deixo que entre

    //semaforo para minTicket?
    minTicket=max(minTicket,ticketIN.valor);

    //contesto
    ticketOUT.valor=0;
    ticketOUT.id_orixe=miID;
    ticketOUT.type=2;

    erro=msgsnd(ticketIN.id_orixe,&ticketOUT,sizeof(ticketOUT.valor)+sizeof(ticketOUT.id_orixe),0);

    if(erro==-1){
        printf("Problema mandando confirmación \n");
    }
    else{
        printf("Confirmancion mandada a nodo %i\n",ticketIN.id_orixe);
    }


  }
}
}

int main(int argc,char* argv[]){
    int id_nodos[nodos-1];
    ticket ticketIN, ticketOUT;
    pthread_t fio;

    int direccion_nodos[nodos-1];

    //key_t key = ftok("/home/pedro/Teleco/PCCD/grupoc",atoi(argv[1]));
<<<<<<< HEAD:tickets.c
    key_t key = ftok("/Users/laura/Desktop/teleco/cuarto/2ºcuatri/pccd/testigo",atoi(argv[1]));
=======
    key_t key = ftok("/home/iago/Downloads",atoi(argv[1]));
>>>>>>> origin/main:tickets/SCtickets.c

    miID=msgget(key, IPC_CREAT | 0777);
    printf("Mi ID é: %i\n",miID);

    if(miID==-1){
        printf("Erro o crear o buzon\n");
        return 0;
    }

    //inicializo semaforos
    int erro = sem_init(&xenericoSC,0,0);
    if(erro==-1){
        printf("Erro o crear o semaforo xenerico\n");
        return 0; 
    }

    //creo fio
    erro = pthread_create(&fio,NULL,fioReceptor,NULL);
    if(erro==-1){
        printf("Erro o crear o fio\n");
        return 0;
    }

    int i;
    for ( i = 0; i < nodos - 1 ; i++)
    {
        printf("ID do nodo %i ? \n",i);
        scanf("%i",&direccion_nodos[i]);
    }

    fflush(stdin);

    printf("Entro en bucle de SC\n");
    while (1) {
        sleep(2);
        printf("Principio do bucle\n");
        while (getchar()!='q');

        srand ((unsigned)time(NULL));
        valorTicket=minTicket+(float)((rand()% 1000)/1000.0f);

        //semaforo sc
        quero_sc = 1;

        ticketOUT.valor=valorTicket;
        ticketOUT.id_orixe=miID;
        ticketOUT.type=1;

        for(i=0; i<nodos-1;i++){
            
            erro=msgsnd(direccion_nodos[i],&ticketOUT,sizeof(ticketOUT.valor)+sizeof(ticketOUT.id_orixe),0);

            if(erro==-1){
                printf("Erro mandando o nodo %i con id %i", i,direccion_nodos[i]);
            }
            else
            {
                printf("Peticion o nodo %i con id %i\n", i,direccion_nodos[i]);
            }
            

        }

        printf("Todas as peticions feitas\n");

        sem_wait(&xenericoSC);  //o post esta no fio receptor
        printf("ENTRO EN SECCIÓN CRITICA\n");
        sleep(5);
        printf("SAIO EN SECCIÓN CRITICA\n");

        ticketOUT.id_orixe=miID;
        ticketOUT.type=2;
        printf("[DEBUG] Pendentes despois de SC = %i\n", pendentes);
        printf("[DEBUG] Cola despois de SC = %i\n", colaNodos[0]);

        quero_sc=0;

        for(i=0; i < pendentes;i++){

            erro=msgsnd(colaNodos[i],&ticketOUT,sizeof(ticketOUT.valor)+sizeof(ticketOUT.id_orixe),0);

            if(erro==-1){
                printf("Erro mandando o nodo %i con id %i", i,colaNodos[i]);
            

        }

 }
 
    
}
return 0;
}
