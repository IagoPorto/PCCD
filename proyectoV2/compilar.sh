#!/bin/bash

gcc -Wall consultas.c -o consultas -lpthread
gcc -Wall admin.c -o admin -lpthread
gcc -Wall reservas.c -o reservas -lpthread
gcc -Wall pagos.c -o pagos -lpthread
gcc -Wall receptor.c -o receptor -lpthread
gcc -Wall anulaciones.c -o anulaciones -lpthread

echo "Todo compilado, dale."

exit
