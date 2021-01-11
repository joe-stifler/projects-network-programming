#!/bin/bash

IP="192.168.100.11"
PORT=$1

for i in {1..3}
do
    ./bin/cliente.o $IP $PORT &
done

