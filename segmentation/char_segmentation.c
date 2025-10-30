#include "char_segmentation.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// ============================================================================
// STRUCTURES
// ============================================================================

typedef struct {
    int min_x, max_x;
    int min_y, max_y;
    int pixel_count;
    bool valid;
} Component;

typedef struct {
    int* x;
    int* y;
    int front;
    int rear;
    int capacity;
} Queue;

// ============================================================================
// FILE (QUEUE) POUR BFS
// ============================================================================

static Queue* create_queue(int capacity) {
    Queue* q = malloc(sizeof(Queue));
    if (!q) return NULL;
    
    q->x = malloc(capacity * sizeof(int));
    q->y = malloc(capacity * sizeof(int));
    if (!q->x || !q->y) {
        free(q->x);
        free(q->y);
        free(q);
        return NULL;
    }
    
    q->front = 0;
    q->rear = 0;
    q->capacity = capacity;
    return q;
}

static void free_queue(Queue* q) {
    if (q) {
        free(q->x);
        free(q->y);
        free(q);
    }
}

static void enqueue(Queue* q, int x, int y) {
    if (q->rear < q->capacity) {
        q->x[q->rear] = x;
        q->y[q->rear] = y;
        q->rear++;
    }
}

static bool dequeue(Queue* q, int* x, int* y) {
    if (q->front >= q->rear) return false;
    *x = q->x[q->front];
    *y = q->y[q->front];
    q->front++;
    return true;
}

// ============================================================================
// BFS POUR DÉTECTION DE COMPOSANTES
// ============================================================================

static void bfs_component(BinaryImage* bin, int start_x, int start_y, 
                         uint8_t* visited, Component* comp) {
    Queue* q = create_queue(bin->width * bin->height);
    if (!q) return;

    enqueue(q, start_x, start_y);
    visited[start_y * bin->width + start_x] = 1;

    int x, y;
    while (dequeue(q, &x, &y)) {
        comp->pixel_count++;

        if (x < comp->min_x) comp->min_x = x;
        if (x > comp->max_x) comp->max_x = x;
        if (y < comp->min_y) comp->min_y = y;
        if (y > comp->max_y) comp->max_y = y;

        // Explorer les 8 voisins
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                if (dx == 0 && dy == 0) continue;

                int nx = x + dx;
                int ny = y + dy;

                if (nx >= 0 && nx < bin->width && ny >= 0 && ny < bin->height) {
                    int idx = ny * bin->width + nx;
                    if (bin->data[idx] == 0 && !visited[idx]) {
                        visited[idx] = 1;
                        enqueue(q, nx, ny);
                    }
                }
            }
        }
    }

    free_queue(q);
}

Character* find_characters_in_grid(BinaryImage* bin, int* num_chars, int offset_x, int offset_y) {
    if (!bin || !bin->data || !num_chars) return NULL;

    *num_chars = 0;
    uint8_t* visited = calloc(bin->width * bin->height, sizeof(uint8_t));
    if (!visited) return NULL;

    Component* temp_comps = malloc(1000 * sizeof(Component));
    if (!temp_comps) {
        free(visited);
        return NULL;
    }

    int comp_count = 0;

    printf("  🔍 Scanning for connected components...\n");

    // Parcourir l'image
    for (int y = 0; y < bin->height; y++) {
        for (int x = 0; x < bin->width; x++) {
            if (bin->data[y * bin->width + x] == 0 && !visited[y * bin->width + x]) {
                Component comp = {
                    .min_x = x, .max_x = x,
                    .min_y = y, .max_y = y,
                    .pixel_count = 0,
                    .valid = false
                };

                bfs_component(bin, x, y, visited, &comp);

                int width = comp.max_x - comp.min_x + 1;
                int height = comp.max_y - comp.min_y + 1;

                comp.valid = true;
                temp_comps[comp_count++] = comp;
                if (comp_count >= 1000) goto extraction_done;
            }
        }
    }

extraction_done:
    printf("     • Found %d components\n", comp_count);

    if (comp_count == 0) {
        free(visited);
        free(temp_comps);
        return NULL;
    }

    // Créer les caractères
    Character* chars = malloc(comp_count * sizeof(Character));
    if (!chars) {
        free(visited);
        free(temp_comps);
        return NULL;
    }

    *num_chars = comp_count;

    for (int i = 0; i < comp_count; i++) {
        Component* comp = &temp_comps[i];
        
        chars[i].x = comp->min_x + offset_x;
        chars[i].y = comp->min_y + offset_y;
        chars[i].width = comp->max_x - comp->min_x + 1;
        chars[i].height = comp->max_y - comp->min_y + 1;
        chars[i].zone = ZONE_UNKNOWN;
        
        int size = chars[i].width * chars[i].height;
        chars[i].data = malloc(size);
        
        if (chars[i].data) {
            memset(chars[i].data, 255, size);
            
            for (int y = comp->min_y; y <= comp->max_y; y++) {
                for (int x = comp->min_x; x <= comp->max_x; x++) {
                    if (bin->data[y * bin->width + x] == 0) {
                        int local_x = x - comp->min_x;
                        int local_y = y - comp->min_y;
                        chars[i].data[local_y * chars[i].width + local_x] = 0;
                    }
                }
            }
        }
    }

    free(visited);
    free(temp_comps);
    return chars;
}

