#include <sys/types.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]){

if (argc != 3){
  printf("Modo de empleo: %s \"primer id a eliminar\" \"último id a eliminar\"\n", argv[0]);
  return 0;
}

int i = 0;
int msg;

do{

  printf("La cola que se va a eliminar es: %d\n", i + atoi(argv[1]));
  msg = msgctl(i + atoi(argv[1]), IPC_RMID, NULL);
  if(msg == -1){
    perror("No se ha podido eliminar la cola\n");
    return -1;
  }
  i++;
}while((i + atoi(argv[1])) <= atoi(argv[2]));
printf("Colas eliminadas con éxito\n");

return 0;

}
