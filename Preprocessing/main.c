#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include "preprocessing.h"

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        errx(1,"Missing path.");
    }

    const char* inPath  = argv[1];

    if (preprocess_image(inPath, "output.bmp") != 0) return 1;

    return EXIT_SUCCESS;
}
