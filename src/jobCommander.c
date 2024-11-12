#include "../include/jobCommander.h"
#include "../include/myFunctions.h"

int main(int argc, char** argv)
{
    if (argc < 4)
    {
        printf("Wrong number of arguments [given: %d], required 4 or more !\n", argc);
        exit(1);
    }

    char* server_name = argv[1];
    int port = atoi(argv[2]);
    char* command = getCommand(argc, argv);

    connectToServer(server_name, port, command);

    free(command);

    return 1;
}

void connectToServer(char* server_name, int port, char* command)
{
    int sock;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket error");
        exit(1);
    } 

    struct sockaddr_in server;
    struct sockaddr* server_ptr = (struct sockaddr*)&server;
    struct hostent* rem;

    /* Find and set up server address */
    if ((rem = gethostbyname(server_name)) == NULL)
    {
        perror("gethostbyname");
        exit(1);
    }
    server.sin_family = AF_INET;
    memcpy(&server.sin_addr, rem->h_addr_list[0], rem->h_length);
    server.sin_port = htons(ntohs(port));

    int err;
    if ((err = connect(sock, (struct sockaddr*) &server, sizeof(server))) < 0)
    {
        perror2("Connect error", err);
        close(sock);
        exit(1);
    }

    printf("\nConnected to the server %s:%d\n\n", rem->h_name, server.sin_port);

    /* Send our command */
    sendMessage(sock, command);

    /* Set number of iterations based on command type */
    enum Command type = getCommandType(command);
    int iterations;
    switch (type)
    {
        case IssueJob:
            iterations = 3;
            break;
        case Exit:
        case SetConcurrency:
        case Stop:
        case Poll:
            iterations = 1;
            break;
        default:
            iterations = 0;
            break;
    }

    /* 
        Why read in a for loop ? Each iteration reads something different. More specifically:
        1. Read response from server - server will respond in a certain way for each command we give
        2. Read job output - server will read the jobID line required for debuggin purposes
        3. Read command output from file
        { 2-3 are result from the sendFile function }
    */
   for (int i=0; i<iterations; i++)
    {
        char* response = readMessage(sock);

        /* Meaning that the job got removed while it was on queue :( - Or server terminated */
        if (type == IssueJob && (strcmp(response, "REMOVED") == 0 || (strcmp(response, "SERVER TERMINATED BEFORE EXECUTION") == 0))) 
        {
            free(response);
            break;
        }
        printf("%d: %s\n\n", i, response);
        
        free(response);
    }

    close(sock);
}

char* getCommand(int argc, char** argv)
{
    int length = 0;
    char* retVal;
    for (int i=3; i<argc; i++)
        length += strlen(argv[i]) + 1; /* Will make sense later */
    length += 1;

    retVal = malloc(sizeof(char) * length);
    strcpy(retVal, argv[3]);
    if (argc > 4)
    {
        strcat(retVal, " ");
        for (int i=4; i<argc; i++)
        {
            strcat(retVal, argv[i]);
            strcat(retVal, " ");         /* Now i hope it does */
        }
        retVal[strlen(retVal)-1] = '\0';
    }

    return retVal;
}