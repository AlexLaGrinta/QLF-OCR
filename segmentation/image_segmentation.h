#ifndef IMAGE_SEGMENTATION_H
#define IMAGE_SEGMENTATION_H

#include <stdint.h>
#include <stdbool.h>
#include <SDL2/SDL.h>

typedef struct {
    uint8_t* data;
    int width;
    int height;
} BinaryImage;

typedef struct {
    int x, y;
    int width, height;
    bool valid;
} GridBounds;

typedef struct {
    int rows;
    int cols;
    float cell_width;
    float cell_height;
    GridBounds box;
} GridZone;

typedef struct {
    char** words;
    int count;
    GridBounds box;
} WordList;

BinaryImage* create_binary_image_from_surface(SDL_Surface* surface);
void free_binary_image(BinaryImage* img);

GridBounds detect_grid_smart(BinaryImage* bin);

GridBounds detect_wordlist_smart(BinaryImage* bin, GridBounds grid);

int detect_major_zones_improved(BinaryImage* bin, GridBounds* grid_bounds, GridBounds* list_bounds);

void draw_rectangle_color(SDL_Surface* surface, int x, int y, int w, int h, Uint8 r, Uint8 g, Uint8 b);
void save_surface(const char* filename, SDL_Surface* surface);
void free_grid_zone(GridZone* grid);
void free_word_list(WordList* list);

#endif

