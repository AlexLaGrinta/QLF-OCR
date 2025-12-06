#ifndef SEGMENTATION_H
#define SEGMENTATION_H

#include <stdlib.h> // For size_t, dynamic allocation types if needed (though not strictly for the structs below if just using int/char)

// Structure pour stocker un chemin de lettre
typedef struct {
  char path[512];
} LetterPath;

// Structure pour stocker un mot (liste de chemins de lettres)
typedef struct {
  LetterPath *letters;
  int letter_count;
} Word2;

// Structure pour stocker une ligne de la grille
typedef struct {
  LetterPath *letters;
  int letter_count;
} GridLine;

// Structure pour stocker tous les résultats de segmentation
typedef struct {
  GridLine *grid_lines;
  int grid_line_count;
  Word2 *words;
  int word_count;
} SegmentationResult;

// Function prototypes
SegmentationResult *perform_segmentation(const char *path);
void free_segmentation_result(SegmentationResult *result);
void print_segmentation_result(SegmentationResult *result);

#endif // SEGMENTATION_H
