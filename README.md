# About code

Compile : make all
Clean : make clean 

# jobExecutorServer

Command to run server (in bin folder): ./jobExecutorServer <port_number> <buffer_size> <thread_pool_size>

1. initializes buffer - the queue that will keep the jobs to be executed and the condition
variables
2. creates the worker threadpool 
3. Starts the server amd listens for client connections.

Uses mutexes and condition variables to manage access to shared resources and ensure thread
synchronization.

# Worker Threads 

Continuously wait for jobs to be added in the queue. When a job is added, check the concurrency
Level and then execute the job if it is allowed.
Jobs get executed by the runJob() function.

# Controller Threads

Handles client commands such as :
- issueJob : adds a job to the queue
- setConcurrency : sets the concurrency level to given num
- stop : stops the specified job - or returns message that it was not found in queue
- poll : returns jobs to be executed by current client. The way this works is i keep the client name 
on the QueueNode so i check every triplet that has the clients name.
- exit : Terminates server

# jobCommander

Command to run Commander (in bin folder) : ./jobCommander <host_name> <port_number> <command>

Connects to server and sends commands. Based on the command it send it waits for the appropriate 
message (basically the number of reads it does differs for each command).

# Notes

- Synchronization between server - commander communication:
It is done by the sendMessage and readMessage functions. How these functions work 
is analyzed in the comments.

- The folders bin/ and build/ are created on command make all if they dont exist 

- The poll command returns a list of the current client's jobs that exist in the queue. For example :
Suppose i issue a job from the linux04 machine. The server queue has 1 job in it. If i run the poll
command from the linux05 machine, it will return nothing. If i run it from the linux04 machine it will
return the job issued.

- Created an enum for the commands so that i don't have to use strcmp at all times.

Everything else concerning functionality and algorithms of each function is explained in the code's comments.  

# Tests 

Here are the tests i ran on the linux systems.

linux03: ./bin/jobExecutorServer 12345 2 2

[Command that checks issueJob functionality]
linux04: ./bin/jobCommander linux03.di.uoa.gr 12345 issueJob ls -l      

[Commands that check setConcurrency and issueJob functionality]
linux04: ./bin/jobCommander linux03.di.uoa.gr 12345 setConcurrency 0
linux04: ./bin/jobCommander linux03.di.uoa.gr 12345 issueJob ls
    [linux04: wait for concurrency to allow job to be ran]
linux05: ./bin/jobCommander linux03.di.uoa.gr 12345 setConcurrency 1
    [linux04: print job output]

[Commands that check multiple jobs execution]
linux04: ./bin/jobCommander linux03.di.uoa.gr 12345 issueJob ./progDelay 100
linux05: ./bin/jobCommander linux03.di.uoa.gr 12345 issueJob ls -l
    [linux05: different worker thread handles it and prints output]
linux05: ./bin/jobCommander linux03.di.uoa.gr 12345 issueJob ./progDelay 100
linux05: ./bin/jobCommander linux03.di.uoa.gr 12345 issueJob ./progDelay 100
    [this job waits for the other 2 ./progDelay programs to finish execution, so that it can run (we have 2 worker threads)]

[Commands that check poll functionality]
linux04: ./bin/jobCommander linux03.di.uoa.gr 12345 setConcurrency 0
linux04: ./bin/jobCommander linux03.di.uoa.gr 12345 issueJob ls
    [linux04: wait for concurrency to allow job to be ran]
linux05: ./bin/jobCommander linux03.di.uoa.gr 12345 poll
    [linux05: returns empty line]
linux04: ./bin/jobCommander linux03.di.uoa.gr 12345 poll
    [linux04: returns the job]
linux05: ./bin/jobCommander linux03.di.uoa.gr 12345 stop job_0
    [linux05: JOB <job_0> REMOVED ]
linux05: ./bin/jobCommander linux03.di.uoa.gr 12345 exit
    [linux05: SERVER TERMINATED]