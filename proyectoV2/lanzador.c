#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define nodos 4
#define pagos 4
#define admin 4
#define anulaciones 4

int main(int argc, char *argv[]){

/*int e=3;
char t=e+'0';
char e2[5];
e2[0]=t;
char* const argv2[]={"dumytest",e2,NULL};
execv("./dumytest",argv2); */

char orden[100]="./receptor ";
//char aux2[100]="./receptor ";
int i=0;
//char* const argv2[]={"receptor",e2,NULL};
char numero_receptor[5];

for(i=0;i<nodos;i++){               //inicio receptores
int pid=fork();
if(pid==0){
    char aux=i+'0';
    numero_receptor[0]=aux;
    char* const argv2[]={"receptor",numero_receptor,NULL};
    execv("./receptor",argv2);
}
} 

                                    //pagos
i=0;
int numero_nodo=0;
char char_nodo[5];

for(numero_nodo=0;numero_nodo<nodos;numero_nodo++){
i=0;
for(i=0;i<pagos;i++){
    char aux=numero_nodo+'0';
    char_nodo[0]=aux;
    char* const argv2[]={"pagos",char_nodo,NULL};
    execv("./pagos",argv2);
}
}

                                   //admin
i=0;
numero_nodo=0;

for(numero_nodo=0;numero_nodo<nodos;numero_nodo++){
i=0;
for(i=0;i<pagos;i++){
    char aux=numero_nodo+'0';
    char_nodo[0]=aux;
    char* const argv2[]={"admin",char_nodo,NULL};
    execv("./admin",argv2);
}
}

                                   //reservas
i=0;
numero_nodo=0;

for(numero_nodo=0;numero_nodo<nodos;numero_nodo++){
i=0;
for(i=0;i<pagos;i++){
    char aux=numero_nodo+'0';
    char_nodo[0]=aux;
    char* const argv2[]={"reservas",char_nodo,NULL};
    execv("./reservas",argv2);
}
}


                                   //reservas
i=0;
numero_nodo=0;

for(numero_nodo=0;numero_nodo<nodos;numero_nodo++){
i=0;
for(i=0;i<pagos;i++){
    char aux=numero_nodo+'0';
    char_nodo[0]=aux;
    char* const argv2[]={"reservas",char_nodo,NULL};
    execv("./reservas",argv2);
}
}

return 0;
}
