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

// Helper to draw a line on SDL_Surface
void draw_line(SDL_Surface *surface, int x1, int y1, int x2, int y2,
               Uint32 color) {
  int dx = abs(x2 - x1);
  int dy = abs(y2 - y1);
  int sx = (x1 < x2) ? 1 : -1;
  int sy = (y1 < y2) ? 1 : -1;
  int err = dx - dy;

  while (1) {
    if (x1 >= 0 && x1 < surface->w && y1 >= 0 && y1 < surface->h) {
      // Draw a 3x3 point for thickness
      for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
          int px = x1 + i;
          int py = y1 + j;
          if (px >= 0 && px < surface->w && py >= 0 && py < surface->h) {
            Uint32 *pixels = (Uint32 *)((Uint8 *)surface->pixels +
                                        py * surface->pitch + px * 4);
            *pixels = color;
          }
        }
      }
    }

    if (x1 == x2 && y1 == y2)
      break;
    int e2 = 2 * err;
    if (e2 > -dy) {
      err -= dy;
      x1 += sx;
    }
    if (e2 < dx) {
      err += dx;
      y1 += sy;
    }
  }
}

// Helper to draw a rectangle outline on SDL_Surface
void draw_rect(SDL_Surface *surface, int x, int y, int w, int h, Uint32 color) {
  // Top and Bottom
  draw_line(surface, x, y, x + w, y, color);
  draw_line(surface, x, y + h, x + w, y + h, color);
  // Left and Right
  draw_line(surface, x, y, x, y + h, color);
  draw_line(surface, x + w, y, x + w, y + h, color);
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    errx(EXIT_FAILURE, "Utilisation : ./ocr {image}");
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

  // Load visualization image
  SDL_Surface *vis_img = IMG_Load("temp/rotated.bmp");
  if (!vis_img) {
    warnx("Failed to load output/visualization.bmp to draw results");
  } else {
    // Must convert to RGBA to ensure format matches drawings (or just MapRGB)
    // Assuming it was saved as correct format or we use MapRGB.
    // Since we just loaded it, let's use MapRGB.

    // Color palette for different words
    Uint32 colors[] = {
        SDL_MapRGB(vis_img->format, 255, 0, 0),   // Red
        SDL_MapRGB(vis_img->format, 0, 255, 0),   // Green
        SDL_MapRGB(vis_img->format, 0, 0, 255),   // Blue
        SDL_MapRGB(vis_img->format, 255, 255, 0), // Yellow
        SDL_MapRGB(vis_img->format, 255, 0, 255), // Magenta
        SDL_MapRGB(vis_img->format, 0, 255, 255), // Cyan
        SDL_MapRGB(vis_img->format, 255, 128, 0), // Orange
        SDL_MapRGB(vis_img->format, 128, 0, 255), // Purple
    };
    int num_colors = 8;

    for (int i = 0; i < word_count; i++) {
      char *word = words[i];
      size_t len = strlen(word);
      size_t start_c = 0, start_r = 0;
      size_t end_c = 0, end_r = 0;

      int max_mis = (int)len / 4;
      if (len > 3 && max_mis == 0)
        max_mis = 1;

      Uint32 color = colors[i % num_colors];

      int mismatches =
          find_best_match(solver_grid, word, grid_rows, max_cols, max_mis,
                          &start_c, &start_r, &end_c, &end_r);

      if (mismatches != -1) {
        printf("Word '%s' found (mis=%d): (%zu, %zu) -> (%zu, %zu)\n", word,
               mismatches, start_c, start_r, end_c, end_r);

        // Draw oriented box around the word
        if (start_r < (size_t)result->grid_line_count &&
            start_c < (size_t)result->grid_lines[start_r].letter_count &&
            end_r < (size_t)result->grid_line_count &&
            end_c < (size_t)result->grid_lines[end_r].letter_count) {
          LetterPath start_lp = result->grid_lines[start_r].letters[start_c];
          LetterPath end_lp = result->grid_lines[end_r].letters[end_c];

          // Corners
          int sx = start_lp.x;
          int sy = start_lp.y;
          int sw = start_lp.width;
          int sh = start_lp.height;
          int ex = end_lp.x;
          int ey = end_lp.y;
          int ew = end_lp.width;
          int eh = end_lp.height;

          // Determine direction to pick correct hull edges
          int dx = 0;
          if (end_c > start_c)
            dx = 1;
          else if (end_c < start_c)
            dx = -1;
          int dy = 0;
          if (end_r > start_r)
            dy = 1;
          else if (end_r < start_r)
            dy = -1;

          int x1_a, y1_a, x2_a, y2_a; // Side 1
          int x1_b, y1_b, x2_b, y2_b; // Side 2

          if (dy == 0) {   // Horizontal
            if (dx == 1) { // L -> R
              x1_a = sx;
              y1_a = sy;
              x2_a = ex + ew;
              y2_a = ey; // Top
              x1_b = sx;
              y1_b = sy + sh;
              x2_b = ex + ew;
              y2_b = ey + eh; // Bottom
            } else {          // R -> L
              x1_a = sx + sw;
              y1_a = sy;
              x2_a = ex;
              y2_a = ey; // Top
              x1_b = sx + sw;
              y1_b = sy + sh;
              x2_b = ex;
              y2_b = ey + eh; // Bottom
            }
          } else if (dx == 0) { // Vertical
            if (dy == 1) {      // T -> B
              x1_a = sx;
              y1_a = sy;
              x2_a = ex;
              y2_a = ey + eh; // Left
              x1_b = sx + sw;
              y1_b = sy;
              x2_b = ex + ew;
              y2_b = ey + eh; // Right
            } else {          // B -> T
              x1_a = sx;
              y1_a = sy + sh;
              x2_a = ex;
              y2_a = ey; // Left
              x1_b = sx + sw;
              y1_b = sy + sh;
              x2_b = ex + ew;
              y2_b = ey; // Right
            }
          } else { // Diagonal
            // Main Diagonal-ish slope (TL-BR or BR-TL)
            if (dx * dy > 0) {
              x1_a = sx + sw;
              y1_a = sy;
              x2_a = ex + ew;
              y2_a = ey; // TR -> TR
              x1_b = sx;
              y1_b = sy + sh;
              x2_b = ex;
              y2_b = ey + eh; // BL -> BL
            }
            // Anti Diagonal-ish slope (TR-BL or BL-TR)
            else {
              x1_a = sx;
              y1_a = sy;
              x2_a = ex;
              y2_a = ey; // TL -> TL
              x1_b = sx + sw;
              y1_b = sy + sh;
              x2_b = ex + ew;
              y2_b = ey + eh; // BR -> BR
            }
          }

          // Draw the 4 sides
          draw_line(vis_img, x1_a, y1_a, x2_a, y2_a, color); // Side 1
          draw_line(vis_img, x1_b, y1_b, x2_b, y2_b, color); // Side 2
          draw_line(vis_img, x1_a, y1_a, x1_b, y1_b, color); // Cap 1 (Start)
          draw_line(vis_img, x2_a, y2_a, x2_b, y2_b, color); // Cap 2 (End)
        }

      } else {
        printf("Word '%s' NOT found\n", word);
      }
    }

    // Save the result
    if (SDL_SaveBMP(vis_img, "output/solved.bmp") != 0) {
      warnx("Failed to save output/solved.bmp: %s", SDL_GetError());
    } else {
      printf("Saved solved puzzle to output/solved.bmp\n");
    }

    SDL_FreeSurface(vis_img);
  }

  // Free solver grid
  free_grid(solver_grid, grid_rows);

  // Free resources
  free_grid(grid, grid_rows);
  free_grid(words, word_count);
  free_segmentation_result(result);

  return EXIT_SUCCESS;
}