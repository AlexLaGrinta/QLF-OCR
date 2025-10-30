#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "image_segmentation.h"
#include "char_segmentation.h"

int perform_segmentation(const char *path) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    if (IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG) == 0) {
        fprintf(stderr, "IMG_Init Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Surface* img = IMG_Load(path);
    if (!img) {
        fprintf(stderr, "Failed to load: %s\n", IMG_GetError());
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // ========================================================================
    // STEP 1: CONVERSION EN IMAGE BINAIRE
    // ========================================================================

    BinaryImage* bin = create_binary_image_from_surface(img);
    if (!bin) {
        fprintf(stderr, "✗ Binary conversion failed\n");
        SDL_FreeSurface(img);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // ========================================================================
    // STEP 2: DÉTECTION DES ZONES (GRILLE + LISTE)
    // ========================================================================

    GridBounds grid_bounds = {0};
    GridBounds list_bounds = {0};

    int zones_detected = detect_major_zones_improved(bin, &grid_bounds, &list_bounds);

    if (!grid_bounds.valid) {
        fprintf(stderr, "✗ Grid detection failed\n");
        free_binary_image(bin);
        SDL_FreeSurface(img);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // ========================================================================
    // STEP 3: EXTRACTION DES CARACTÈRES
    // ========================================================================

    int num_chars = 0;
    Character* chars = find_characters_in_grid(bin, &num_chars, 0, 0);

    if (!chars) {
        fprintf(stderr, "✗ Character extraction failed\n");
        free_binary_image(bin);
        SDL_FreeSurface(img);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // ========================================================================
    // STEP 4: ASSIGNATION DES ZONES
    // ========================================================================

    assign_character_zones(chars, num_chars, grid_bounds, list_bounds);

    // ========================================================================
    // STEP 5: FILTRAGE (optionnel)
    // ========================================================================

    //chars = filter_letters(chars, &num_chars);

    // ========================================================================
    // STEP 6: CRÉATION DES DOSSIERS DE SORTIE
    // ========================================================================

    #ifdef _WIN32
        _mkdir("output");
        _mkdir("output\\grid_letters");
        _mkdir("output\\wordlist_letters");
    #else
        mkdir("output", 0755);
        mkdir("output/grid_letters", 0755);
        mkdir("output/wordlist_letters", 0755);
    #endif

    // ========================================================================
    // STEP 7: SAUVEGARDE DES CARACTÈRES
    // ========================================================================

    int grid_count = 0;
    int list_count = 0;

    for (int i = 0; i < num_chars; i++) {
        char filename[512];

        if (chars[i].zone == ZONE_GRID) {
            snprintf(filename, sizeof(filename), "output/grid_letters/letter_%04d.bmp", grid_count);
            save_character_bmp(&chars[i], filename, 28);
            grid_count++;
        } else if (chars[i].zone == ZONE_WORDLIST) {
            snprintf(filename, sizeof(filename), "output/wordlist_letters/letter_%04d.bmp", list_count);
            save_character_bmp(&chars[i], filename, 28);
            list_count++;
        }
    }

    // ========================================================================
    // STEP 8: VISUALISATION
    // ========================================================================

    SDL_Surface* rgba_img = SDL_ConvertSurfaceFormat(img, SDL_PIXELFORMAT_RGBA32, 0);
    if (rgba_img) {

        draw_rectangle_color(rgba_img, 
                           grid_bounds.x, grid_bounds.y, 
                           grid_bounds.width, grid_bounds.height,
                           0, 0, 255);

        if (list_bounds.valid) {
            draw_rectangle_color(rgba_img,
                               list_bounds.x, list_bounds.y,
                               list_bounds.width, list_bounds.height,
                               0, 255, 0);
        }

        SDL_LockSurface(rgba_img);

        for (int i = 0; i < num_chars; i++) {
            Uint8 r = (chars[i].zone == ZONE_GRID) ? 255 : 255;
            Uint8 g = (chars[i].zone == ZONE_GRID) ? 0 : 255;
            Uint8 b = 0;

            int x = chars[i].x;
            int y = chars[i].y;
            int w = chars[i].width;
            int h = chars[i].height;

            for (int px = x; px < x + w && px < rgba_img->w; px++) {
                if (y >= 0 && y < rgba_img->h) {
                    Uint32* pixel = (Uint32*)((Uint8*)rgba_img->pixels + y * rgba_img->pitch + px * 4);
                    *pixel = SDL_MapRGB(rgba_img->format, r, g, b);
                }
            }

            for (int px = x; px < x + w && px < rgba_img->w; px++) {
                int py = y + h - 1;
                if (py >= 0 && py < rgba_img->h) {
                    Uint32* pixel = (Uint32*)((Uint8*)rgba_img->pixels + py * rgba_img->pitch + px * 4);
                    *pixel = SDL_MapRGB(rgba_img->format, r, g, b);
                }
            }

            for (int py = y; py < y + h && py < rgba_img->h; py++) {
                if (x >= 0 && x < rgba_img->w) {
                    Uint32* pixel = (Uint32*)((Uint8*)rgba_img->pixels + py * rgba_img->pitch + x * 4);
                    *pixel = SDL_MapRGB(rgba_img->format, r, g, b);
                }
            }

            for (int py = y; py < y + h && py < rgba_img->h; py++) {
                int px_right = x + w - 1;
                if (px_right >= 0 && px_right < rgba_img->w) {
                    Uint32* pixel = (Uint32*)((Uint8*)rgba_img->pixels + py * rgba_img->pitch + px_right * 4);
                    *pixel = SDL_MapRGB(rgba_img->format, r, g, b);
                }
            }
        }

        SDL_UnlockSurface(rgba_img);

        save_surface("output/visualization.bmp", rgba_img);
        SDL_FreeSurface(rgba_img);
    }

    // ========================================================================
    // NETTOYAGE
    // ========================================================================

    free_characters(chars, num_chars);
    free_binary_image(bin);
    SDL_FreeSurface(img);

    IMG_Quit();
    SDL_Quit();

    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <image.bmp>\n", argv[0]);
        return EXIT_FAILURE;
    }

    return perform_segmentation(argv[1]);
}

