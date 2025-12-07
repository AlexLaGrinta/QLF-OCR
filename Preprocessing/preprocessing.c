#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "preprocessing.h"



int preprocess_image(const char* path, const char* outName)
{

    const char* outDir = "temp";
    //const char* outName = "output.bmp";
    char outPath[512];

    if (mkdir(outDir, 0777) != 0 && errno != EEXIST) {
        fprintf(stderr, "mkdir(%s) : %s\n", outDir, strerror(errno));
        return 1;
    }

    snprintf(outPath, sizeof(outPath), "%s/%s", outDir, outName);


    if (SDL_Init(0) != 0){
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    int imgFlags = IMG_INIT_PNG | IMG_INIT_JPG;
    if ((IMG_Init(imgFlags) & imgFlags) == 0){
        fprintf(stderr, "IMG_Init: %s\n", IMG_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Surface* src = IMG_Load(path);
    if (!src){
        fprintf(stderr, "IMG_Load: %s\n", IMG_GetError());
        IMG_Quit(); SDL_Quit();
        return 1;
    }

    SDL_Surface* img = SDL_ConvertSurfaceFormat(src, SDL_PIXELFORMAT_ARGB8888, 0);
    SDL_FreeSurface(src);
    if (!img){
        fprintf(stderr, "SDL_ConvertSurfaceFormat: %s\n", SDL_GetError());
        IMG_Quit(); SDL_Quit();
        return 1;
    }

    grayscale_inplace(img);

    // Denoise (not working)
    median_filter(img);

    uint8_t th = otsu_threshold(img);
    fprintf(stdout, "Otsu threshold = %u\n", th);

    threshold_inplace(img, th, 0);

    if (SDL_SaveBMP(img, outPath) != 0){
        fprintf(stderr, "Save (%s): %s\n", outPath, SDL_GetError());
        SDL_FreeSurface(img); IMG_Quit(); SDL_Quit();
        return 1;
    }

    SDL_FreeSurface(img);
    IMG_Quit();
    SDL_Quit();

    return EXIT_SUCCESS;

}





static inline uint8_t clamp_u8(int v){ return (uint8_t)(v < 0 ? 0 : (v > 255 ? 255 : v)); }

void grayscale_inplace(SDL_Surface* s){
    SDL_LockSurface(s);
    SDL_PixelFormat* f = s->format; 
    uint8_t* p = (uint8_t*)s->pixels;
    for (int y=0; y<s->h; ++y){
        uint32_t* row = (uint32_t*)(p + y*s->pitch);
        for (int x=0; x<s->w; ++x){
            uint32_t px = row[x];

            uint32_t R = (px & f->Rmask) >> f->Rshift; if (f->Rloss) R >>= f->Rloss;
            uint32_t G = (px & f->Gmask) >> f->Gshift; if (f->Gloss) G >>= f->Gloss;
            uint32_t B = (px & f->Bmask) >> f->Bshift; if (f->Bloss) B >>= f->Bloss;
            uint32_t A = (px & f->Amask) >> f->Ashift; if (f->Aloss) A >>= f->Aloss;

            int y8 = (77 * (int)R + 150 * (int)G + 29 * (int)B) >> 8; // 77+150+29=256
            uint8_t Y = clamp_u8(y8);

            uint32_t Rv = (uint32_t)Y << f->Rshift;
            uint32_t Gv = (uint32_t)Y << f->Gshift;
            uint32_t Bv = (uint32_t)Y << f->Bshift;
            uint32_t Av = ((uint32_t)A) << f->Ashift;
            row[x] = Av | Rv | Gv | Bv;
        }
    }
    SDL_UnlockSurface(s);
}

static void histogram_gray(SDL_Surface* s, uint32_t hist[256]){
    memset(hist, 0, 256*sizeof(uint32_t));
    SDL_LockSurface(s);
    SDL_PixelFormat* f = s->format;
    uint8_t* p = (uint8_t*)s->pixels;
    for (int y=0; y<s->h; ++y){
        uint32_t* row = (uint32_t*)(p + y*s->pitch);
        for (int x=0; x<s->w; ++x){
            uint32_t px = row[x];
            uint32_t R = (px & f->Rmask) >> f->Rshift; if (f->Rloss) R >>= f->Rloss;
            uint8_t Y = (uint8_t)R;
            hist[Y]++;
        }
    }
    SDL_UnlockSurface(s);
}

uint8_t otsu_threshold(SDL_Surface* s){
    uint32_t hist[256];
    histogram_gray(s, hist);

    uint64_t total = (uint64_t)s->w * (uint64_t)s->h;
    if (total == 0) return 0;

    uint64_t sum_all = 0;
    for (int i=0;i<256;i++) sum_all += (uint64_t)i * hist[i];

    uint64_t sumB = 0;
    uint64_t wB = 0;
    double maxVar = -1.0;
    int thresh = 0;

    for (int t=0; t<256; t++){
        wB += hist[t];               
        if (wB == 0) continue;
        uint64_t wF = total - wB;    
        if (wF == 0) break;

        sumB += (uint64_t)t * hist[t];

        double mB = (double)sumB / (double)wB;
        double mF = (double)(sum_all - sumB) / (double)wF;
        double diff = mB - mF;
        double varBetween = (double)wB * (double)wF * diff * diff;

        if (varBetween > maxVar){
            maxVar = varBetween;
            thresh = t;
        }
    }
    return (uint8_t)thresh;
}

void threshold_inplace(SDL_Surface* s, uint8_t th, int invert){
    SDL_LockSurface(s);
    SDL_PixelFormat* f = s->format;
    uint8_t* p = (uint8_t*)s->pixels;
    for (int y=0; y<s->h; ++y){
        uint32_t* row = (uint32_t*)(p + y*s->pitch);
        for (int x=0; x<s->w; ++x){
            uint32_t px = row[x];
            uint32_t R = (px & f->Rmask) >> f->Rshift; if (f->Rloss) R >>= f->Rloss;
            uint32_t A = (px & f->Amask) >> f->Ashift; if (f->Aloss) A >>= f->Aloss;

            uint8_t Y = (uint8_t)R; 
            uint8_t v = (Y >= th) ? 255 : 0;
            if (invert) v = 255 - v;

            uint32_t Rv = ((uint32_t)v) << f->Rshift;
            uint32_t Gv = ((uint32_t)v) << f->Gshift;
            uint32_t Bv = ((uint32_t)v) << f->Bshift;
            uint32_t Av = ((uint32_t)A) << f->Ashift;
            row[x] = Av | Rv | Gv | Bv;
        }
    }
    SDL_UnlockSurface(s);
}

static int compare_uint8(const void *a, const void *b) {
    return (*(uint8_t*)a - *(uint8_t*)b);
}

void median_filter(SDL_Surface *s) {
    if (!s) return;

    SDL_LockSurface(s);
    
    int w = s->w;
    int h = s->h;
    SDL_PixelFormat* f = s->format;
    
    size_t img_size = h * s->pitch;
    uint8_t* backup = malloc(img_size);
    memcpy(backup, s->pixels, img_size);
    
    for (int y = 1; y < h - 1; y++) {
        for (int x = 1; x < w - 1; x++) {
            uint32_t* src_row = (uint32_t*)(backup + y * s->pitch);
            uint32_t pixel = src_row[x];
            uint32_t R = (pixel & f->Rmask) >> f->Rshift;
            if (f->Rloss) R >>= f->Rloss;
            uint8_t original = (uint8_t)R;
            
            uint8_t values[9];
            int k = 0;
            for (int dy = -1; dy <= 1; dy++) {
                uint32_t* row = (uint32_t*)(backup + (y + dy) * s->pitch);
                for (int dx = -1; dx <= 1; dx++) {
                    uint32_t npixel = row[x + dx];
                    uint32_t nR = (npixel & f->Rmask) >> f->Rshift;
                    if (f->Rloss) nR >>= f->Rloss;
                    values[k++] = (uint8_t)nR;
                }
            }
            
            qsort(values, 9, sizeof(uint8_t), compare_uint8);
            uint8_t min_val = values[0];
            uint8_t max_val = values[8];
            uint8_t median = values[4];
            
            uint8_t new_value;
            int range = max_val - min_val;
            
            int similar_count = 0;
            for (int i = 0; i < 9; i++) {
                if (abs((int)values[i] - (int)original) <= 15) {
                    similar_count++;
                }
            }
            
            if (range > 20 && similar_count < 3) {
                int deviation = abs((int)original - (int)median);
                if (deviation > 40) {
                    new_value = median;
                } else {
                    new_value = original;
                }
            } else {
                new_value = original;
            }
            
            uint32_t* dst_row = (uint32_t*)((uint8_t*)s->pixels + y * s->pitch);
            uint32_t alpha = 0xFF << f->Ashift;
            dst_row[x] = alpha | (new_value << f->Rshift) | (new_value << f->Gshift) | (new_value << f->Bshift);
        }
    }
    
    free(backup);
    SDL_UnlockSurface(s);
}
