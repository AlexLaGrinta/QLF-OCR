# === Variables ===
CC = gcc
CFLAGS = -Wall -Wextra -O2 `pkg-config --cflags gtk+-3.0`
LIBS = `pkg-config --libs gtk+-3.0` -lm
TARGET = interface
SRC = interface.c
OBJ = $(SRC:.c=.o)

# === Règle par défaut ===
all: $(TARGET)

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

