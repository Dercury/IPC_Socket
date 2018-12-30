#! /bin/bash

gcc -fPIC -shared -I../include -I../src ../src/ipcs_server.c ../src/ipcs_common.c ../src/ipcs_client.c -o libipcs.so

gcc -I../include ./server_main.c ./libipcs.so -lpthread -o server.exe

gcc -I../include ./client_main.c ./libipcs.so -lpthread -o client.exe


