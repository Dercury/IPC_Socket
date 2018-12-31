#! /bin/bash

rm -fv libipcs.so server.exe client.exe

gcc -Wall -g -fPIC -shared -I../include -I../src ../src/ipcs_server.c ../src/ipcs_common.c ../src/ipcs_client.c -o libipcs.so

gcc -Wall -g -I../include -I. ./server_main.c ./libipcs.so -lpthread -o server.exe

gcc -Wall -g -I../include -I. ./client_main.c ./libipcs.so -lpthread -o client.exe


