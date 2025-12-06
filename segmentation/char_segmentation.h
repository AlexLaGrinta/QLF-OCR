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

typedef struct {
    int word_id;
    Character* letters;
    int num_letters;
    int x, y, width, height;
} Word;

// Nouvelle fonction
Word* group_characters_into_words(Character* chars, int num_chars, int* num_words);
void free_words(Word* words, int num_words);


Character* find_characters_in_grid(BinaryImage* bin, int* num_chars, int offset_x, int offset_y);

void assign_character_zones(Character* chars, int num_chars, 
                            GridBounds grid_bounds, GridBounds list_bounds);

Character* filter_letters(Character* chars, int* num_chars);

void save_character_bmp(Character* ch, const char* filename, int target_size);

void free_characters(Character* chars, int num_chars);

#endif

