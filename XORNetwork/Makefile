CC = gcc
CFLAGS = -O0 -Wall -Wextra
LDFLAGS = -lm
TARGET = xor
SOURCE = xor_network.c

$(TARGET): $(SOURCE)
	$(CC) -o $(TARGET) $(SOURCE) $(LDFLAGS) $(CFLAGS)

clean:
	rm -f $(TARGET)

.PHONY: clean
