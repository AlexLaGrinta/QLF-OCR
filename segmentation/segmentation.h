#ifndef SEGMENTATION_H
#define SEGMENTATION_H

#include <stdint.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

/**
 * @brief Effectue la segmentation des caractères dans une image
 *
 * @return int 0 en cas de succès, 1 en cas d'erreur
 */
int perform_segmentation(const char *path);

#endif // SEGMENTATION_H

