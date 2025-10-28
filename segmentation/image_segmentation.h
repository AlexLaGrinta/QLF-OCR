#ifndef IMAGE_PROCESSING_H
#define IMAGE_PROCESSING_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdint.h>

// Définition des structures
typedef struct {
    uint8_t* data;
    int width;
    int height;
} BinaryImage;

typedef struct {
    uint8_t* data;
    int width;
    int height;
    int x;
    int y;
} Character;

// Fonctions pour la segmentation
BinaryImage* create_binary_image(SDL_Surface* surface);
void free_binary_image(BinaryImage* bin);

// Fonctions pour le dessin (compatibles avec SDL_Surface)
void draw_rectangle(SDL_Surface* surf, int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b);
void save_surface(const char* filename, SDL_Surface* img);

// Encadre la grille de lettres (rectangle rouge autour de la zone principale)
void highlight_letter_grid(SDL_Surface* surf);

// Encadre la liste de mots à rechercher (rectangle rouge en bas de l'image)
void highlight_search_words(SDL_Surface* surf);

#endif // IMAGE_PROCESSING_H

