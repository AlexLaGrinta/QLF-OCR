#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "image_segmentation.h"
#include "char_segmentation.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <image.bmp>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char* image_path = argv[1];

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }

    if (IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG) == 0) {
        fprintf(stderr, "IMG_Init Error: %s\n", SDL_GetError());
        SDL_Quit();
        return EXIT_FAILURE;
    }

    printf("\n");
    printf("═══════════════════════════════════════════════════════\n");
    printf("  WORD SEARCH - INTELLIGENT SEGMENTATION v2.0\n");
    printf("═══════════════════════════════════════════════════════\n\n");
    
    printf("📂 Loading: %s\n", image_path);
    
    SDL_Surface* img = IMG_Load(image_path);
    if (!img) {
        fprintf(stderr, "Failed to load: %s\n", IMG_GetError());
        IMG_Quit();
        SDL_Quit();
        return EXIT_FAILURE;
    }

    printf("   ✓ Image: %dx%d pixels\n\n", img->w, img->h);

    SDL_Surface* rgba_img = SDL_ConvertSurfaceFormat(img, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(img);
    
    if (!rgba_img) {
        fprintf(stderr, "Format conversion failed\n");
        IMG_Quit();
        SDL_Quit();
        return EXIT_FAILURE;
    }

    printf("═══════════════════════════════════════════════════════\n");
    printf("  STEP 1: BINARIZATION\n");
    printf("═══════════════════════════════════════════════════════\n\n");
    
    BinaryImage* bin = create_binary_image_from_surface(rgba_img);
    
    if (!bin) {
        fprintf(stderr, "✗ Binarization failed\n");
        SDL_FreeSurface(rgba_img);
        IMG_Quit();
        SDL_Quit();
        return EXIT_FAILURE;
    }

    printf("   ✓ Binary image: %dx%d\n\n", bin->width, bin->height);

    printf("═══════════════════════════════════════════════════════\n");
    printf("  STEP 2: INTELLIGENT ZONE DETECTION\n");
    printf("═══════════════════════════════════════════════════════\n\n");
    
    GridBounds grid_bounds, list_bounds;
    list_bounds.valid = false;
    
    int zones_found = detect_major_zones_improved(bin, &grid_bounds, &list_bounds);

    if (!zones_found) {
        fprintf(stderr, "\n✗ Zone detection failed\n");
        free_binary_image(bin);
        SDL_FreeSurface(rgba_img);
        IMG_Quit();
        SDL_Quit();
        return EXIT_FAILURE;
    }

    printf("\n");
    printf("═══════════════════════════════════════════════════════\n");
    printf("  STEP 3: CHARACTER EXTRACTION\n");
    printf("═══════════════════════════════════════════════════════\n\n");
    
    int num_chars = 0;
    Character* chars = find_characters_in_grid(bin, &num_chars, 0, 0);
    
    if (!chars) {
        fprintf(stderr, "✗ Character extraction failed\n");
        free_binary_image(bin);
        SDL_FreeSurface(rgba_img);
        IMG_Quit();
        SDL_Quit();
        return EXIT_FAILURE;
    }

    printf("\n");
    printf("═══════════════════════════════════════════════════════\n");
    printf("  STEP 4: ZONE ASSIGNMENT\n");
    printf("═══════════════════════════════════════════════════════\n\n");
    
    assign_character_zones(chars, num_chars, grid_bounds, list_bounds);
    
    printf("\n");
    printf("═══════════════════════════════════════════════════════\n");
    printf("  STEP 5: FILTERING\n");
    printf("═══════════════════════════════════════════════════════\n\n");
    
   // chars = filter_letters(chars, &num_chars);
    printf("\n   ✓ Final: %d characters\n\n", num_chars);

    // Créer les dossiers
    #ifdef _WIN32
        mkdir("output");
        mkdir("output\\grid_letters");
        mkdir("output\\wordlist_letters");
    #else
        mkdir("output", 0755);
        mkdir("output/grid_letters", 0755);
        mkdir("output/wordlist_letters", 0755);
    #endif

    printf("═══════════════════════════════════════════════════════\n");
    printf("  STEP 6: SAVING CHARACTERS\n");
    printf("═══════════════════════════════════════════════════════\n\n");
    
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
    
    printf("  💾 Grid letters: %d → output/grid_letters/\n", grid_count);
    printf("  💾 Word list letters: %d → output/wordlist_letters/\n\n", list_count);

    printf("═══════════════════════════════════════════════════════\n");
    printf("  STEP 7: VISUALIZATION\n");
    printf("═══════════════════════════════════════════════════════\n\n");
    
    printf("  🎨 Creating visualization...\n");
    SDL_Surface* output = SDL_ConvertSurfaceFormat(rgba_img, SDL_PIXELFORMAT_RGBA32, 0);
    if (output) {
        // Cadre bleu épais pour la grille
        draw_rectangle_color(output, grid_bounds.x, grid_bounds.y, 
                           grid_bounds.width, grid_bounds.height, 
                           0, 100, 255);
        
        // Cadre vert pour la liste
        if (list_bounds.valid) {
            draw_rectangle_color(output, list_bounds.x, list_bounds.y, 
                               list_bounds.width, list_bounds.height, 
                               0, 255, 100);
        }
        
        // Lettres individuelles
        for (int i = 0; i < num_chars; i++) {
            if (chars[i].zone == ZONE_GRID) {
                draw_rectangle_color(output, chars[i].x, chars[i].y, 
                                   chars[i].width, chars[i].height, 
                                   255, 0, 0);
            } else if (chars[i].zone == ZONE_WORDLIST) {
                draw_rectangle_color(output, chars[i].x, chars[i].y, 
                                   chars[i].width, chars[i].height, 
                                   255, 255, 0);
            }
        }
        
        save_surface("output/visualization.bmp", output);
        SDL_FreeSurface(output);
        
        printf("     ✓ Saved: output/visualization.bmp\n");
        printf("     • 🔵 Blue   = Grid zone\n");
        printf("     • 🟢 Green  = Word list zone\n");
        printf("     • 🔴 Red    = Grid letters\n");
        printf("     • 🟡 Yellow = Word list letters\n\n");
    }

    // Nettoyage
    free_characters(chars, num_chars);
    free_binary_image(bin);
    SDL_FreeSurface(rgba_img);
    IMG_Quit();
    SDL_Quit();

    printf("═══════════════════════════════════════════════════════\n");
    printf("  ✅ SUMMARY\n");
    printf("═══════════════════════════════════════════════════════\n\n");
    printf("  📊 Grid: (%d,%d) %dx%d\n", 
           grid_bounds.x, grid_bounds.y, grid_bounds.width, grid_bounds.height);
    if (list_bounds.valid) {
        printf("  📊 List: (%d,%d) %dx%d\n", 
               list_bounds.x, list_bounds.y, list_bounds.width, list_bounds.height);
    }
    printf("  🔤 Grid letters: %d\n", grid_count);
    printf("  📝 List letters: %d\n", list_count);
    printf("  📁 Output: output/grid_letters/ + output/wordlist_letters/\n");
    printf("\n═══════════════════════════════════════════════════════\n");
    printf("  ✅ COMPLETE!\n");
    printf("═══════════════════════════════════════════════════════\n\n");

    return EXIT_SUCCESS;
}

