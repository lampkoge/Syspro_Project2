#pragma once

#include "myFunctions.h"

typedef struct queue_node* QueueNode;

struct queue_node{
    char* jobID;
    char* job;
    enum Command type;
    int position;
    char* client_name;
    int client_socket;
    QueueNode next;
};

struct queue{
    int size;
    int max_size;
    QueueNode first;
    QueueNode last;
};

typedef struct queue* Queue;

Queue createQueue(int max_size);

int queueSize(Queue queue);

int queueMaxSize(Queue queue);

void queueInsert(Queue queue, QueueNode node);

void queueRemove(Queue queue, char* jobID);

void updatePositions(Queue queue, QueueNode to_be_removed);

QueueNode queueFind(Queue queue, char* jobID);

void queueDestroy(Queue queue);

QueueNode queuePop(Queue queue);

void queuePrint(Queue queue);

int queueRemove2(Queue queue, char* jobID);