#ifndef BMP_CONVERTER_H
#define BMP_CONVERTER_H
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "neural_network.h"



typedef struct {
    double*** matrices;            
    int* samples_per_letter;     
} DoubleDataset;


double* img_to_binary_matrix(const char *path);
DoubleDataset load_double_dataset(const char* dataset_path);
void free_double_dataset(DoubleDataset* dataset);
SampleData** double_dataset_to_samples(DoubleDataset* dataset, size_t* out_count);
void print_matrix_double(double* matrix);

#endif