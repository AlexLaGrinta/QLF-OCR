#ifndef BMP_CONVERTER_H
#define BMP_CONVERTER_H
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

typedef struct {
    int w, h;
    uint8_t *data; // w*h, valeurs 0 ou 1
} BinaryMatrix;

void print_matrix(BinaryMatrix matrix);
BinaryMatrix img_to_binary_matrix_strict10(const char *path);


#endif