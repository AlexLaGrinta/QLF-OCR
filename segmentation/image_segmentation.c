#include "image_segmentation.h"
#include <stdio.h>
#include <stdlib.h>

// Création d'une BinaryImage à partir d'une SDL_Surface
BinaryImage* create_binary_image(SDL_Surface* surface) {
    if (!surface) return NULL;

    BinaryImage* bin = malloc(sizeof(BinaryImage));
    if (!bin) {
        fprintf(stderr, "malloc failed for BinaryImage\n");
        return NULL;
    }

    bin->width = surface->w;
    bin->height = surface->h;
    bin->data = malloc(bin->width * bin->height);
    if (!bin->data) {
        fprintf(stderr, "malloc failed for BinaryImage data\n");
        free(bin);
        return NULL;
    }

    SDL_LockSurface(surface);
    uint8_t* pixels = (uint8_t*)surface->pixels;

    for (int y = 0; y < bin->height; y++) {
        for (int x = 0; x < bin->width; x++) {
            uint32_t* pixel = (uint32_t*)(pixels + y * surface->pitch + x * 4);
            uint8_t r = (*pixel & surface->format->Rmask) >> surface->format->Rshift;
            bin->data[y * bin->width + x] = r;
        }
    }

    SDL_UnlockSurface(surface);
    return bin;
}

// Libération d'une BinaryImage
void free_binary_image(BinaryImage* bin) {
    if (bin) {
        free(bin->data);
        free(bin);
    }
}

// Dessin d'un rectangle sur une SDL_Surface
void draw_rectangle(SDL_Surface* surf, int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b) {
    if (!surf) return;

    uint32_t color = SDL_MapRGB(surf->format, r, g, b);
    uint32_t* pixels = (uint32_t*)surf->pixels;

    // Dessiner les bords horizontaux
    for (int i = x; i < x + w; i++) {
        if (i >= 0 && i < surf->w) {
            if (y >= 0 && y < surf->h) pixels[y * surf->w + i] = color;
            if (y + h - 1 >= 0 && y + h - 1 < surf->h) pixels[(y + h - 1) * surf->w + i] = color;
        }
    }

    // Dessiner les bords verticaux
    for (int j = y; j < y + h; j++) {
        if (j >= 0 && j < surf->h) {
            if (x >= 0 && x < surf->w) pixels[j * surf->w + x] = color;
            if (x + w - 1 >= 0 && x + w - 1 < surf->w) pixels[j * surf->w + (x + w - 1)] = color;
        }
    }
}

// Sauvegarde d'une SDL_Surface en BMP
void save_surface(const char* filename, SDL_Surface* img) {
    if (!img) return;
    if (SDL_SaveBMP(img, filename) != 0) {
        fprintf(stderr, "Erreur SDL_SaveBMP: %s\n", SDL_GetError());
    }
}

// Encadre la grille de lettres (zone principale du tableau) en rouge
void highlight_letter_grid(SDL_Surface* surf) {
    if (!surf) return;

    int y_limit = (int)(surf->h * 0.9f); // exclure ~10% du bas (zone des mots à rechercher)
    if (y_limit <= 0) y_limit = surf->h;

    SDL_LockSurface(surf);
    uint8_t* pixels = (uint8_t*)surf->pixels;

    int minx = surf->w, miny = y_limit, maxx = -1, maxy = -1;

    for (int y = 0; y < y_limit; y++) {
        for (int x = 0; x < surf->w; x++) {
            uint32_t* pixel = (uint32_t*)(pixels + y * surf->pitch + x * 4);
            uint8_t r = (*pixel & surf->format->Rmask) >> surf->format->Rshift;
            // considérer les pixels sombres comme appartenant à la grille
            if (r < 128) {
                if (x < minx) minx = x;
                if (x > maxx) maxx = x;
                if (y < miny) miny = y;
                if (y > maxy) maxy = y;
            }
        }
    }

    SDL_UnlockSurface(surf);

    // Rien détecté
    if (maxx < 0 || maxy < 0 || minx >= surf->w || miny >= surf->h) return;

    // Ajouter une petite marge et recadrer aux bornes
    int margin = 3;
    int x = (minx - margin < 0) ? 0 : (minx - margin);
    int y = (miny - margin < 0) ? 0 : (miny - margin);
    int right = (maxx + margin >= surf->w) ? (surf->w - 1) : (maxx + margin);
    int bottom = (maxy + margin >= y_limit) ? (y_limit - 1) : (maxy + margin);
    int w = right - x + 1;
    int h = bottom - y + 1;

    // Dessiner le rectangle rouge
    draw_rectangle(surf, x, y, w, h, 255, 0, 0);
}

// Encadre la liste de mots à rechercher (en bas de l'image) en rouge
void highlight_search_words(SDL_Surface* surf) {
    if (!surf) return;

    // Supposons que les mots à rechercher sont dans les 10% inférieurs
    int y_start = (int)(surf->h * 0.9f);
    if (y_start >= surf->h) return;

    SDL_LockSurface(surf);
    uint8_t* pixels = (uint8_t*)surf->pixels;

    int minx = surf->w, maxx = -1, miny = surf->h, maxy = -1;

    // Chercher les pixels sombres dans la zone du bas
    for (int y = y_start; y < surf->h; y++) {
        for (int x = 0; x < surf->w; x++) {
            uint32_t* pixel = (uint32_t*)(pixels + y * surf->pitch + x * 4);
            uint8_t r = (*pixel & surf->format->Rmask) >> surf->format->Rshift;
            if (r < 128) { // Pixel sombre (texte)
                if (x < minx) minx = x;
                if (x > maxx) maxx = x;
                if (y < miny) miny = y;
                if (y > maxy) maxy = y;
            }
        }
    }

    SDL_UnlockSurface(surf);

    // Rien détecté
    if (maxx < 0 || maxy < 0 || minx >= surf->w || miny >= surf->h) return;

    // Ajouter une petite marge et recadrer aux bornes
    int margin = 3;
    int x = (minx - margin < 0) ? 0 : (minx - margin);
    int y = (miny - margin < y_start) ? y_start : (miny - margin);
    int right = (maxx + margin >= surf->w) ? (surf->w - 1) : (maxx + margin);
    int bottom = (maxy + margin >= surf->h) ? (surf->h - 1) : (maxy + margin);
    int w = right - x + 1;
    int h = bottom - y + 1;

    // Dessiner le rectangle rouge autour de la zone des mots à rechercher
    draw_rectangle(surf, x, y, w, h, 255, 0, 0);
}

