#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <err.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include "bmp_converter.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>


static int is_bmp_ext(const char *path) {
    if (!path) return 0;
    const char *dot = strrchr(path, '.');
    if (!dot) return 0;

    return strcasecmp(dot, ".bmp") == 0;

}


void print_matrix_double(double* matrix)
{
    for (int y = 0; y < 28; y++)
    {
        for (int x = 0; x < 28; x++)
        {
            size_t i = (size_t)y * (size_t)28 + (size_t)x;
            printf("%.0f ", matrix[i]);
        }
        printf("\n");
    }
}

double* img_to_binary_matrix(const char *path)
{
    if (!path) return NULL;

    SDL_Surface *src = is_bmp_ext(path) ? SDL_LoadBMP(path) : IMG_Load(path);
    if (!src) {
        fprintf(stderr, "Loading failed: %s\n", is_bmp_ext(path) ? SDL_GetError() : IMG_GetError());
        return NULL;
    }
    
    SDL_Surface *rgba = SDL_ConvertSurfaceFormat(src, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(src);
    if (!rgba) {
        fprintf(stderr, "Convert failed: %s\n", SDL_GetError());
        return NULL;
    }

    double* matrix = (double*)malloc(28 * 28 * sizeof(double));
    if (!matrix) {
        fprintf(stderr, "Allocation failed\n");
        SDL_FreeSurface(rgba);
        return NULL;
    }

    SDL_Surface *resized = SDL_CreateRGBSurface(0, 28, 28, 32, 0, 0, 0, 0);
    if (!resized) {
        fprintf(stderr, "Resize surface creation failed: %s\n", SDL_GetError());
        free(matrix);
        SDL_FreeSurface(rgba);
        return NULL;
    }
    
    SDL_BlitScaled(rgba, NULL, resized, NULL);
    SDL_FreeSurface(rgba);

    for (int y = 0; y < 28; y++) {
        const uint32_t *px = (const uint32_t*)((const uint8_t*)resized->pixels + (size_t)y * (size_t)resized->pitch);
        for (int x = 0; x < 28; x++) {
            uint8_t r, g, b, a;
            SDL_GetRGBA(px[x], resized->format, &r, &g, &b, &a);
            matrix[y * 28 + x] = (r == 0) ? 1.0 : 0.0;
        }
    }

    SDL_FreeSurface(resized);
    return matrix;
}


int is_directory(const char* path) {
    struct stat statbuf;
    return (stat(path, &statbuf) == 0) && S_ISDIR(statbuf.st_mode);
}


void free_double_dataset(DoubleDataset* dataset) {
    for (int i = 0; i < 26; i++) {
        if (dataset->matrices[i]) {
            for (int j = 0; j < dataset->samples_per_letter[i]; j++) {
                if (dataset->matrices[i][j]) {
                    free(dataset->matrices[i][j]);
                }
            }
            free(dataset->matrices[i]);
        }
    }
    free(dataset->matrices);
    free(dataset->samples_per_letter);
    dataset->matrices = NULL;
    dataset->samples_per_letter = NULL;
}

DoubleDataset load_double_dataset(const char* dataset_path) {
    int total_samples = 0;

    DoubleDataset dataset;
    dataset.matrices = calloc(26, sizeof(double***));
    dataset.samples_per_letter = calloc(26, sizeof(int));

    DIR* dir = opendir(dataset_path);
    if (!dir) {
        perror("Error while opening dataset folder");
        return dataset;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;

        char subdir_path[512];
        snprintf(subdir_path, sizeof(subdir_path), "%s/%s", dataset_path, entry->d_name);

        if (is_directory(subdir_path) &&
            strlen(entry->d_name) == 1 &&
            entry->d_name[0] >= 'A' &&
            entry->d_name[0] <= 'Z') {

            int letter_index = entry->d_name[0] - 'A';
            DIR* subdir = opendir(subdir_path);
            if (!subdir) {
                perror("Error while opening subfolder");
                continue;
            }

            int sample_count = 0;
            struct dirent* subentry;
            while ((subentry = readdir(subdir)) != NULL) {
                if (strstr(subentry->d_name, ".png") != NULL ||
                    strstr(subentry->d_name, ".PNG") != NULL) {
                    sample_count++;
                    total_samples++;
                }
            }

            if (sample_count == 0) {
                closedir(subdir);
                continue;  
            }

            dataset.samples_per_letter[letter_index] = sample_count;
            dataset.matrices[letter_index] = calloc(sample_count, sizeof(double*));

            rewinddir(subdir);
            int sample_idx = 0;
            while ((subentry = readdir(subdir)) != NULL) {
                if ((strstr(subentry->d_name, ".png") != NULL ||
                     strstr(subentry->d_name, ".PNG") != NULL) &&
                    sample_idx < sample_count) {

                    char img_path[512];
                    snprintf(img_path, sizeof(img_path), "%s/%s", subdir_path, subentry->d_name);

                    dataset.matrices[letter_index][sample_idx] = img_to_binary_matrix(img_path);
                    if (!dataset.matrices[letter_index][sample_idx]) {
                        fprintf(stderr, "Failed to load image: %s\n", img_path);
                    }
                    sample_idx++;
                }
            }
            closedir(subdir);
        }
    }
    closedir(dir);

    printf("Successfully loaded %i samples as double matrices from target dataset\n", total_samples);
    return dataset;
}

SampleData** double_dataset_to_samples(DoubleDataset* dataset, size_t* out_count) {
    if (!dataset || !out_count) return NULL;

    *out_count = 0;
    for (int i = 0; i < 26; i++) {
        *out_count += dataset->samples_per_letter[i];
    }

    SampleData** samples = malloc(*out_count * sizeof(SampleData*));
    if (!samples) return NULL;

    size_t sample_idx = 0;
    for (int letter = 0; letter < 26; letter++) {
        for (int j = 0; j < dataset->samples_per_letter[letter]; j++) {
            samples[sample_idx] = malloc(sizeof(SampleData));
            if (!samples[sample_idx]) {
                for (size_t k = 0; k < sample_idx; k++) {
                    free(samples[k]->pixels);
                    free(samples[k]);
                }
                free(samples);
                return NULL;
            }

            samples[sample_idx]->expected_value = letter;
            samples[sample_idx]->pixels = malloc(INPUT_SIZE * sizeof(double));
            if (!samples[sample_idx]->pixels) {
                free(samples[sample_idx]);
                for (size_t k = 0; k < sample_idx; k++) {
                    free(samples[k]->pixels);
                    free(samples[k]);
                }
                free(samples);
                return NULL;
            }

            memcpy(samples[sample_idx]->pixels, dataset->matrices[letter][j], INPUT_SIZE * sizeof(double));
            sample_idx++;
        }
    }

    return samples;
}