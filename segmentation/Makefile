# Makefile pour le projet de segmentation de caractères

CC = gcc
CFLAGS = -Wall -Wextra -O2 -g `sdl2-config --cflags`
LDFLAGS = `sdl2-config --libs` -lSDL2_image -lm

# Fichiers sources et objets
SOURCES = segmentation.c image_segmentation.c char_segmentation.c
OBJECTS = $(SOURCES:.c=.o)
EXECUTABLE = segmentation

# Règle par défaut
all: $(EXECUTABLE)

# Compilation de l'exécutable
$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)
	@echo "Build complete: $(EXECUTABLE)"

# Compilation des fichiers objets
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Nettoyage
clean:
	rm -f $(OBJECTS) $(EXECUTABLE)
	rm -rf output
	@echo "Clean complete"


