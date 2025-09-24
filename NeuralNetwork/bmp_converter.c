#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <err.h>
#include "bmp_converter.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>


static int is_bmp_ext(const char *path) {
    if (!path) return 0;
    const char *dot = strrchr(path, '.');
    if (!dot) return 0;

    return strcasecmp(dot, ".bmp") == 0;

}

void print_matrix(BinaryMatrix matrix)
{
    for (int y = 0; y < matrix.h; y++)
    {
        for (int x = 0; x < matrix.w; x++)
        {
            size_t i = (size_t)y * (size_t)matrix.w + (size_t)x;
            printf("%i ", matrix.data[i]);
        }
        printf("\n");
    }
}


// noir=1, blanc=0
BinaryMatrix img_to_binary_matrix_strict10(const char *path)
{
    BinaryMatrix out = (BinaryMatrix){0,0,NULL};
    if (!path) return out;

    SDL_Surface *src = is_bmp_ext(path) ? SDL_LoadBMP(path) : IMG_Load(path);
    if (!src) {
        fprintf(stderr, "Echec chargement: %s\n", is_bmp_ext(path) ? SDL_GetError() : IMG_GetError());
        return out;
    }
    SDL_Surface *rgba = SDL_ConvertSurfaceFormat(src, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(src);
    if (!rgba) {
        fprintf(stderr, "Echec conversion format: %s\n", SDL_GetError());
        return out;
    }

    out.w = rgba->w;
    out.h = rgba->h;
    out.data = (uint8_t*)malloc((size_t)out.w * (size_t)out.h);
    if (!out.data) {
        fprintf(stderr, "Echec allocation\n");
        SDL_FreeSurface(rgba);
        out.w = out.h = 0;
        return out;
    }

    for (int y = 0; y < out.h; ++y) {
        const uint32_t *px = (const uint32_t*)((const uint8_t*)rgba->pixels + (size_t)y * (size_t)rgba->pitch);
        for (int x = 0; x < out.w; ++x) {
            uint8_t r, g, b, a;
            SDL_GetRGBA(px[x], rgba->format, &r, &g, &b, &a);
            // Image N&B stricte → R=G=B∈{0,255}
            out.data[(size_t)y * (size_t)out.w + (size_t)x] = (r == 0) ? 1 : 0; // noir=1, blanc=0
        }
    }

    SDL_FreeSurface(rgba);
    return out;
}
