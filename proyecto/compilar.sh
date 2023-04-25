#!/bin/bash

#gcc consultas.c -o consultas -Wall
#gcc admin.c -o admin -Wall
#gcc reservas.c -o reservas -Wall
gcc -Wall pagos.c -o pagos -lpthread
gcc -Wall receptor.c -o receptor -lpthread
#gcc anulaciones.c -o anulaciones -Wall

echo "Todo compilado, si hay errores te jodes."

exit
