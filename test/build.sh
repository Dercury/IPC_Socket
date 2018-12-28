#! /bin/bash

gcc -I../include -I../src ../src/ipcs_server.c ../src/ipcs_common.c ./server_main.c -lpthread -o server.exe

gcc -I../include -I../src ../src/ipcs_client.c ../src/ipcs_common.c ./client_main.c -lpthread -o client.exe


