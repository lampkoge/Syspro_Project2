OBJS = build/jobCommander.o build/myFunctions.o build/Queue.o
OBJS2 = build/jobExecutorServer.o build/myFunctions.o build/Queue.o
SOURCE = src/jobCommander.c src/myFunctions.c src/Queue.c
SOURCE2 = src/jobExecutorServer.c src/myFunctions.c src/Queue.c
HEADER = include/myFunctions.h include/jobCommander.h include/jobExecutorServer.h include/Queue.h
OUT = bin/jobCommander
OUT2 = bin/jobExecutorServer
CC = gcc 
FLAGS = -g -c -lpthread
INCLUDES = -Iinclude 

all: $(OUT) $(OUT2)

$(OUT): $(OBJS)
	@mkdir -p bin
	$(CC) -g $(OBJS) -o $@ -lpthread

$(OUT2): $(OBJS2)
	@mkdir -p bin
	$(CC) -g $(OBJS2) -o $@ -lpthread

build/Queue.o: src/Queue.c
	@mkdir -p build
	$(CC) $(FLAGS) $(INCLUDES) $< -o $@

build/jobCommander.o: src/jobCommander.c
	@mkdir -p build
	$(CC) $(FLAGS) $(INCLUDES) $< -o $@

build/jobExecutorServer.o: src/jobExecutorServer.c
	@mkdir -p build
	$(CC) $(FLAGS) $(INCLUDES) $< -o $@

build/myFunctions.o: src/myFunctions.c
	@mkdir -p build
	$(CC) $(FLAGS) $(INCLUDES) $< -o $@

clean:
	rm -f $(OBJS) $(OUT)
	rm -f $(OBJS2) $(OUT2)