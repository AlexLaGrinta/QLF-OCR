#Makefile
CC      := gcc
TARGET  := ocr
SRC     := main.c preprocessing.c
OBJ     := $(SRC:.c=.o)

CFLAGS  := -std=c11 -Wall -Wextra -O2 $(shell pkg-config --cflags sdl2 SDL2_image)
LDLIBS := -lSDL2 -lSDL2_image

.PHONY: all clean
all: $(TARGET)

.INTERMEDIATE: $(OBJ)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)


