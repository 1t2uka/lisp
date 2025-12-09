CC = gcc
TARGET = prompt
SOURCE = main.c 
OBJECT = $(SOURCE:.c=.o)
CFLAGS = -std=c99 -Wall -g
LIBS = -ledit

.PHONY: all
all:$(TARGET)

$(TARGET): $(OBJECT)
	$(CC) $(OBJECT) -o $@ $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f $(TARGET) $(OBJECT)
