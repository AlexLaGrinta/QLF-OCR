#include "image_segmentation.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

// ============================================================================
// STRUCTURES INTERNES
// ============================================================================

typedef struct {
    int x, y;
} Point;

typedef struct {
    Point* points;
    int count;
    int capacity;
} Cluster;

// ============================================================================
// FONCTIONS UTILITAIRES CLUSTER
// ============================================================================

static Cluster* create_cluster(int capacity) {
    Cluster* cluster = malloc(sizeof(Cluster));
    cluster->points = malloc(capacity * sizeof(Point));
    cluster->count = 0;
    cluster->capacity = capacity;
    return cluster;
}

static void add_to_cluster(Cluster* cluster, Point p) {
    if (cluster->count >= cluster->capacity) {
        cluster->capacity *= 2;
        cluster->points = realloc(cluster->points, cluster->capacity * sizeof(Point));
    }
    cluster->points[cluster->count++] = p;
}

static void free_cluster(Cluster* cluster) {
    if (cluster) {
        free(cluster->points);
        free(cluster);
    }
}

static GridBounds cluster_bounding_box(Cluster* cluster) {
    if (cluster->count == 0) return (GridBounds){0};

    int min_x = cluster->points[0].x;
    int max_x = cluster->points[0].x;
    int min_y = cluster->points[0].y;
    int max_y = cluster->points[0].y;

    for (int i = 1; i < cluster->count; i++) {
        Point p = cluster->points[i];
        if (p.x < min_x) min_x = p.x;
        if (p.x > max_x) max_x = p.x;
        if (p.y < min_y) min_y = p.y;
        if (p.y > max_y) max_y = p.y;
    }

    return (GridBounds){
        .x = min_x,
        .y = min_y,
        .width = max_x - min_x + 1,
        .height = max_y - min_y + 1,
        .valid = true
    };
}

// ============================================================================
// CRÉATION D'IMAGE BINAIRE
// ============================================================================

BinaryImage* create_binary_image_from_surface(SDL_Surface* surface) {
    if (!surface) return NULL;

    BinaryImage* bin = malloc(sizeof(BinaryImage));
    if (!bin) return NULL;

    bin->width = surface->w;
    bin->height = surface->h;
    bin->data = malloc(bin->width * bin->height);
    
    if (!bin->data) {
        free(bin);
        return NULL;
    }

    SDL_Surface* rgb_surface = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGB24, 0);
    if (!rgb_surface) {
        free(bin->data);
        free(bin);
        return NULL;
    }

    SDL_LockSurface(rgb_surface);
    uint8_t* pixels = (uint8_t*)rgb_surface->pixels;

    for (int y = 0; y < bin->height; y++) {
        for (int x = 0; x < bin->width; x++) {
            int idx = y * rgb_surface->pitch + x * 3;
            uint8_t r = pixels[idx];
            uint8_t g = pixels[idx + 1];
            uint8_t b = pixels[idx + 2];
            
            uint8_t luminance = (uint8_t)(0.299 * r + 0.587 * g + 0.114 * b);
            bin->data[y * bin->width + x] = (luminance < 128) ? 0 : 255;
        }
    }

    SDL_UnlockSurface(rgb_surface);
    SDL_FreeSurface(rgb_surface);

    return bin;
}

void free_binary_image(BinaryImage* img) {
    if (img) {
        free(img->data);
        free(img);
    }
}

// ============================================================================
// DÉTECTION DE LIGNES (pour détecter la grille)
// ============================================================================

