#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <err.h>
#include "ocr.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "bmp_converter.h"



void ocr_main(const char* path)
{

    
}

int main()
{

    BinaryMatrix matrix = img_to_binary_matrix_strict10("selem.png");
    print_matrix(matrix);

    return EXIT_SUCCESS;
}