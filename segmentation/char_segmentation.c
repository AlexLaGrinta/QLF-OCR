#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include "char_segmentation.h"
#include "bmp_utils.h"

// Fonction pour libérer un tableau de Character
void free_characters(Character* chars, int num_chars) {
    if (!chars) return;
    for (int i = 0; i < num_chars; i++) {
        free(chars[i].data);
    }
    free(chars);
}

// Trouver les caractères (connected components)
Character* find_characters(const BinaryImage* bin, int* num_chars) {
    if (!bin || !num_chars) return NULL;

    int w = bin->width, h = bin->height;
    uint8_t* visited = calloc(w * h, 1);
    if (!visited) return NULL;

    Character* chars = NULL;
    *num_chars = 0;

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            if (bin->data[y * w + x] == 0 && !visited[y * w + x]) {
                int minx = x, maxx = x, miny = y, maxy = y;

                // Allouer des files d'attente pour le BFS
                int* qx = malloc(w * h * sizeof(int));
                int* qy = malloc(w * h * sizeof(int));
                if (!qx || !qy) {
                    free(qx);
                    free(qy);
                    free(visited);
                    free_characters(chars, *num_chars);
                    return NULL;
                }

                int front = 0, rear = 0;
                qx[rear] = x;
                qy[rear] = y;
                rear++;
                visited[y * w + x] = 1;

                // BFS pour trouver la composante connexe
                while (front < rear) {
                    int cx = qx[front], cy = qy[front];
                    front++;

                    if (cx < minx) minx = cx;
                    if (cx > maxx) maxx = cx;
                    if (cy < miny) miny = cy;
                    if (cy > maxy) maxy = cy;

                    // Parcourir les 8 voisins
                    for (int dy = -1; dy <= 1; dy++) {
                        for (int dx = -1; dx <= 1; dx++) {
                            int nx = cx + dx, ny = cy + dy;
                            if (nx >= 0 && nx < w && ny >= 0 && ny < h) {
                                if (bin->data[ny * w + nx] == 0 && !visited[ny * w + nx]) {
                                    visited[ny * w + nx] = 1;
                                    qx[rear] = nx;
                                    qy[rear] = ny;
                                    rear++;
                                }
                            }
                        }
                    }
                }

                // Extraire la région du caractère
                int cw = maxx - minx + 1;
                int ch = maxy - miny + 1;
                uint8_t* data = malloc(cw * ch);
                if (!data) {
                    free(qx);
                    free(qy);
                    free(visited);
                    free_characters(chars, *num_chars);
                    return NULL;
                }

                for (int yy = 0; yy < ch; yy++) {
                    for (int xx = 0; xx < cw; xx++) {
                        data[yy * cw + xx] = bin->data[(miny + yy) * w + (minx + xx)];
                    }
                }

                // Ajouter le caractère à la liste
                (*num_chars)++;
                Character* temp = realloc(chars, (*num_chars) * sizeof(Character));
                if (!temp) {
                    free(data);
                    free(qx);
                    free(qy);
                    free(visited);
                    free_characters(chars, *num_chars - 1);
                    return NULL;
                }
                chars = temp;
                chars[*num_chars - 1] = (Character){data, cw, ch, minx, miny};

                free(qx);
                free(qy);
            }
        }
    }

    free(visited);
    return chars;
}

// Sauvegarde d'un caractère en BMP
void save_character(const char* filename, const Character* ch) {
    if (!ch || !filename) return;

    // Créer le dossier "chars" s'il n'existe pas
    if (mkdir("chars", 0777) != 0 && errno != EEXIST) {
        fprintf(stderr, "mkdir(chars) failed: %s\n", strerror(errno));
        return;
    }

    char path[256];
    snprintf(path, sizeof(path), "chars/%s.bmp", filename);

    // Calculer les tailles pour le BMP
    int row_size = (ch->width + 3) & ~3; // Alignement sur 4 octets
    int image_size = row_size * ch->height;
    int file_size = sizeof(BMPHeader) + sizeof(BMPInfoHeader) + 256 * 4 + image_size;

    FILE* f = fopen(path, "wb");
    if (!f) {
        perror("salam");
        return;
    }

    // Écrire l'en-tête BMP
    BMPHeader header = {
        .type = 0x4D42,
        .size = file_size,
        .reserved1 = 0,
        .reserved2 = 0,
        .offset = sizeof(BMPHeader) + sizeof(BMPInfoHeader) + 256 * 4
    };
    fwrite(&header, sizeof(header), 1, f);

    // Écrire l'en-tête DIB (8 bits par pixel)
    BMPInfoHeader info = {
        .size = sizeof(BMPInfoHeader),
        .width = ch->width,
        .height = ch->height,
        .planes = 1,
        .bpp = 8,
        .compression = 0,
        .image_size = image_size,
        .x_pixels_per_meter = 2835,
        .y_pixels_per_meter = 2835,
        .colors_used = 256,
        .colors_important = 0
    };
    fwrite(&info, sizeof(info), 1, f);

    // Écrire la palette de gris
    for (int i = 0; i < 256; i++) {
        uint8_t color[4] = {i, i, i, 0};
        fwrite(color, sizeof(color), 1, f);
    }

    // Écrire les données de l'image (inversées verticalement)
    uint8_t* row_buffer = malloc(row_size);
    if (!row_buffer) {
        perror("malloc");
        fclose(f);
        return;
    }

    for (int y = ch->height - 1; y >= 0; y--) {
        memcpy(row_buffer, &ch->data[y * ch->width], ch->width);
        for (int x = ch->width; x < row_size; x++) {
            row_buffer[x] = 0; // Padding
        }
        fwrite(row_buffer, row_size, 1, f);
    }

    free(row_buffer);
    fclose(f);
}

// Libération d'un Character
void free_character(Character* ch) {
    if (ch) {
        free(ch->data);
    }
}

