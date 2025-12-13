#include "solver.h"
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char **get_matrix_of_file(const char *path, size_t *num_lines,
                          size_t *num_columns) {
  FILE *fptr;

  fptr = fopen(path, "r");

  if (fptr == NULL) {
    errx(EXIT_FAILURE, "file not found.");
  }

  char line[100];

  size_t number_of_lines = 1;
  size_t number_of_columns = 0;

  fgets(line, 100, fptr);

  while (line[number_of_columns] != '\0' && line[number_of_columns] != '\n' &&
         line[number_of_columns] != ' ') {
    number_of_columns++;
  }

  while (fgets(line, 100, fptr)) {
    number_of_lines++;
  }

  rewind(fptr);

  char **grid = malloc(sizeof(char *) * number_of_lines);

  for (size_t i = 0; i < number_of_lines; i++) {
    grid[i] = malloc(number_of_columns + 1);
  }

  size_t ind = 0;
  while (fgets(line, 100, fptr)) {
    memcpy(grid[ind++], line, number_of_columns);
    grid[ind - 1][number_of_columns] = '\0';
  }

  fclose(fptr);

  *num_lines = number_of_lines;
  *num_columns = number_of_columns;

  return grid;
}

void free_grid_solver(char **grid, size_t lines) {
  for (size_t i = 0; i < lines; i++) {
    free(grid[i]);
  }
  free(grid);
}

static int check_direction(char **grid, const char *word, size_t size_word,
                           size_t x, size_t y, int dx, int dy,
                           int max_mismatches, size_t *end_x, size_t *end_y) {
  int mismatches = 0;
  size_t curr_x = x;
  size_t curr_y = y;

  for (size_t i = 0; i < size_word; i++) {
    if (grid[curr_y][curr_x] != word[i]) {
      mismatches++;
      if (mismatches > max_mismatches) {
        return -1;
      }
    }

    if (i < size_word - 1) { 
      curr_x += dx;
      curr_y += dy;
    }
  }

  *end_x = curr_x;
  *end_y = curr_y;
  return mismatches;
}

int search_word_fuzzy(char **grid, const char *word, size_t size_word, size_t x,
                      size_t y, size_t lines, size_t columns, size_t *end_x,
                      size_t *end_y, int max_mismatches) {

  int directions[8][2] = {{1, 0}, {-1, 0}, {0, 1},   {0, -1},
                          {1, 1}, {1, -1}, {-1, -1}, {-1, 1}};

  for (int d = 0; d < 8; d++) {
    int dx = directions[d][0];
    int dy = directions[d][1];

    long long check_end_x = (long long)x + (size_word - 1) * dx;
    long long check_end_y = (long long)y + (size_word - 1) * dy;

    if (check_end_x >= 0 && check_end_x < (long long)columns &&
        check_end_y >= 0 && check_end_y < (long long)lines) {

      if (check_direction(grid, word, size_word, x, y, dx, dy, max_mismatches,
                          end_x, end_y) != -1) {
        return 1;
      }
    }
  }

  return 0;
}

int find_best_match(char **grid, const char *word, size_t rows, size_t cols,
                    int max_mismatches, size_t *out_sx, size_t *out_sy,
                    size_t *out_ex, size_t *out_ey) {
  int best_mismatches = max_mismatches + 1;
  int directions[8][2] = {{1, 0}, {-1, 0}, {0, 1},   {0, -1},
                          {1, 1}, {1, -1}, {-1, -1}, {-1, 1}};

  size_t size_word = strlen(word);

  for (size_t y = 0; y < rows; y++) {
    for (size_t x = 0; x < cols; x++) {

      for (int d = 0; d < 8; d++) {
        int dx = directions[d][0];
        int dy = directions[d][1];

        long long check_end_x = (long long)x + (size_word - 1) * dx;
        long long check_end_y = (long long)y + (size_word - 1) * dy;

        if (check_end_x >= 0 && check_end_x < (long long)cols &&
            check_end_y >= 0 && check_end_y < (long long)rows) {

          size_t end_x, end_y;
          int mis = check_direction(grid, word, size_word, x, y, dx, dy,
                                    max_mismatches, &end_x, &end_y);

          if (mis != -1 && mis < best_mismatches) {
            best_mismatches = mis;
            *out_sx = x;
            *out_sy = y;
            *out_ex = end_x;
            *out_ey = end_y;

            if (best_mismatches == 0)
              return 0;
          }
        }
      }
    }
  }

  if (best_mismatches <= max_mismatches) {
    return best_mismatches;
  }
  return -1;
}

#ifndef NO_MAIN_SOLVER
int main(int argc, char *argv[]) {
  if (argc < 3) {
    errx(EXIT_FAILURE, "Usage: ./solver <grid_file> <word> [max_mismatches]");
  }

  const char *path = argv[1];
  const char *word = argv[2];
  int max_mis = 0;

  if (argc >= 4) {
    max_mis = atoi(argv[3]);
  }

  size_t number_of_lines;
  size_t number_of_columns;

  char **grid = get_matrix_of_file(path, &number_of_lines, &number_of_columns);

  size_t size_word = strlen(word);

  int res = 0;

  for (size_t y = 0; y < number_of_lines; y++) {
    for (size_t x = 0; x < number_of_columns; x++) {

      size_t end_x;
      size_t end_y;
      res = search_word_fuzzy(grid, word, size_word, x, y, number_of_lines,
                              number_of_columns, &end_x, &end_y, max_mis);
      if (res) {
        printf("(%li,%li)(%li,%li)\n", x, y, end_x, end_y);
        break;
      }
    }
    if (res)
      break;
  }

  if (!res)
    printf("Not Found\n");

  free_grid_solver(grid, number_of_lines);

  return EXIT_SUCCESS;
}
#endif