#include "../include/myFunctions.h"

char* intToString(int num){
    int len = snprintf(NULL, 0, "%d", num);
    char *str = malloc(sizeof(char) * (len + 1));
    snprintf(str, len + 1, "%d", num);
    return str;
}

void sendMessage(int sock, char* message)
{
    /* Send number of bytes */
    int message_length = strlen(message);
    if (write(sock, &message_length, sizeof(int)) != sizeof(int))
    {
        perror("Write error");
        exit(1);
    }
    /* Wait for other process to respond with OK message */
    int bytes;
    if (read(sock, &bytes, sizeof(int)) != sizeof(int))
    {
        perror("Read error");
        exit(1);
    }
    /* Send actual message */
    if (write(sock, message, message_length) != message_length)
    {
        perror("Write error");
        exit(1);
    }
}

char* readMessage(int sock)
{
    /* Read number of bytes about to be send */
    int bytes;
    if (read(sock, &bytes, sizeof(int)) != sizeof(int))
    {
        perror("Read error");
        exit(1);
    }
    /* Send message that we are ready to receive message */
    int ready = 1;
    if (write(sock, &ready, sizeof(int)) != sizeof(int))
    {
        perror("Write error");
        exit(1);
    }

    /* Now read message */
    int read_bytes = 0;
    char buf[bytes];
    memset(buf, 0, bytes);
    if ((read_bytes = read(sock, buf, bytes)) != bytes)
    {
        perror("Read error");
        exit(1);
    }
    buf[read_bytes] = '\0';
    char* message = malloc(sizeof(char) * strlen(buf) + 1);
    strcpy(message, buf);
    memset(buf, 0, bytes);

    return message;
}

enum Command getCommandType(char* command)
{
    char* command2 = strtok(command, " ");
    if (strcmp(command, "issueJob") == 0) return IssueJob;
    else if (strcmp(command, "setConcurrency") == 0) return SetConcurrency;
    else if (strcmp(command, "poll") == 0) return Poll;
    else if (strcmp(command, "stop") == 0) return Stop;
    else return Exit;
}