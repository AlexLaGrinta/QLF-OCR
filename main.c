#include "NewNeuralNetwork/bmp_converter.h"
#include "NewNeuralNetwork/new_neural_network.h"
#include "Preprocessing/preprocessing.h"
#include "RotationPNG/rotate_png.h"
#include "segmentation/segmentation.h"
#include "solver-cli/solver.h"
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

  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    errx(EXIT_FAILURE, "SDL_Init failed: %s", SDL_GetError());
  }
  atexit(SDL_Quit);

  if (IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG) == 0) {
    errx(EXIT_FAILURE, "IMG_Init failed: %s", IMG_GetError());
  }
  atexit(IMG_Quit);

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
  load_network_weights(&network, "NewNeuralNetwork/new_model_weights.bin");

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

  // 3. SOLVER
  printf("\n--- SOLVER ---\n");

  // Find max width
  size_t max_cols = 0;
  for (int r = 0; r < grid_rows; r++) {
    if (result->grid_lines[r].letter_count > (int)max_cols)
      max_cols = result->grid_lines[r].letter_count;
  }

  // Create padded grid for solver
  char **solver_grid = malloc(grid_rows * sizeof(char *));
  for (int r = 0; r < grid_rows; r++) {
    solver_grid[r] = malloc(max_cols + 1);
    memset(solver_grid[r], '*', max_cols); // Padding char
    solver_grid[r][max_cols] = '\0';

    int len = result->grid_lines[r].letter_count;
    memcpy(solver_grid[r], grid[r], len);
  }

  for (int i = 0; i < word_count; i++) {
    char *word = words[i];
    size_t len = strlen(word);
    size_t end_x = 0, end_y = 0;

    int max_mis = (int)len / 4; // Margin of error
    if (len > 3 && max_mis == 0)
      max_mis = 1;

    int gl_found = 0;
    for (size_t r = 0; r < (size_t)grid_rows; r++) {
      for (size_t c = 0; c < max_cols; c++) {
        if (search_word_fuzzy(solver_grid, word, len, c, r, grid_rows, max_cols,
                              &end_x, &end_y, max_mis)) {
          printf("Word '%s' found: (%zu, %zu) -> (%zu, %zu)\n", word, c, r,
                 end_x, end_y);
          gl_found = 1;
          break;
        }
      }
      if (gl_found)
        break;
    }
    if (!gl_found) {
      printf("Word '%s' NOT found\n", word);
    }
  }

  // Free solver grid
  free_grid(solver_grid, grid_rows);

  // Free resources
  free_grid(grid, grid_rows);
  free_grid(words, word_count);
  free_segmentation_result(result);

  return EXIT_SUCCESS;
}