#pragma once 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/wait.h> 
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>

#define perror2(s, e) fprintf(stderr, "%s : %s \n", s, strerror(e))

char* getCommand(int argc, char** argv);

void connectToServer(char* server_name, int port, char* command);