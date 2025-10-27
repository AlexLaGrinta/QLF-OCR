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

