#include "../include/jobExecutorServer.h"
#include "../include/myFunctions.h"

#define JOB "job_"

int main(int argc, char** argv)
{
    if (argc < 4)
    {
        printf("Wrong number of arguments [arguments passed : %d], required 4!\n", argc);
        exit(1);
    }

    int port_num = atoi(argv[1]);
    int buffer_size = atoi(argv[2]);
    int thread_pool_size = atoi(argv[3]);
    
    if (buffer_size <= 0)
    {
        printf("Buffer must be greater than 0 ! [Current value : %d]\n", buffer_size);
        exit(1);
    }

    printf("port: %d, buffersize: %d, thread pool size: %d\n", port_num, buffer_size, thread_pool_size);

    buffer = createQueue(buffer_size);

    initCondMutexVars();

    createWorkerThreadPool(thread_pool_size, buffer_size);

    serverInit(port_num, buffer_size, thread_pool_size);

    return 1;
}

void initCondMutexVars()
{
    pthread_mutex_init(&socket_mutex, 0);
    pthread_mutex_init(&counter_mutex, 0);
    pthread_mutex_init(&concurrency_mutex, 0);
    pthread_mutex_init(&buffer_mutex, 0);
    pthread_mutex_init(&runningJobs_mutex, 0);

    pthread_cond_init(&concurrency_cond, 0);
    pthread_cond_init(&buffer_cond, 0);
}

void createWorkerThreadPool(int _thread_pool_size, int buffer_size)
{
    thread_pool_size = _thread_pool_size;
    thread_pool = malloc(sizeof(*thread_pool) * thread_pool_size);
    for (int i=0; i<thread_pool_size; i++)
    {
        int* arg = malloc(sizeof(int));
        *arg = buffer_size;
        int err;
        if ((err = pthread_create(&thread_pool[i], NULL, workerThread, arg)) != 0){
            perror("Create worker thread");
            exit(1);
        }
        printf("Created worker thread %ld\n", thread_pool[i]);
    }
}

void* workerThread(void* args)
{
    while(1)
    {
        /* Lock buffer mutex and wait for jobs to be available in the queue */
        pthread_mutex_lock(&buffer_mutex);
        while(buffer != NULL && (queueSize(buffer) == 0))
            pthread_cond_wait(&buffer_cond, &buffer_mutex);
        pthread_mutex_unlock(&buffer_mutex);

        /* If concurrency does not allow us to run a job, we don't pop it from the queue */
        pthread_mutex_lock(&concurrency_mutex);
        while (runningJobs >= concurrencyLevel || concurrencyLevel == 0)
            pthread_cond_wait(&concurrency_cond, &concurrency_mutex);
        
        /* Increment the number of running jobs */
        runningJobs++;
        pthread_mutex_unlock(&concurrency_mutex);

        pthread_mutex_lock(&buffer_mutex);
        /* Pop job from the queue - Signal that slot is now available */
        QueueNode triplet = queuePop(buffer);
        /* NULL is most likely when the job got removed (stop job) so we just continue */
        if (triplet != NULL) 
            printf("<%s, %s, %d>\n", triplet->jobID, triplet->job, triplet->client_socket);
        pthread_cond_broadcast(&buffer_cond);
        pthread_mutex_unlock(&buffer_mutex);

        /* Run the job */
        if (triplet != NULL)
            runJob(triplet);

        /* Decrement the number of running jobs and signal concurrency condition */
        pthread_mutex_lock(&concurrency_mutex);
        runningJobs--;
        pthread_cond_broadcast(&concurrency_cond);
        pthread_mutex_unlock(&concurrency_mutex);
    }
}

