#ifndef IMAGE_SEGMENTATION_H
#define IMAGE_SEGMENTATION_H

#include <stdint.h>
#include <stdbool.h>
#include <SDL2/SDL.h>

// ============================================================================
// STRUCTURES
// ============================================================================

typedef struct {
    uint8_t* data;
    int width;
    int height;
} BinaryImage;

typedef struct {
    int x, y;
    int width, height;
    bool valid;
} GridBounds;

typedef struct {
    int rows;
    int cols;
    float cell_width;
    float cell_height;
    GridBounds box;
} GridZone;

typedef struct {
    char** words;
    int count;
    GridBounds box;
} WordList;

// ============================================================================
// FONCTIONS D'IMAGE BINAIRE
// ============================================================================

BinaryImage* create_binary_image_from_surface(SDL_Surface* surface);
void free_binary_image(BinaryImage* img);

// ============================================================================
// DÉTECTION INTELLIGENTE DE ZONES
// ============================================================================

/**
 * Détecte automatiquement la grille (recherche de structure régulière)
 */
GridBounds detect_grid_smart(BinaryImage* bin);

/**
 * Détecte la liste de mots (tout ce qui n'est pas la grille)
 */
GridBounds detect_wordlist_smart(BinaryImage* bin, GridBounds grid);

/**
 * Détection améliorée avec DBSCAN + analyse de densité
 */
int detect_major_zones_improved(BinaryImage* bin, GridBounds* grid_bounds, GridBounds* list_bounds);

// ============================================================================
// UTILITAIRES
// ============================================================================

void draw_rectangle_color(SDL_Surface* surface, int x, int y, int w, int h, Uint8 r, Uint8 g, Uint8 b);
void save_surface(const char* filename, SDL_Surface* surface);
void free_grid_zone(GridZone* grid);
void free_word_list(WordList* list);

#endif // IMAGE_SEGMENTATION_H