static int* detect_horizontal_lines(BinaryImage* bin, int* num_lines) {
    int* lines = malloc(bin->height * sizeof(int));
    *num_lines = 0;

    // Projeter horizontalement
    int* projection = calloc(bin->height, sizeof(int));
    
    for (int y = 0; y < bin->height; y++) {
        for (int x = 0; x < bin->width; x++) {
            if (bin->data[y * bin->width + x] == 0) {
                projection[y]++;
            }
        }
    }

    // Trouver les pics (lignes denses)
    int threshold = (int)(bin->width * 0.3);
    
    for (int y = 1; y < bin->height - 1; y++) {
        if (projection[y] > threshold && 
            projection[y] >= projection[y-1] && 
            projection[y] >= projection[y+1]) {
            lines[(*num_lines)++] = y;
        }
    }

    free(projection);
    return lines;
}

static int* detect_vertical_lines(BinaryImage* bin, int* num_lines) {
    int* lines = malloc(bin->width * sizeof(int));
    *num_lines = 0;

    // Projeter verticalement
    int* projection = calloc(bin->width, sizeof(int));
    
    for (int x = 0; x < bin->width; x++) {
        for (int y = 0; y < bin->height; y++) {
            if (bin->data[y * bin->width + x] == 0) {
                projection[x]++;
            }
        }
    }

    // Trouver les pics
    int threshold = (int)(bin->height * 0.3);
    
    for (int x = 1; x < bin->width - 1; x++) {
        if (projection[x] > threshold && 
            projection[x] >= projection[x-1] && 
            projection[x] >= projection[x+1]) {
            lines[(*num_lines)++] = x;
        }
    }

    free(projection);
    return lines;
}

// ============================================================================
// DÉTECTION INTELLIGENTE DE LA GRILLE
// ============================================================================

GridBounds detect_grid_smart(BinaryImage* bin) {
    printf("  🔍 Smart grid detection...\n");

    GridBounds grid = {0};
    grid.valid = false;

    // 1. Détecter les lignes
    int num_h_lines, num_v_lines;
    int* h_lines = detect_horizontal_lines(bin, &num_h_lines);
    int* v_lines = detect_vertical_lines(bin, &num_v_lines);

    printf("     • Horizontal lines: %d\n", num_h_lines);
    printf("     • Vertical lines: %d\n", num_v_lines);

    // 2. Si on a assez de lignes régulières, c'est probablement la grille
    if (num_h_lines >= 5 && num_v_lines >= 5) {
        // Calculer l'espacement moyen
        int avg_h_spacing = 0, avg_v_spacing = 0;
        
        if (num_h_lines > 1) {
            for (int i = 1; i < num_h_lines; i++) {
                avg_h_spacing += h_lines[i] - h_lines[i-1];
            }
            avg_h_spacing /= (num_h_lines - 1);
        }
        
        if (num_v_lines > 1) {
            for (int i = 1; i < num_v_lines; i++) {
                avg_v_spacing += v_lines[i] - v_lines[i-1];
            }
            avg_v_spacing /= (num_v_lines - 1);
        }

        printf("     • Average spacing: H=%d, V=%d\n", avg_h_spacing, avg_v_spacing);

        // Vérifier la régularité (grille = espacement régulier)
        bool is_regular = (avg_h_spacing > 15 && avg_h_spacing < 100 &&
                          avg_v_spacing > 15 && avg_v_spacing < 100);

        if (is_regular) {
            grid.x = v_lines[0];
            grid.y = h_lines[0];
            grid.width = v_lines[num_v_lines - 1] - v_lines[0];
            grid.height = h_lines[num_h_lines - 1] - h_lines[0];
            grid.valid = true;
            
            printf("     ✓ Regular grid detected\n");
        }
    }

    free(h_lines);
    free(v_lines);

    // 3. Méthode de secours : chercher le plus grand carré/rectangle de contenu
    if (!grid.valid) {
        printf("     • Using density-based detection...\n");
        
        int min_x = bin->width, max_x = 0;
        int min_y = bin->height, max_y = 0;
        int pixel_count = 0;

        for (int y = 0; y < bin->height; y++) {
            for (int x = 0; x < bin->width; x++) {
                if (bin->data[y * bin->width + x] == 0) {
                    if (x < min_x) min_x = x;
                    if (x > max_x) max_x = x;
                    if (y < min_y) min_y = y;
                    if (y > max_y) max_y = y;
                    pixel_count++;
                }
            }
        }

        if (pixel_count > 1000) {
            grid.x = min_x;
            grid.y = min_y;
            grid.width = max_x - min_x + 1;
            grid.height = max_y - min_y + 1;
            grid.valid = true;
            printf("     ✓ Density-based grid found\n");
        }
    }

    if (grid.valid) {
        printf("     → Grid: (%d,%d) %dx%d\n", grid.x, grid.y, grid.width, grid.height);
    }

    return grid;
}

