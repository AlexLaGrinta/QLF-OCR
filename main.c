#include "NeuralNetwork/bmp_converter.h"
#include "NeuralNetwork/neural_network.h"
#include "Preprocessing/preprocessing.h"
#include "RotationPNG/rotate_png.h"
#include "segmentation/segmentation.h"
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void free_grid(char **grid, size_t size) {
  for (size_t i = 0; i < size; i++) {
    free(grid[i]);
  }
  free(grid);
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    errx(EXIT_FAILURE, "Utilisation : ./selem {image}");
  }

  const char *inPath = argv[1];

  if (automatic_rotate(inPath, "temp/rotated.bmp") != 0)
    return 1;

  if (preprocess_image("temp/rotated.bmp", "rotated_proprocessed.bmp") != 0)
    return 1;

  SegmentationResult *result =
      perform_segmentation("temp/rotated_proprocessed.bmp");

  if (!result) {
    errx(EXIT_FAILURE, "Segmentation failed");
  }

  NeuralNetwork network;
  load_network_weights(&network, "temp/oat.bin");

  // 1. GRID PREDICTION
  printf("\n--- GRID PREDICTION ---\n");
  int grid_rows = result->grid_line_count;
  char **grid = malloc(sizeof(char *) * grid_rows);

  for (int i = 0; i < grid_rows; i++) {
    int row_len = result->grid_lines[i].letter_count;
    grid[i] = malloc(
        sizeof(char) *
        (row_len + 1)); // +1 for null terminator if we want to print as string

    for (int j = 0; j < row_len; j++) {
      char *path = result->grid_lines[i].letters[j].path;
      double *inputs = img_to_binary_matrix(path);
      if (inputs) {
        int pred = predict(&network, inputs);
        grid[i][j] = (char)(pred + 'A'); // Assume A-Z mapping
        free(inputs);
      } else {
        grid[i][j] = '?'; // Error placeholder
      }
    }
    grid[i][row_len] = '\0';
    printf("Row %d: %s\n", i, grid[i]);
  }

  // 2. WORDS PREDICTION
  printf("\n--- WORDS PREDICTION ---\n");
  int word_count = result->word_count;
  char **words = malloc(sizeof(char *) * word_count);

  for (int i = 0; i < word_count; i++) {
    int word_len = result->words[i].letter_count;
    words[i] = malloc(sizeof(char) * (word_len + 1));

    for (int j = 0; j < word_len; j++) {
      char *path = result->words[i].letters[j].path;
      double *inputs = img_to_binary_matrix(path);
      if (inputs) {
        int pred = predict(&network, inputs);
        words[i][j] = (char)(pred + 'A');
        free(inputs);
      } else {
        words[i][j] = '?';
      }
    }
    words[i][word_len] = '\0';
    printf("Word %d: %s\n", i, words[i]);
  }

  // Free resources
  free_grid(grid, grid_rows);
  free_grid(words, word_count);
  free_segmentation_result(result);

  return EXIT_SUCCESS;
}