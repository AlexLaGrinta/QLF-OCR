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

#endif
