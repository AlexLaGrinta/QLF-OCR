# === Variables ===
CC = gcc
CFLAGS = -Wall -Wextra -O2 `pkg-config --cflags gtk+-3.0 sdl2 SDL2_image`
LIBS = `pkg-config --libs gtk+-3.0 sdl2 SDL2_image` -lm
TARGET = ocr_gui
SRC = interface.c
OBJ = $(SRC:.c=.o)

all: $(TARGET)

.INTERMEDIATE: $(OBJ)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ) $(LIBS)


%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)
	rm -rf ../ocr
	rm -rf ../output

re: clean all

run: all
	./$(TARGET)

