#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

// Heuristique simple pour détecter si une composante ressemble à une lettre
static int is_likely_letter(const Character* ch) {
    if (!ch) return 0;

    // Seuils de taille minimaux pour éliminer le bruit
    if (ch->width < 5 || ch->height < 5) return 0;

    // Ratio d'aspect raisonnable pour des lettres imprimées
    float aspect = (float)ch->width / (float)ch->height;
    if (aspect < 0.2f || aspect > 5.0f) return 0;

    // Densité de pixels sombres (0 = noir, 255 = blanc)
    int dark = 0;
    int total = ch->width * ch->height;
    for (int y = 0; y < ch->height; y++) {
        for (int x = 0; x < ch->width; x++) {
            if (ch->data[y * ch->width + x] < 128) dark++;
        }
    }
    float density = (float)dark / (float)total;

    // Densité trop faible -> bruit; trop élevée -> bloc plein
    if (density < 0.03f || density > 0.7f) return 0;

    // Seuil d'aire minimale pour éliminer petites particules
    if (total < 30) return 0;

    return 1;
}

// Filtre les composantes pour ne garder que les lettres probables
Character* filter_letters(Character* chars, int* num_chars) {
    if (!chars || !num_chars || *num_chars <= 0) return chars;

    Character* filtered = NULL;
    int kept = 0;

    for (int i = 0; i < *num_chars; i++) {
        if (is_likely_letter(&chars[i])) {
            Character* tmp = realloc(filtered, (kept + 1) * sizeof(Character));
            if (!tmp) {
                // En cas d'échec, libérer ce qui a été copié et retourner l'original
                for (int k = 0; k < kept; k++) free(filtered[k].data);
                free(filtered);
                return chars;
            }
            filtered = tmp;

            // Copier la structure et dupliquer le buffer pour propriété claire
            filtered[kept].width = chars[i].width;
            filtered[kept].height = chars[i].height;
            filtered[kept].x = chars[i].x;
            filtered[kept].y = chars[i].y;
            int size = chars[i].width * chars[i].height;
            filtered[kept].data = malloc(size);
            if (!filtered[kept].data) {
                for (int k = 0; k < kept; k++) free(filtered[k].data);
                free(filtered);
                return chars;
            }
            memcpy(filtered[kept].data, chars[i].data, size);
            kept++;
        }
        // Libérer la composante originale dans tous les cas; on reconstruit filtered
        free(chars[i].data);
    }

    free(chars);
    *num_chars = kept;
    return filtered;
}

// Sauvegarde d'un caractère en BMP (28x28 centré)
void save_character(const char* filename, const Character* ch) {
    if (!ch || !filename) return;

    // Créer le dossier "chars" s'il n'existe pas
    if (mkdir("chars", 0777) != 0 && errno != EEXIST) {
        fprintf(stderr, "mkdir(chars) failed: %s\n", strerror(errno));
        return;
    }

    char path[256];
    snprintf(path, sizeof(path), "chars/%s.bmp", filename);

    const int target_size = 28;
    const int row_size = (target_size + 3) & ~3; // Alignement sur 4 octets
    int image_size = row_size * target_size;
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

    // Écrire l'en-tête DIB (8 bits par pixel, 28x28)
    BMPInfoHeader info = {
        .size = sizeof(BMPInfoHeader),
        .width = target_size,
        .height = target_size,
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

    // Créer une image 28x28 en échelle de gris
    uint8_t* scaled_data = calloc(target_size * target_size, sizeof(uint8_t));
    if (!scaled_data) {
        perror("malloc");
        fclose(f);
        return;
    }

    // Calculer le facteur d'échelle et la position de centrage
    float scale_x = (float)target_size / ch->width;
    float scale_y = (float)target_size / ch->height;
    float scale = (scale_x < scale_y) ? scale_x : scale_y; // Garder les proportions
    
    int scaled_w = (int)(ch->width * scale);
    int scaled_h = (int)(ch->height * scale);
    
    int offset_x = (target_size - scaled_w) / 2;
    int offset_y = (target_size - scaled_h) / 2;

    // Interpolation bilinéaire pour redimensionner
    for (int y = 0; y < target_size; y++) {
        for (int x = 0; x < target_size; x++) {
            int orig_x = (int)((x - offset_x) / scale);
            int orig_y = (int)((y - offset_y) / scale);
            
            if (orig_x >= 0 && orig_x < ch->width && orig_y >= 0 && orig_y < ch->height) {
                scaled_data[y * target_size + x] = ch->data[orig_y * ch->width + orig_x];
            } else {
                scaled_data[y * target_size + x] = 255; // Fond blanc pour les zones vides
            }
        }
    }

    // Écrire les données de l'image (inversées verticalement pour BMP)
    uint8_t* row_buffer = malloc(row_size);
    if (!row_buffer) {
        perror("malloc");
        free(scaled_data);
        fclose(f);
        return;
    }

    for (int y = target_size - 1; y >= 0; y--) {
        memcpy(row_buffer, &scaled_data[y * target_size], target_size);
        for (int x = target_size; x < row_size; x++) {
            row_buffer[x] = 0; // Padding
        }
        fwrite(row_buffer, row_size, 1, f);
    }

    free(scaled_data);
    free(row_buffer);
    fclose(f);
}

// Libération d'un Character
void free_character(Character* ch) {
    if (ch) {
        free(ch->data);
    }
}

