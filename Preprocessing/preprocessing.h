#ifndef PREPROCESSING_H
#define PREPROCESSING_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

void to_grayscale(SDL_Surface* s);
void grayscale_inplace(SDL_Surface* s);
uint8_t otsu_threshold(SDL_Surface* s);
void threshold_inplace(SDL_Surface* s, uint8_t th, int invert);
int preprocess_image(const char* path, const char* outName);
void median_filter(SDL_Surface *s);

#endif
