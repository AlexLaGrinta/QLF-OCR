#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <err.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
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


void print_matrix_double(double* matrix)
{
    for (int y = 0; y < 28; y++)
    {
        for (int x = 0; x < 28; x++)
        {
            size_t i = (size_t)y * (size_t)28 + (size_t)x;
            printf("%f ", matrix[i]);
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
        fprintf(stderr, "Loading failed: %s\n", is_bmp_ext(path) ? SDL_GetError() : IMG_GetError());
        return out;
    }
    SDL_Surface *rgba = SDL_ConvertSurfaceFormat(src, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(src);
    if (!rgba) {
        fprintf(stderr, "Convert failed: %s\n", SDL_GetError());
        return out;
    }

    out.w = rgba->w;
    out.h = rgba->h;
    out.data = (uint8_t*)malloc((size_t)out.w * (size_t)out.h);
    if (!out.data) {
        fprintf(stderr, "Allocation failed\n");
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


// Vérifie si un chemin est un dossier
int is_directory(const char* path) {
    struct stat statbuf;
    return (stat(path, &statbuf) == 0) && S_ISDIR(statbuf.st_mode);
}

// Charge le dataset complet
LetterDataset load_dataset(const char* dataset_path) {
    LetterDataset dataset;
    dataset.matrices = calloc(26, sizeof(BinaryMatrix**));
    dataset.samples_per_letter = calloc(26, sizeof(int));

    DIR* dir = opendir(dataset_path);
    if (!dir) {
        perror("Error while opening dataset folder");
        return dataset;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        // Ignorer les fichiers cachés et les dossiers spéciaux (. et ..)
        if (entry->d_name[0] == '.') continue;

        // Construire le chemin complet
        char subdir_path[512];
        snprintf(subdir_path, sizeof(subdir_path), "%s/%s", dataset_path, entry->d_name);

        // Vérifier que c'est un dossier et que le nom est une lettre majuscule (A-Z)
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

            // Compter les fichiers PNG dans ce sous-dossier
            int sample_count = 0;
            struct dirent* subentry;
            while ((subentry = readdir(subdir)) != NULL) {
                if (strstr(subentry->d_name, ".png") != NULL ||
                    strstr(subentry->d_name, ".PNG") != NULL) {
                    sample_count++;
                }
            }

            if (sample_count == 0) {
                closedir(subdir);
                continue;  // Pas d'images PNG dans ce dossier
            }

            dataset.samples_per_letter[letter_index] = sample_count;
            dataset.matrices[letter_index] = calloc(sample_count, sizeof(BinaryMatrix*));

            // Re-parcourir pour charger les images
            rewinddir(subdir);
            int sample_idx = 0;
            while ((subentry = readdir(subdir)) != NULL) {
                if ((strstr(subentry->d_name, ".png") != NULL ||
                     strstr(subentry->d_name, ".PNG") != NULL) &&
                    sample_idx < sample_count) {

                    char img_path[512];
                    snprintf(img_path, sizeof(img_path), "%s/%s", subdir_path, subentry->d_name);

                    // Charger et convertir l'image
                    dataset.matrices[letter_index][sample_idx] =
                        malloc(sizeof(BinaryMatrix));
                    *dataset.matrices[letter_index][sample_idx] =
                        img_to_binary_matrix_strict10(img_path);
                    sample_idx++;
                }
            }
            closedir(subdir);
        }
    }
    closedir(dir);
    return dataset;
}

// Libère toute la mémoire allouée
void free_dataset(LetterDataset* dataset) {
    for (int i = 0; i < 26; i++) {
        if (dataset->matrices[i]) {
            for (int j = 0; j < dataset->samples_per_letter[i]; j++) {
                if (dataset->matrices[i][j]) {
                    free(dataset->matrices[i][j]->data);
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