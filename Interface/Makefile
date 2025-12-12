# === Variables ===
CC = gcc
CFLAGS = -Wall -Wextra -O2 `pkg-config --cflags gtk+-3.0 sdl2 SDL2_image`
LIBS = `pkg-config --libs gtk+-3.0 sdl2 SDL2_image` -lm
TARGET = interface
SRC = interface.c ../Preprocessing/preprocessing.c ../RotationPNG/rotate_png.c
OBJ = $(SRC:.c=.o)

# === Règle par défaut ===
all: $(TARGET)

.INTERMEDIATE: $(OBJ)

# === Compilation de l’exécutable ===
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ) $(LIBS)

# === Compilation des fichiers objets ===
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# === Nettoyage ===
clean:
	rm -f $(OBJ) $(TARGET)

# === Recompilation complète ===
re: clean all

# === Règle pratique pour exécuter directement ===
run: all
	./$(TARGET)

