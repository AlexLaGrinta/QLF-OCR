#ifndef SOLVER_H
#define SOLVER_H

#include <stddef.h>

/**
 * Search for a word in the grid with fuzzy matching.
 *
 * @param grid The 2D grid of characters.
 * @param word The word to search for.
 * @param size_word The length of the word.
 * @param x The starting x position.
 * @param y The starting y position.
 * @param lines The number of lines in the grid.
 * @param columns The number of columns in the grid.
 * @param end_x Pointer to store the ending x coordinate.
 * @param end_y Pointer to store the ending y coordinate.
 * @param max_mismatches Maximum number of allowed mismatched characters.
 * @return 1 if found, 0 otherwise.
 */
int search_word_fuzzy(char **grid, const char *word, size_t size_word, size_t x,
                      size_t y, size_t lines, size_t columns, size_t *end_x,
                      size_t *end_y, int max_mismatches);

/**
 * Searches for the BEST match (minimum mismatches) of the word in the entire
 * grid.
 *
 * @param grid The 2D grid.
 * @param word The word to search for.
 * @param rows Number of rows in grid.
 * @param cols Number of columns in grid.
 * @param max_mismatches Maximum allowed mismatches.
 * @param out_sx Pointer to store start x.
 * @param out_sy Pointer to store start y.
 * @param out_ex Pointer to store end x.
 * @param out_ey Pointer to store end y.
 * @return The number of mismatches of the best found match, or -1 if no match
 * found within max_mismatches.
 */
int find_best_match(char **grid, const char *word, size_t rows, size_t cols,
                    int max_mismatches, size_t *out_sx, size_t *out_sy,
                    size_t *out_ex, size_t *out_ey);

#endif
