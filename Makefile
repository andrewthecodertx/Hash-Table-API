CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g
LDFLAGS =

SRCS = main.c hashtable.c
OBJS = $(SRCS:.c=.o)
TARGET = hashtable_demo

.PHONY: all clean

RM = rm -f

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $(TARGET) $(OBJS)
	$(RM) $(OBJS)

%.o: %.c hashtable.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

