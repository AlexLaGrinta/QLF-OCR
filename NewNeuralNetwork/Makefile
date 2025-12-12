CC = gcc
CFLAGS = -Wall -Wextra -O3 -g
CDM = -lm -lSDL2 -lSDL2_image

SRC = new_neural_network.c main_test.c bmp_converter.c
OBJ = $(SRC:.c=.o)
TARGET = test_nn

.INTERMEDIATE: $(OBJ)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(CDM)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)
