#ifndef SEGMENTATION_H
#define SEGMENTATION_H

#include <stdlib.h>

typedef struct {
  char path[512];
  int x, y, width, height;
} LetterPath;

typedef struct {
  LetterPath *letters;
  int letter_count;
} Word2;

typedef struct {
  LetterPath *letters;
  int letter_count;
} GridLine;

typedef struct {
  GridLine *grid_lines;
  int grid_line_count;
  Word2 *words;
  int word_count;
} SegmentationResult;

SegmentationResult *perform_segmentation(const char *path);
void free_segmentation_result(SegmentationResult *result);
void print_segmentation_result(SegmentationResult *result);

#endif
