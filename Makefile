CC = gcc
TARGET = prompt
SOURCE = parsing.c mpc.c
OBJECT = $(SOURCE:.c=.o)
CFLAGS = -std=c99 -Wall -g
LIBS = -ledit -lm

.PHONY: all
all:$(TARGET)

$(TARGET): $(OBJECT)
	$(CC) $(OBJECT) -o $@ $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f $(TARGET) $(OBJECT)
