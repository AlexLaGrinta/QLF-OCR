CC = gcc
CFLAGS = -O2 -Wall -Wextra
LDFLAGS = -lm
TARGET = xor
SOURCE = nxor_network.c

$(TARGET): $(SOURCE)
	$(CC) -o $(TARGET) $(SOURCE) $(LDFLAGS) $(CFLAGS)

clean:
	rm -f $(TARGET)

.PHONY: clean