void runJob(QueueNode triplet)
{
    if (triplet == NULL) return;
    pid_t pid;
    switch (triplet->type)
    {
        case IssueJob:
            /* fork and run with exec */
            pid = fork();
            if (pid < 0)
            {
                perror("Fork error");
                exit(1);
            }
            if (pid == 0)
            {
                char filename[40];
                snprintf(filename, sizeof(filename), "%d.output", getpid());
                
                int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd == -1)
                {
                    perror("Open error");
                    exit(1);
                }
                
                char** args = getExecArgs(triplet->job);
                if (dup2(fd, STDOUT_FILENO) == -1)
                {
                    perror("dup2 error");
                    exit(1);
                }

                close(fd);

                execvp(args[0], args);
                perror("Execvp failed");
                exit(1);
            }
            else
            {
                int status;
                waitpid(pid, &status, 0);

                /* Check that file is created */
                char filename[40];
                snprintf(filename, sizeof(filename), "%d.output", pid);
                sendFile(triplet->client_socket, filename, triplet->jobID);
            }

            break;
        default:
            printf("You shouldn't be here\n");
    }

    /* Free queue node */
    free(triplet->jobID);
    free(triplet->job);
    free(triplet);
}

void serverInit(int port_num, int buffer_size, int thread_pool_size)
{
    int sock;

    struct sockaddr_in server, client;
    struct sockaddr* serverptr = (struct sockaddr*) &server;
    struct sockaddr* clientptr = (struct sockaddr*) &client;
    struct hostent* rem, *ser;

    /* Create socket */
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket create error");
        exit(1);
    }

    /* Address family IPv4 */
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(ntohs(port_num));

    /* Bind socket to address */
    if (bind(sock, serverptr, sizeof(server)) < 0)
    {
        perror("Bind error");
        exit(1);
    }

    /* Listen for connections */
    if (listen(sock, 100) < 0)
    {
        perror("Listen error");
        exit(1);
    }

    while (1)
    {
        int newsock;
        socklen_t clientlen = sizeof(client);
        
        printf("\n\nWaiting for client to connect (%d)...\n\n", server.sin_port);
        /* Accept connection - Blocks until connection is made */
        if ((newsock = accept(sock, clientptr, &clientlen)) < 0)
        {
            perror("Accept error");
            exit(1);
        }

        /* Find client's name after we got a connection */
        if ((rem = gethostbyaddr((char*)&client.sin_addr.s_addr, sizeof(client.sin_addr.s_addr), client.sin_family)) == NULL)
        {
            perror("gethostbyaddr");
            exit(1);
        }

        printf("Accepted connection from %s socket -> %d\n", rem->h_name, newsock);
        
        /* Create controller thread */
        pthread_t controller_thread;
        Args controller_args = malloc(sizeof(*controller_args));
        controller_args->client_name = malloc(strlen(rem->h_name) + 1);
        strcpy(controller_args->client_name, rem->h_name);
        controller_args->client_socket = newsock;
        pthread_create(&controller_thread, NULL, controllerThread, controller_args);
    }
}

/* Server controller thread that reads command given by client */
void* controllerThread(void* args)
{
    Args controller_args = (Args)args;
    int sock = controller_args->client_socket;
    
    char* command = readMessage(sock);

    pthread_mutex_lock(&buffer_mutex);
    /* Must be blocked when queue is full - Gets unblocked when we pop a job from the queue */
    while (queueSize(buffer) == queueMaxSize(buffer))
        pthread_cond_wait(&buffer_cond, &buffer_mutex);
    pthread_mutex_unlock(&buffer_mutex);

    /* Create triplet */
    QueueNode triplet = createTriplet(controller_args, command);
    printf("Controller thread [%ld] created <%s, %s, %d, %d> \n", pthread_self(), triplet->jobID, triplet->job, triplet->type, triplet->client_socket);
    runControllerJob(triplet, command, sock);

    free(command);
    
    pthread_exit(0);
}

