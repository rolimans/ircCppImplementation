CC=g++
LD=g++

CFLAGS= -std=c++11 -pthread -Wall -Wextra -Werror -pedantic -g -O0
LDLIBS=-lm -lstdc++ -lncurses -lreadline
DLDFLAGS=-g
LDFLAGS=

VFLAGS=--leak-check=full --show-leak-kinds=all --track-origins=yes

SRCS    := $(wildcard ./*.cpp)
SERVER_SRCS := $(filter-out %/client.cpp,$(SRCS))
CLIENT_SRCS := $(filter-out %/server.cpp,$(SRCS))
SERVER_OBJS    := $(patsubst ./%.cpp,./%.o,$(SERVER_SRCS))
CLIENT_OBJS    := $(patsubst ./%.cpp,./%.o,$(CLIENT_SRCS))

SERVER_TARGET=server
CLIENT_TARGET=client

./%.o: ./%.cpp ./%.hpp
	$(CC) $(CFLAGS) -c $< -o $@

server: $(SERVER_OBJS)
	$(LD) $(LDFLAGS) $^ -o $(SERVER_TARGET) $(LDLIBS)

client: $(CLIENT_OBJS)
	$(LD) $(LDFLAGS) $^ -o $(CLIENT_TARGET) $(LDLIBS)
clean:
	rm -rf $(SERVER_OBJS) $(CLIENT_OBJS) $(SERVER_TARGET) $(CLIENT_TARGET) vgcore*

hardClean:
	rm -rf $(OBJS) $(SERVER_TARGET) $(CLIENT_TARGET) $(TARGET).zip *.cpp *.hpp *.in *.out vgcore* in out README.txt

runServer: server
	./$(SERVER_TARGET)

runClient: client
	./$(CLIENT_TARGET)

zip:
	zip -r main.zip LICENSE README.md Makefile *.hpp *.cpp