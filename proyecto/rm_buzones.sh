#!/bin/bash

if [ $# -ne 2 ]; then
  echo "Se necesitan dos argumentos, el primero el primer id de buzon, el segundo, el último id de buzon a borrar"
  exit 1
fi

if [ $1 -ge $2 ]; then
  echo "El primer argumento debe ser menor que el segundo"
  exit 1
fi

i=$1

while [ $i -le $2 ]
do
    sudo ipcrm -q $i
    i=$((i+1))
done

echo "Memorias borradas"

exit 0