void runControllerJob(QueueNode triplet, char* command, int sock)
{
    switch (triplet->type)
    {
        /* Add job to queue and return message to client */
        case IssueJob:
            issueJob(triplet, command, sock);
            break;
        case SetConcurrency:
            setConcurrency(triplet, sock);
            break;
        case Poll:
            poll(triplet, triplet->client_socket);
            break;
        case Stop:
            stopJob(triplet, sock);
            break;
        /* Waits for running jobs to finish - Destroys buffer and terminates main thread and sends message */
        case Exit:
            pthread_mutex_lock(&buffer_mutex);
            /* Send message to every client with queued job */
            for (QueueNode node = buffer->first; node != NULL; node = node->next)
            {
                sendMessage(node->client_socket, "SERVER TERMINATED BEFORE EXECUTION");
                queueRemove(buffer, node->jobID);
            }
            pthread_mutex_unlock(&buffer_mutex);

            sendMessage(sock, "SERVER TERMINATED");

            exit(1);
            break;
        default:
            printf("Not defined job \n");
            exit(1);
    }
}

void issueJob(QueueNode triplet, char* command, int sock)
{
    /* We are about to write on queue so lock-unlock */
    pthread_mutex_lock(&buffer_mutex);
    queueInsert(buffer, triplet);
    pthread_cond_signal(&buffer_cond); 
    pthread_mutex_unlock(&buffer_mutex);

    char* message = constructMessage(command, triplet->jobID, triplet->job);

    counterAdd();

    /* Send response to client */
    pthread_mutex_lock(&socket_mutex);
    sendMessage(sock, message);
    pthread_mutex_unlock(&socket_mutex);
}

void changeConcurrency(int concurrency)
{
    pthread_mutex_lock(&concurrency_mutex);

    concurrencyLevel = concurrency;

    pthread_cond_signal(&concurrency_cond);
    pthread_mutex_unlock(&concurrency_mutex); 
}

void setConcurrency(QueueNode triplet, int sock)
{
    char* dupstr = strdup(triplet->job);
    char* command2 = strtok(dupstr, " ");
    char* rest = strtok(NULL, "");
    int n = atoi(rest);
    
    changeConcurrency(n);
    char* message = malloc(sizeof(char) * (strlen("\tCONCURRENCY SET TO ") + strlen(rest) + 1) );
    strcpy(message, "\tCONCURRENCY SET TO ");
    strcat(message, rest);
    
    pthread_mutex_lock(&socket_mutex);
    sendMessage(sock, message);
    pthread_mutex_unlock(&socket_mutex);

    free(dupstr);
}

void stopJob(QueueNode triplet, int sock)
{
    char* dupstr = strdup(triplet->job);
    char* command2 = strtok(dupstr, " ");
    char* jobID = strtok(NULL, "");

    int client_sock = queueRemove2(buffer, jobID);
    pthread_mutex_lock(&buffer_mutex);
    /* Search buffer for triplet */
    if (client_sock > 0)
    {
        char* message = malloc(sizeof(char) * strlen(triplet->jobID) + 17);
        strcpy(message, "JOB <");
        strcat(message, jobID);
        strcat(message, "> REMOVED");

        /* Notifies both the client that issued the job and the client that terminated it */
        pthread_mutex_lock(&socket_mutex);
        /* The one that is hopelessly waiting for a response */
        sendMessage(sock, message);
        /* Controller (the one that sent the stop command) */
        sendMessage(client_sock, "REMOVED");
        pthread_mutex_unlock(&socket_mutex);

        pthread_cond_signal(&buffer_cond);
    }
    else
    {
        char* message = malloc(sizeof(char) * strlen(jobID) + 18);
        strcpy(message, "JOB <");
        strcat(message, jobID);
        strcat(message, "> NOTFOUND");
        
        pthread_mutex_lock(&socket_mutex);
        sendMessage(sock, message);
        pthread_mutex_unlock(&socket_mutex);
    }
    pthread_mutex_unlock(&buffer_mutex);
}

