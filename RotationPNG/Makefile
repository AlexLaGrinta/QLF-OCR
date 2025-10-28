CC = gcc
CFLAGS = -Wall -Wextra -O2
LIBS = -lpng -lm
TARGET = rotate_png
SRC = rotate_png.c
OBJ = $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

re: clean all

run: $(TARGET)
	./$(TARGET) input.png 45

