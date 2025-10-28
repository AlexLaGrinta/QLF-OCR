#ifndef CHAR_SEGMENTATION_H
#define CHAR_SEGMENTATION_H

#include "image_segmentation.h"  // Pour BinaryImage et Character

// Prototypes des fonctions
Character* find_characters(const BinaryImage* bin, int* num_chars);
void save_character(const char* filename, const Character* ch);
void free_character(Character* ch);
void free_characters(Character* chars, int num_chars);

// Filtre les composantes pour ne garder que les lettres probables
Character* filter_letters(Character* chars, int* num_chars);

#endif // CHAR_SEGMENTATION_H

