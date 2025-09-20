#ifndef PREPROCESSING_H
#define PREPROCESSING_H


void to_grayscale(SDL_Surface* s);
void grayscale_inplace(SDL_Surface* s);
uint8_t otsu_threshold(SDL_Surface* s);
void threshold_inplace(SDL_Surface* s, uint8_t th, int invert);
int preprocess_image(const char* path);

#endif