// ============================================================================
// DÉTECTION INTELLIGENTE DE LA LISTE DE MOTS
// ============================================================================

GridBounds detect_wordlist_smart(BinaryImage* bin, GridBounds grid) {
    printf("  🔍 Smart word list detection...\n");

    GridBounds list = {0};
    list.valid = false;

    if (!grid.valid) return list;

    // Créer 4 zones candidates (gauche, droite, haut, bas de la grille)
    typedef struct {
        GridBounds bounds;
        int density;
        const char* position;
    } Candidate;

    Candidate candidates[4] = {0};
    
    // Zone GAUCHE
    candidates[0].bounds = (GridBounds){
        .x = 0,
        .y = 0,
        .width = grid.x,
        .height = bin->height,
        .valid = true
    };
    candidates[0].position = "LEFT";

    // Zone DROITE
    candidates[1].bounds = (GridBounds){
        .x = grid.x + grid.width,
        .y = 0,
        .width = bin->width - (grid.x + grid.width),
        .height = bin->height,
        .valid = true
    };
    candidates[1].position = "RIGHT";

    // Zone HAUT
    candidates[2].bounds = (GridBounds){
        .x = 0,
        .y = 0,
        .width = bin->width,
        .height = grid.y,
        .valid = true
    };
    candidates[2].position = "TOP";

    // Zone BAS
    candidates[3].bounds = (GridBounds){
        .x = 0,
        .y = grid.y + grid.height,
        .width = bin->width,
        .height = bin->height - (grid.y + grid.height),
        .valid = true
    };
    candidates[3].position = "BOTTOM";

    // Calculer la densité de chaque zone
    for (int i = 0; i < 4; i++) {
        Candidate* c = &candidates[i];
        
        if (c->bounds.width <= 0 || c->bounds.height <= 0) {
            c->bounds.valid = false;
            continue;
        }

        int pixel_count = 0;
        int total_pixels = c->bounds.width * c->bounds.height;

        for (int y = c->bounds.y; y < c->bounds.y + c->bounds.height && y < bin->height; y++) {
            for (int x = c->bounds.x; x < c->bounds.x + c->bounds.width && x < bin->width; x++) {
                if (bin->data[y * bin->width + x] == 0) {
                    pixel_count++;
                }
            }
        }

        c->density = (total_pixels > 0) ? (pixel_count * 1000 / total_pixels) : 0;
        
        printf("     • %s zone: %dx%d, density=%d\n", 
               c->position, c->bounds.width, c->bounds.height, c->density);
    }

    // Choisir la zone avec la plus forte densité (mais pas trop faible)
    int best_idx = -1;
    int max_density = 5; // Seuil minimum
    
    for (int i = 0; i < 4; i++) {
        if (candidates[i].bounds.valid && candidates[i].density > max_density) {
            max_density = candidates[i].density;
            best_idx = i;
        }
    }

    if (best_idx != -1) {
        list = candidates[best_idx].bounds;
        
        // Affiner les limites (enlever les marges vides)
        int min_x = list.x + list.width;
        int max_x = list.x;
        int min_y = list.y + list.height;
        int max_y = list.y;

        for (int y = list.y; y < list.y + list.height && y < bin->height; y++) {
            for (int x = list.x; x < list.x + list.width && x < bin->width; x++) {
                if (bin->data[y * bin->width + x] == 0) {
                    if (x < min_x) min_x = x;
                    if (x > max_x) max_x = x;
                    if (y < min_y) min_y = y;
                    if (y > max_y) max_y = y;
                }
            }
        }

        if (min_x <= max_x && min_y <= max_y) {
            list.x = min_x;
            list.y = min_y;
            list.width = max_x - min_x + 1;
            list.height = max_y - min_y + 1;
            list.valid = true;
            
            printf("     ✓ Word list found at %s\n", candidates[best_idx].position);
            printf("     → List: (%d,%d) %dx%d\n", list.x, list.y, list.width, list.height);
        }
    } else {
        printf("     ⚠️  No word list detected\n");
    }

    return list;
}