// ============================================================================
// ASSIGNATION DES ZONES
// ============================================================================

void assign_character_zones(Character* chars, int num_chars, 
                            GridBounds grid_bounds, GridBounds list_bounds) {
    if (!chars) return;

    printf("  🎯 Assigning character zones...\n");

    int grid_count = 0;
    int list_count = 0;

    for (int i = 0; i < num_chars; i++) {
        int cx = chars[i].x + chars[i].width / 2;
        int cy = chars[i].y + chars[i].height / 2;

        // Vérifier si dans la grille
        bool in_grid = (cx >= grid_bounds.x && 
                       cx < grid_bounds.x + grid_bounds.width &&
                       cy >= grid_bounds.y && 
                       cy < grid_bounds.y + grid_bounds.height);

        // Vérifier si dans la liste
        bool in_list = false;
        if (list_bounds.valid) {
            in_list = (cx >= list_bounds.x && 
                      cx < list_bounds.x + list_bounds.width &&
                      cy >= list_bounds.y && 
                      cy < list_bounds.y + list_bounds.height);
        }

        if (in_grid) {
            chars[i].zone = ZONE_GRID;
            grid_count++;
        } else if (in_list) {
            chars[i].zone = ZONE_WORDLIST;
            list_count++;
        } else {
            chars[i].zone = ZONE_UNKNOWN;
        }
    }

    printf("     • Grid letters: %d\n", grid_count);
    printf("     • Word list letters: %d\n", list_count);
    printf("     • Unknown: %d\n", num_chars - grid_count - list_count);
}


// ============================================================================
// FILTRAGE
// ============================================================================

Character* filter_letters(Character* chars, int* num_chars) {
    if (!chars || !num_chars || *num_chars == 0) return chars;

    printf("  🔧 Filtering characters...\n");
    printf("     • Initial: %d\n", *num_chars);

    int total_w = 0, total_h = 0;
    for (int i = 0; i < *num_chars; i++) {
        total_w += chars[i].width;
        total_h += chars[i].height;
    }
    
    int avg_w = total_w / *num_chars;
    int avg_h = total_h / *num_chars;

    printf("     • Average size: %dx%d\n", avg_w, avg_h);

    int kept = 0;
    for (int i = 0; i < *num_chars; i++) {
        int w = chars[i].width;
        int h = chars[i].height;
        float aspect = (float)w / h;

        bool valid = (
            w >= avg_w * 0.3 && w <= avg_w * 3.0 &&
            h >= avg_h * 0.3 && h <= avg_h * 3.0 &&
            aspect >= 0.2 && aspect <= 5.0
        );

        if (valid) {
            if (kept != i) {
                chars[kept] = chars[i];
            }
            kept++;
        } else {
            free(chars[i].data);
        }
    }

    *num_chars = kept;
    printf("     • After filtering: %d\n", kept);

    return realloc(chars, kept * sizeof(Character));
}

// ============================================================================
// SAUVEGARDE BMP
// ============================================================================

void save_character_bmp(Character* ch, const char* filename, int target_size) {
    if (!ch || !ch->data) return;

    SDL_Surface* surf = SDL_CreateRGBSurfaceWithFormat(
        0, ch->width, ch->height, 32, SDL_PIXELFORMAT_RGBA32
    );
    
    if (!surf) return;

    SDL_LockSurface(surf);
    uint8_t* pixels = (uint8_t*)surf->pixels;

    for (int y = 0; y < ch->height; y++) {
        for (int x = 0; x < ch->width; x++) {
            uint8_t val = ch->data[y * ch->width + x];
            uint32_t color = SDL_MapRGBA(surf->format, val, val, val, 255);
            *(uint32_t*)&pixels[y * surf->pitch + x * 4] = color;
        }
    }

    SDL_UnlockSurface(surf);

    if (target_size > 0 && (ch->width != target_size || ch->height != target_size)) {
        SDL_Surface* resized = SDL_CreateRGBSurfaceWithFormat(
            0, target_size, target_size, 32, SDL_PIXELFORMAT_RGBA32
        );
        
        if (resized) {
            SDL_FillRect(resized, NULL, SDL_MapRGB(resized->format, 255, 255, 255));
            
            SDL_Rect dst;
            float scale = fminf((float)target_size / ch->width, (float)target_size / ch->height);
            dst.w = (int)(ch->width * scale);
            dst.h = (int)(ch->height * scale);
            dst.x = (target_size - dst.w) / 2;
            dst.y = (target_size - dst.h) / 2;
            
            SDL_BlitScaled(surf, NULL, resized, &dst);
            SDL_FreeSurface(surf);
            surf = resized;
        }
    }

    SDL_SaveBMP(surf, filename);
    SDL_FreeSurface(surf);
}

void free_characters(Character* chars, int num_chars) {
    if (!chars) return;
    for (int i = 0; i < num_chars; i++) {
        free(chars[i].data);
    }
    free(chars);
}

