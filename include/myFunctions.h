#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>

typedef struct args* Args;
struct args
{
    char* client_name;
    int client_socket;
};

enum Command 
{
    IssueJob,
    SetConcurrency,
    Stop,
    Poll,
    Exit
};

enum Command getCommandType(char* command);

/* Converts an integer to string - NEED TO BE free'd */
char* intToString(int num);

/* Function that deals with read-write synchronization to read message from given socket. */
char* readMessage(int sock);

/* Function that deals with read-write synchronization to send given message to given socket. */
void sendMessage(int sock, char* message);