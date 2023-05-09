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

     /*if (argc != 2){
        printf("La forma correcta de ejecución es: %s \"fich.conf\"\n", argv[0]);
        return -1;
    }*/
    char entrada [50], numeros [3];
    int i, j, k, cont, nNodosAux, numProcesosAux,nProcConPrio;
    printf("ha pasado por aqui 1");
   
    FILE * ficheroIn = fopen (argv [1], "r");
        printf("ha pasado por aqui 2");


    if (ficheroIn == NULL) {

        printf ("Error: Fichero de entrada no encontrado. \n\n");
        return 0;
    }


    // Esta primera parte lee l hasta que se encuentra una linea solo con un \n

    fgets (entrada, 50, ficheroIn);


     do {

        char variable [15] = {'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0'};

        for (i = 0, cont = 0; entrada [i] != '='; i++) {

            variable [i] = entrada [i];
            cont ++;
        }
                   
            printf("ha pasado por aqui 3");


        memcpy (numeros, &entrada [cont + 1], 3);
        i = atoi (numeros);

        if (strcmp (variable, "nNodos") == 0) {

            nNodosAux = i;
                        printf("ha pasado por aqui 4");

            printf ("nNodos = %i\n", nNodosAux);


         
        } else if (strcmp (variable, "numProcesos") == 0) {

            numProcesosAux = i;
            printf ("numProcesos = %i\n", numProcesosAux);
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
                               

            
        }

    }



    //////////////
    //entrada [0] = '0';
    //fgets (entrada, 50, ficheroIn);
     for (i = 0; i < nNodosAux && !feof (ficheroIn); i++) {

        /*printf ("Entrada = %s", entrada);

        char * nProcesosPrio = strtok (entrada, " ");*/

        char iAux [3];
        sprintf (iAux, "%i", i);

       

            for (k = 0; k < numProcesosAux; k++) {

                int procHijo = fork ();

                if (procHijo == 0) {

                    //fprintf (ficheroOut, "Proc %i, prio %i, nodo %i (%s - %s)\n", k, j, i, iAux, jAux);
                    //execl ("pagos", iAux, jAux, kAux, (char *) NULL);
                    execl ("pagos", "pagos",iAux, (char *) NULL);
                    execl ("reservas", "reservas",iAux, (char *) NULL);

                    return 0;
                }

                nProcConPrio ++;
            }

            //nProcesosPrio = strtok (NULL, " ");

        





     }

    fclose (ficheroIn);
    return 0;


}