void poll(QueueNode triplet, int sock)
{
    int length = 0;
    pthread_mutex_lock(&buffer_mutex);
    for (QueueNode node = buffer->first; node != NULL; node = node->next)
        if (strcmp(node->client_name, triplet->client_name) == 0)
            length += (strlen(node->jobID) + strlen(node->job) + 6);

    char* message = malloc(length + 1);
    strcpy(message, "");
    for (QueueNode node = buffer->first; node != NULL; node = node->next)
    {
        if (strcmp(node->client_name, triplet->client_name) == 0)
        {
            strcat(message, "<");
            strcat(message, node->jobID);
            strcat(message, ", ");
            strcat(message, node->job);
            strcat(message, ">");
        }
    }
    pthread_mutex_unlock(&buffer_mutex);

    pthread_mutex_lock(&socket_mutex);
    sendMessage(sock, message);
    pthread_mutex_unlock(&socket_mutex);

    free(message);
}

char** getExecArgs(char* job)
{
    /* Parse to get number of arguments by counting whitespace and newline characters */
    int argc = 1;
    for (int i=0; job[i] != '\0'; i++)
        if (job[i] == ' ' || job[i] == '\n')
            argc++;

    /* Now we have the number of arguments (i hope), so we parse the string again to get the args */
    char** args = malloc(sizeof(char*) * (argc + 1));
    int index = 0, i=0;
    char* token = strtok(job, " \n");
    while (token != NULL)
    {
        if (i > 0)
        {
            args[index] = strdup(token);
            if (args[index] == NULL)
            {
                perror("Error on strdup");
                exit(1);
            }
            index++;
        }
        
        i++;
        token = strtok(NULL, " \n");
    }
    args[index] = NULL;

    return args;
}

void counterAdd()
{
    /* Lock mutex before accessing counter */
    pthread_mutex_lock(&counter_mutex);
    
    current_jobID_counter++;

    /* Unlock mutex */
    pthread_mutex_unlock(&counter_mutex);
}

char* constructMessage(char* given_command, char* jobID, char* job)
{
    char* retVal = NULL;
    char* cmd = strdup(given_command);
    char* command = strtok(cmd, " ");
    if (strcmp(command, "issueJob") == 0)
    {
        retVal = malloc(sizeof(char) * (strlen(JOB) + strlen(jobID) + strlen(job) + strlen("SUBMITTED") + 7));
        strcpy(retVal, "JOB");
        strcat(retVal, " <");
        strcat(retVal, jobID);
        strcat(retVal, ", ");
        strcat(retVal, job);
        strcat(retVal, "> SUBMITTED");
    }
    free(cmd);

    return retVal;
}

QueueNode createTriplet(Args controller_args, char* command)
{
    char* jobID_num = intToString(current_jobID_counter);
    QueueNode triplet = malloc(sizeof(*triplet));
    triplet->jobID = malloc(sizeof(char) * (strlen(JOB) + strlen(jobID_num)) + 1);
    strcpy(triplet->jobID, JOB);
    strcat(triplet->jobID, jobID_num);

    triplet->client_socket = controller_args->client_socket;
    triplet->client_name = malloc(strlen(controller_args->client_name) + 1);
    strcpy(triplet->client_name, controller_args->client_name);

    triplet->job = malloc(sizeof(char) * strlen(command) + 1);
    strcpy(triplet->job, command);

    triplet->type = getCommandType(command);

    free(controller_args->client_name);
    free(controller_args);

    return triplet;
}

void sendFile(int sock, char* filename, char* jobID)
{
    pthread_mutex_lock(&socket_mutex);
    FILE* fp = fopen(filename, "r");
    if (!fp)
    {
        perror("fopen error");
        exit(1);
    }

    /* First send the jobID message */
    char* str = "\n===============================================================================\n";
    char* jobMessage = malloc(sizeof(char) * (strlen(jobID) + strlen(str))  + 1);
    strcpy(jobMessage, str);
    strcat(jobMessage, jobID);
    sendMessage(sock, jobMessage);
    free(jobMessage);

    char content[2048];
    size_t bytes_read;
    while(!feof(fp))
    {
        bytes_read = fread(content, 1, sizeof(content), fp);
        content[bytes_read] = '\0';
        sendMessage(sock, content);
    }
    pthread_mutex_unlock(&socket_mutex);
    
    /* Delete output file */
    if (remove(filename) != 0)
    {
        perror("Failed to remove file");
        exit(1);
    }
}
