#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <semaphore.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/time.h>


int main(int argc,char *argv[]){

     
    char entrada [50], numeros [3];
    int i, k, cont, nNodosAux, numPagos = 0,numAnulaciones = 0,numAdmin = 0,numReservas = 0,numConsultas = 0;

    int n = 0;
    int l, p, m, y;
   
    FILE * ficheroIn = fopen (argv [1], "r");


    if (ficheroIn == NULL) {

        printf ("Error:Fichero de entrada no encontrado. \n\n");
        return 0;
    }

    fgets (entrada, 50, ficheroIn);


     do {

        char variable [15] = {'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0'};

        for (i = 0, cont = 0; entrada [i] != '='; i++) {

            variable [i] = entrada [i];
            cont ++;
        }
                   


        memcpy (numeros, &entrada [cont + 1], 3);
        i = atoi (numeros);

        if (strcmp (variable, "nNodos") == 0) {

            nNodosAux = i;

            printf ("nNodos = %i\n", nNodosAux);
         
        } else if (strcmp (variable, "pagos") == 0) {

            numPagos = i;
            printf ("numPagos = %i\n", numPagos);
        }else if (strcmp (variable, "anulaciones") == 0) {

            numAnulaciones = i;
            printf ("numAnulaciones = %i\n", numAnulaciones);
        }
        else if (strcmp (variable, "reservas") == 0) {

            numReservas = i;
            printf ("numReservas = %i\n", numReservas);
        }
        else if (strcmp (variable, "admin") == 0) {

            numAdmin = i;
            printf ("numAdmin = %i\n", numAdmin);
        }
        else if (strcmp (variable, "consultas") == 0) {

            numConsultas = i;
            printf ("numConsultas = %i\n", numConsultas);
        }


        fgets (entrada, 50, ficheroIn);

    } while (entrada [0] != '\n');
                         
 


    int procReceptor[nNodosAux];

    for (i = 1; i < nNodosAux+1; i++) {
       

       procReceptor [i] = fork ();

        if (procReceptor [i] == 0) {
          
            char iAux [2];
            sprintf (iAux, "%i", i);

            execl ("receptor", "receptor",iAux, (char *) NULL);
            return 0;

                               
        } 


    }


    int numProcHijos = (numPagos+numAnulaciones+numReservas+numAdmin+numConsultas)*nNodosAux;
    int procHijo[numProcHijos];

   
    for (i = 1 ;i < nNodosAux + 1 ; i++) {

        char iAux [5];
        sprintf (iAux, "%i", i);
          

        for(k = 0;k < numPagos;k++){
           
            procHijo [n] = fork ();
            if (procHijo [n] == 0) {

                execl ("pagos", "pagos",iAux, (char *) NULL);
                return 0;
            }
            n++;
        }
        
        for(l = 0;l < numAnulaciones;l++){
            procHijo[n] = fork ();
            if (procHijo [n] == 0) {
            execl ("anulaciones", "anulaciones",iAux, (char *) NULL);
            return 0;
            }
              n++;

        }

        for(m = 0;m < numReservas;m++){
            procHijo[n] = fork ();
            if (procHijo [n] == 0) {
                execl ("reservas", "reservas",iAux, (char *) NULL);
                return 0;
            }
            n++;

        }

        for(p = 0;p < numAdmin;p++){
            procHijo[n] = fork ();
            if (procHijo [n] == 0) {
                execl ("admin", "admin",iAux, (char *) NULL);
                return 0;
            }
            

            n++;
        }
       
        for(y = 0;y < numConsultas;y++){
            procHijo[n] = fork ();
            if (procHijo [n] == 0) {
                execl ("consultas", "consultas",iAux, (char *) NULL);
                return 0;
            }
            n++;

        }
       
        

        

    }

    fclose (ficheroIn);
    printf("dale\n");
   
    return 0;


}
