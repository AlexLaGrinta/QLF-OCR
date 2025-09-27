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


typedef struct {
    BinaryMatrix*** matrices;      // [26][N][BinaryMatrix]
    int* samples_per_letter;       // Nombre d'échantillons par lettre (A=0, B=1,...)
} LetterDataset;

void print_matrix(BinaryMatrix matrix);
BinaryMatrix img_to_binary_matrix_strict10(const char *path);
LetterDataset load_dataset(const char* dataset_path);
void free_dataset(LetterDataset* dataset);

#endif