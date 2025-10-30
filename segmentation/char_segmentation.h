#ifndef CHAR_SEGMENTATION_H
#define CHAR_SEGMENTATION_H

#include "image_segmentation.h"
#include <SDL2/SDL.h>
#include <stdbool.h>

typedef enum {
    ZONE_GRID,
    ZONE_WORDLIST,
    ZONE_UNKNOWN
} CharacterZone;

typedef struct {
    uint8_t* data;
    int width;
    int height;
    int x;
    int y;
    CharacterZone zone;
} Character;

typedef struct {
    Character* chars;
    int count;
} CharacterList;

/**
 * Trouve tous les caractères dans une image binaire avec leur zone
 */
Character* find_characters_in_grid(BinaryImage* bin, int* num_chars, int offset_x, int offset_y);

/**
 * Assigne les zones (grille ou liste) aux caractères
 */
void assign_character_zones(Character* chars, int num_chars, 
                            GridBounds grid_bounds, GridBounds list_bounds);

/**
 * Filtre les caractères détectés (retire le bruit)
 */
Character* filter_letters(Character* chars, int* num_chars);

/**
 * Sauvegarde un caractère en BMP (format 28x28)
 */
void save_character_bmp(Character* ch, const char* filename, int target_size);

/**
 * Libère un tableau de caractères
 */
void free_characters(Character* chars, int num_chars);

#endif // CHAR_SEGMENTATION_H

