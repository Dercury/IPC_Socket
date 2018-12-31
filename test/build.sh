#! /bin/bash

gcc -Wall -g -fPIC -shared -I../include -I../src ../src/ipcs_server.c ../src/ipcs_common.c ../src/ipcs_client.c -o libipcs.so

gcc -Wall -g -I../include ./server_main.c ./libipcs.so -lpthread -o server.exe

gcc -Wall -g -I../include ./client_main.c ./libipcs.so -lpthread -o client.exe