// ============================================================================
// DÉTECTION AMÉLIORÉE AVEC LES DEUX MÉTHODES
// ============================================================================

int detect_major_zones_improved(BinaryImage* bin, GridBounds* grid_bounds, GridBounds* list_bounds) {
    if (!bin || !grid_bounds || !list_bounds) return 0;

    printf("  🎯 Detecting zones...\n\n");

    // 1. Détecter la grille (structure régulière)
    *grid_bounds = detect_grid_smart(bin);
    
    if (!grid_bounds->valid) {
        printf("     ✗ Grid detection failed\n");
        return 0;
    }

    printf("\n");

    // 2. Détecter la liste de mots (ce qui reste)
    *list_bounds = detect_wordlist_smart(bin, *grid_bounds);

    return grid_bounds->valid ? 1 : 0;
}

// ============================================================================
// UTILITAIRES
// ============================================================================

void draw_rectangle_color(SDL_Surface* surface, int x, int y, int w, int h, Uint8 r, Uint8 g, Uint8 b) {
    if (!surface) return;

    SDL_LockSurface(surface);
    
    for (int thickness = 0; thickness < 3; thickness++) {
        for (int dx = 0; dx < w; dx++) {
            int px = x + dx;
            int py_top = y + thickness;
            int py_bottom = y + h - thickness - 1;
            
            if (px >= 0 && px < surface->w) {
                if (py_top >= 0 && py_top < surface->h) {
                    Uint32* pixel = (Uint32*)((Uint8*)surface->pixels + py_top * surface->pitch + px * 4);
                    *pixel = SDL_MapRGB(surface->format, r, g, b);
                }
                if (py_bottom >= 0 && py_bottom < surface->h) {
                    Uint32* pixel = (Uint32*)((Uint8*)surface->pixels + py_bottom * surface->pitch + px * 4);
                    *pixel = SDL_MapRGB(surface->format, r, g, b);
                }
            }
        }
        
        for (int dy = 0; dy < h; dy++) {
            int py = y + dy;
            int px_left = x + thickness;
            int px_right = x + w - thickness - 1;
            
            if (py >= 0 && py < surface->h) {
                if (px_left >= 0 && px_left < surface->w) {
                    Uint32* pixel = (Uint32*)((Uint8*)surface->pixels + py * surface->pitch + px_left * 4);
                    *pixel = SDL_MapRGB(surface->format, r, g, b);
                }
                if (px_right >= 0 && px_right < surface->w) {
                    Uint32* pixel = (Uint32*)((Uint8*)surface->pixels + py * surface->pitch + px_right * 4);
                    *pixel = SDL_MapRGB(surface->format, r, g, b);
                }
            }
        }
    }

    SDL_UnlockSurface(surface);
}

void save_surface(const char* filename, SDL_Surface* surface) {
    if (surface) {
        SDL_SaveBMP(surface, filename);
    }
}

void free_grid_zone(GridZone* grid) {
    free(grid);
}

void free_word_list(WordList* list) {
    if (list) {
        for (int i = 0; i < list->count; i++) {
            free(list->words[i]);
        }
        free(list->words);
        free(list);
    }
}

