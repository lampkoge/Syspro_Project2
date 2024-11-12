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
#include "Queue.h"

int current_jobID_counter = 0;
pthread_mutex_t counter_mutex;

int runningJobs = 0;
pthread_mutex_t runningJobs_mutex;

int concurrencyLevel = 1;
pthread_mutex_t concurrency_mutex;
pthread_cond_t concurrency_cond;

pthread_mutex_t socket_mutex;

Queue buffer;
pthread_mutex_t buffer_mutex;
pthread_cond_t buffer_cond;

pthread_t* thread_pool;
int thread_pool_size = 0;
int stop = 0;

void createWorkerThreadPool(int _thread_pool_size, int buffer_size);

void* workerThread(void* args);

void* controllerThread(void* args);

void serverInit(int port_num, int buffer_size, int thread_pool_size);

void counterAdd();

void initCondMutexVars();

void runJob(QueueNode triplet);

char* constructMessage(char* given_command, char* jobID, char* job);

void sendMessageToCommander(int sock, char* message);

QueueNode createTriplet(Args controller_args, char* command);

void runControllerJob(QueueNode triplet, char* command, int sock);

char** getExecArgs(char* job);

void sendFile(int sock, char* filename, char* jobID);

void issueJob(QueueNode triplet, char* command, int sock);

void changeConcurrency(int concurrency);

void setConcurrency(QueueNode triplet, int sock);

void stopJob(QueueNode triplet, int sock);

void poll(QueueNode triplet, int sock);