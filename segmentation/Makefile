#Makefile
CC      := gcc
TARGET  := main
SRC     := main.c segmentation.c char_segmentation.c image_segmentation.c
OBJ     := $(SRC:.c=.o)

CFLAGS  := -std=c11 -Wall -Wextra -O2 $(shell pkg-config --cflags sdl2 SDL2_image)
LDLIBS := -lSDL2 -lSDL2_image -lm

.PHONY: all clean
all: $(TARGET)

.INTERMEDIATE: $(OBJ)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)
	rm -rf chars
	rm output_with_rectangles.bmp

