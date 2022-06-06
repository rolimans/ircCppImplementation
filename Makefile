CC=gcc
LD=gcc

CFLAGS=-Wall -Wextra -Werror -pedantic -g -O0
LDLIBS=-lm -lstdc++
DLDFLAGS=-g
LDFLAGS=

VFLAGS=--leak-check=full --show-leak-kinds=all --track-origins=yes

SRCS    := $(wildcard ./*.cpp)
OBJS    := $(patsubst ./%.cpp,./%.o,$(SRCS))

TARGET=main

./%.o: ./%.cpp
	$(CC) $(CFLAGS) -c $< -o $@

./%.o: ./%.cpp ./%.hpp
	$(CC) $(CFLAGS) -c $< -o $@

all: $(OBJS)
	$(LD) $(LDFLAGS) $^ -o $(TARGET) $(LDLIBS)

debug: $(OBJS)
	$(LD) $(DLDFLAGS) $(OBJS) -o $(TARGET) $(LDLIBS)

clean:
	rm -rf $(OBJS) $(TARGET)

hardClean:
	rm -rf $(OBJS) $(TARGET) $(TARGET).zip *.cpp *.hpp *.in *.out vgcore* in out README.txt

run: all
	./$(TARGET)

valgrind: debug
	valgrind $(VFLAGS) ./$(TARGET)

zip:
	zip -r $(TARGET).zip Makefile *.hpp *.cpp