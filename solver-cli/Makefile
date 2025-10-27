CC = gcc
CFLAGS = -O2 -Wall -Wextra
TARGET = solver
SOURCE = solver.c

$(TARGET): $(SOURCE)
	$(CC) -o $(TARGET) $(SOURCE) $(CFLAGS)

clean:
	rm -f $(TARGET)

.PHONY: clean
