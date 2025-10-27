#include <stdio.h>
#include <stdlib.h>
#include "segmentation.h"
#include "char_segmentation.h"
#include "image_segmentation.h"

int perform_segmentation(const char *path)
{
    if (SDL_Init(0) != 0){
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    int imgFlags = IMG_INIT_PNG | IMG_INIT_JPG;
    if ((IMG_Init(imgFlags) & imgFlags) == 0){
        fprintf(stderr, "IMG_Init: %s\n", IMG_GetError());
        SDL_Quit();
        return 1;
    }

    // Charger l'image
    SDL_Surface* img = IMG_Load(path);
    if (!img) {
        fprintf(stderr, "IMG_Load(%s): %s\n", path, IMG_GetError());
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // Convertir en BinaryImage pour la segmentation
    BinaryImage* bin = create_binary_image(img);
    if (!bin) {
        fprintf(stderr, "Failed to create BinaryImage\n");
        SDL_FreeSurface(img);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // Segmenter les caractères
    int num_chars = 0;
    Character* chars = find_characters(bin, &num_chars);
    if (!chars) {
        fprintf(stderr, "Failed to find characters\n");
        free_binary_image(bin);
        SDL_FreeSurface(img);
        IMG_Quit();
      SDL_Quit();
        return 1;
    }
    printf("Found %d characters\n", num_chars);


    // Sauvegarder chaque caractère
    for (int i = 0; i < num_chars; i++) {
        char filename[64];
        snprintf(filename, sizeof(filename), "char_%d_%d", chars[i].x, chars[i].y);
        save_character(filename, &chars[i]);

        // Dessiner un rectangle autour du caractère sur l'image originale
        draw_rectangle(img, chars[i].x, chars[i].y, chars[i].width, chars[i].height, 255, 0, 0);
    }

    // Sauvegarder l'image avec les rectangles
    if (SDL_SaveBMP(img, "output_with_rectangles.bmp") != 0) {
        fprintf(stderr, "Failed to save output_with_rectangles.bmp: %s\n", SDL_GetError());
        IMG_Quit();
        SDL_Quit();
    }

    // Libérer la mémoire
    free_characters(chars, num_chars);
    free_binary_image(bin);
    SDL_FreeSurface(img);
    IMG_Quit();
    SDL_Quit();

    return EXIT_SUCCESS;
}

