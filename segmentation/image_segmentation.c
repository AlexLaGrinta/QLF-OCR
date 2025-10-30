#include "image_segmentation.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <float.h>
#include <stdbool.h>

// ============================================================================
// STRUCTURES
// ============================================================================

typedef struct {
    int min_x, max_x;
    int min_y, max_y;
    int center_x, center_y;
    int pixel_count;
    int cluster_id;
    bool is_grid;  // Marqueur pour la grille
} Component;

typedef struct {
    Component* components;
    int count;
    int capacity;
} ComponentList;

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
// EXTRACTION DES COMPOSANTES (LETTRES)
// ============================================================================

static ComponentList* create_component_list(int capacity) {
    ComponentList* list = malloc(sizeof(ComponentList));
    if (!list) return NULL;
    
    list->components = malloc(capacity * sizeof(Component));
    list->count = 0;
    list->capacity = capacity;
    return list;
}

static void add_component(ComponentList* list, Component comp) {
    if (list->count >= list->capacity) {
        list->capacity *= 2;
        list->components = realloc(list->components, list->capacity * sizeof(Component));
    }
    list->components[list->count++] = comp;
}

static void free_component_list(ComponentList* list) {
    if (list) {
        free(list->components);
        free(list);
    }
}

static void extract_component_bfs(BinaryImage* bin, bool* visited, int start_x, int start_y, Component* comp) {
    comp->min_x = start_x;
    comp->max_x = start_x;
    comp->min_y = start_y;
    comp->max_y = start_y;
    comp->pixel_count = 0;
    comp->cluster_id = -1;
    comp->is_grid = false;
    
    int* qx = malloc(bin->width * bin->height * sizeof(int));
    int* qy = malloc(bin->width * bin->height * sizeof(int));
    int front = 0, rear = 0;
    
    qx[rear] = start_x;
    qy[rear] = start_y;
    rear++;
    visited[start_y * bin->width + start_x] = true;
    
    int dx[] = {-1, 1, 0, 0, -1, -1, 1, 1};
    int dy[] = {0, 0, -1, 1, -1, 1, -1, 1};
    
    while (front < rear) {
        int x = qx[front];
        int y = qy[front];
        front++;
        
        comp->pixel_count++;
        
        if (x < comp->min_x) comp->min_x = x;
        if (x > comp->max_x) comp->max_x = x;
        if (y < comp->min_y) comp->min_y = y;
        if (y > comp->max_y) comp->max_y = y;
        
        for (int i = 0; i < 8; i++) {
            int nx = x + dx[i];
            int ny = y + dy[i];
            
            if (nx >= 0 && nx < bin->width && ny >= 0 && ny < bin->height) {
                int idx = ny * bin->width + nx;
                if (!visited[idx] && bin->data[idx] == 0) {
                    visited[idx] = true;
                    qx[rear] = nx;
                    qy[rear] = ny;
                    rear++;
                }
            }
        }
    }
    
    comp->center_x = (comp->min_x + comp->max_x) / 2;
    comp->center_y = (comp->min_y + comp->max_y) / 2;
    
    free(qx);
    free(qy);
}

static ComponentList* find_all_components(BinaryImage* bin) {
    printf("  🔍 Extracting letter components...\n");
    
    bool* visited = calloc(bin->width * bin->height, sizeof(bool));
    if (!visited) return NULL;
    
    ComponentList* list = create_component_list(500);
    if (!list) {
        free(visited);
        return NULL;
    }
    
    for (int y = 0; y < bin->height; y++) {
        for (int x = 0; x < bin->width; x++) {
            int idx = y * bin->width + x;
            
            if (!visited[idx] && bin->data[idx] == 0) {
                Component comp;
                extract_component_bfs(bin, visited, x, y, &comp);
                
                int w = comp.max_x - comp.min_x + 1;
                int h = comp.max_y - comp.min_y + 1;
                
                // Filtres plus permissifs
                if (w >= 3 && h >= 3 && w <= 200 && h <= 200 && 
                    comp.pixel_count >= 5) {
                    add_component(list, comp);
                }
            }
        }
    }
    
    free(visited);
    printf("     ✓ Found %d letter components\n", list->count);
    return list;
}

// ============================================================================
// DBSCAN CLUSTERING SIMPLIFIÉ
// ============================================================================

static float distance_between_components(Component* a, Component* b) {
    int dx = a->center_x - b->center_x;
    int dy = a->center_y - b->center_y;
    return sqrtf(dx * dx + dy * dy);
}

static int dbscan_simple(ComponentList* comps, float eps, int minPts) {
    printf("  🎯 Running DBSCAN clustering...\n");
    printf("     • Parameters: eps=%.1f, minPts=%d\n", eps, minPts);
    
    bool* visited = calloc(comps->count, sizeof(bool));
    int cluster_id = 0;
    
    for (int i = 0; i < comps->count; i++) {
        if (visited[i]) continue;
        
        // Trouver les voisins
        int* neighbors = malloc(comps->count * sizeof(int));
        int neighbor_count = 0;
        
        for (int j = 0; j < comps->count; j++) {
            if (i != j) {
                float dist = distance_between_components(&comps->components[i], &comps->components[j]);
                if (dist <= eps) {
                    neighbors[neighbor_count++] = j;
                }
            }
        }
        
        if (neighbor_count < minPts) {
            visited[i] = true;
            comps->components[i].cluster_id = -1;  // Bruit
            free(neighbors);
            continue;
        }
        
        // Créer un cluster
        comps->components[i].cluster_id = cluster_id;
        visited[i] = true;
        
        // Expansion
        for (int k = 0; k < neighbor_count; k++) {
            int neighbor_idx = neighbors[k];
            
            if (!visited[neighbor_idx]) {
                visited[neighbor_idx] = true;
                comps->components[neighbor_idx].cluster_id = cluster_id;
                
                // Trouver les voisins du voisin
                for (int j = 0; j < comps->count; j++) {
                    if (j != neighbor_idx) {
                        float dist = distance_between_components(
                            &comps->components[neighbor_idx], 
                            &comps->components[j]
                        );
                        if (dist <= eps) {
                            bool already_in = false;
                            for (int m = 0; m < neighbor_count; m++) {
                                if (neighbors[m] == j) {
                                    already_in = true;
                                    break;
                                }
                            }
                            if (!already_in && !visited[j]) {
                                neighbors[neighbor_count++] = j;
                            }
                        }
                    }
                }
            } else if (comps->components[neighbor_idx].cluster_id == -1) {
                comps->components[neighbor_idx].cluster_id = cluster_id;
            }
        }
        
        free(neighbors);
        cluster_id++;
    }
    
    free(visited);
    printf("     ✓ Found %d clusters\n", cluster_id);
    return cluster_id;
}

// ============================================================================
// ANALYSE SIMPLE DES CLUSTERS
// ============================================================================

typedef struct {
    int y;
    int count;
} LineInfo;

static int compare_by_y(const void* a, const void* b) {
    Component* c1 = (Component*)a;
    Component* c2 = (Component*)b;
    return c1->center_y - c2->center_y;
}

static float analyze_cluster_regularity(ComponentList* comps, int cluster_id, int* out_rows) {
    // Extraire les composantes du cluster
    int count = 0;
    for (int i = 0; i < comps->count; i++) {
        if (comps->components[i].cluster_id == cluster_id) {
            count++;
        }
    }
    
    if (count < 10) {  // Minimum 10 lettres pour une grille
        *out_rows = 0;
        return 0;
    }
    
    Component* cluster_comps = malloc(count * sizeof(Component));
    int idx = 0;
    for (int i = 0; i < comps->count; i++) {
        if (comps->components[i].cluster_id == cluster_id) {
            cluster_comps[idx++] = comps->components[i];
        }
    }
    
    // Trier par Y
    qsort(cluster_comps, count, sizeof(Component), compare_by_y);
    
    // Grouper en lignes (tolérance 20px)
    LineInfo* lines = malloc(count * sizeof(LineInfo));
    int num_lines = 0;
    int current_y = cluster_comps[0].center_y;
    int current_count = 1;
    
    for (int i = 1; i < count; i++) {
        if (abs(cluster_comps[i].center_y - current_y) < 20) {
            current_count++;
        } else {
            lines[num_lines].y = current_y;
            lines[num_lines].count = current_count;
            num_lines++;
            
            current_y = cluster_comps[i].center_y;
            current_count = 1;
        }
    }
    lines[num_lines].y = current_y;
    lines[num_lines].count = current_count;
    num_lines++;
    
    *out_rows = num_lines;
    
    // Calculer le score de régularité
    if (num_lines < 3) {
        free(cluster_comps);
        free(lines);
        return 0;
    }
    
    // Vérifier si toutes les lignes ont à peu près le même nombre de lettres
    int min_count = lines[0].count;
    int max_count = lines[0].count;
    int total_count = lines[0].count;
    
    for (int i = 1; i < num_lines; i++) {
        if (lines[i].count < min_count) min_count = lines[i].count;
        if (lines[i].count > max_count) max_count = lines[i].count;
        total_count += lines[i].count;
    }
    
    float avg_count = (float)total_count / num_lines;
    float count_regularity = 1.0f - (float)(max_count - min_count) / avg_count;
    
    // Vérifier l'espacement vertical
    float total_v_spacing = 0;
    for (int i = 0; i < num_lines - 1; i++) {
        total_v_spacing += lines[i + 1].y - lines[i].y;
    }
    float avg_v_spacing = total_v_spacing / (num_lines - 1);
    
    float v_variance = 0;
    for (int i = 0; i < num_lines - 1; i++) {
        float diff = (lines[i + 1].y - lines[i].y) - avg_v_spacing;
        v_variance += diff * diff;
    }
    float v_std = sqrtf(v_variance / (num_lines - 1));
    float v_regularity = 1.0f - fminf(v_std / avg_v_spacing, 1.0f);
    
    float regularity = (count_regularity * 0.6f) + (v_regularity * 0.4f);
    
    printf("     • Cluster %d: %d letters, %d lines, regularity=%.2f\n", 
           cluster_id, count, num_lines, regularity);
    printf("       → Letters per line: %.1f (min=%d, max=%d)\n", 
           avg_count, min_count, max_count);
    printf("       → Vertical spacing: %.1f ± %.1f\n", avg_v_spacing, v_std);
    
    free(cluster_comps);
    free(lines);
    
    return regularity;
}

// ============================================================================
// DÉTECTION ZONES PRINCIPALES
// ============================================================================

int detect_major_zones_improved(BinaryImage* bin, GridBounds* grid_bounds, GridBounds* list_bounds) {
    if (!bin || !grid_bounds || !list_bounds) return 0;

    printf("\n");
    printf("═══════════════════════════════════════════════════════\n");
    printf("  SIMPLIFIED DBSCAN GRID DETECTION\n");
    printf("═══════════════════════════════════════════════════════\n\n");

    // 1. Extraire toutes les composantes
    ComponentList* comps = find_all_components(bin);
    if (!comps || comps->count < 10) {
        printf("  ✗ Not enough components found\n");
        if (comps) free_component_list(comps);
        return 0;
    }

    // 2. Calculer eps automatiquement
    float total_dist = 0;
    int dist_samples = fminf(50, comps->count);
    
    for (int i = 0; i < dist_samples; i++) {
        float min_dist = FLT_MAX;
        for (int j = 0; j < comps->count; j++) {
            if (i != j) {
                float dist = distance_between_components(&comps->components[i], &comps->components[j]);
                if (dist < min_dist) min_dist = dist;
            }
        }
        total_dist += min_dist;
    }
    
    float eps = (total_dist / dist_samples) * 2.0f;  // Plus permissif
    if (eps < 30) eps = 30;  // Minimum raisonnable
    if (eps > 80) eps = 80;  // Maximum raisonnable
    
    printf("     • Computed eps=%.1f\n\n", eps);

    // 3. DBSCAN
    int num_clusters = dbscan_simple(comps, eps, 3);
    
    if (num_clusters == 0) {
        printf("  ✗ No clusters found\n");
        free_component_list(comps);
        return 0;
    }

    // 4. Analyser chaque cluster
    printf("\n  📊 Analyzing clusters for grid structure...\n");
    
    int best_cluster = -1;
    float best_regularity = 0;
    int best_rows = 0;
    
    for (int c = 0; c < num_clusters; c++) {
        int rows = 0;
        float regularity = analyze_cluster_regularity(comps, c, &rows);
        
        // Critères plus permissifs pour détecter la grille
        if (rows >= 3 && regularity > 0.3f && regularity > best_regularity) {
            best_regularity = regularity;
            best_cluster = c;
            best_rows = rows;
        }
    }

    // 5. Marquer la grille
    if (best_cluster >= 0) {
        printf("\n  🎯 Grid identified: Cluster %d (regularity: %.2f, rows: %d)\n", 
               best_cluster, best_regularity, best_rows);
        
        int min_x = INT_MAX, max_x = 0;
        int min_y = INT_MAX, max_y = 0;
        int grid_count = 0;
        
        for (int i = 0; i < comps->count; i++) {
            if (comps->components[i].cluster_id == best_cluster) {
                comps->components[i].is_grid = true;
                grid_count++;
                
                Component* c = &comps->components[i];
                if (c->min_x < min_x) min_x = c->min_x;
                if (c->max_x > max_x) max_x = c->max_x;
                if (c->min_y < min_y) min_y = c->min_y;
                if (c->max_y > max_y) max_y = c->max_y;
            }
        }
        
        grid_bounds->x = min_x - 10;
        grid_bounds->y = min_y - 10;
        grid_bounds->width = max_x - min_x + 20;
        grid_bounds->height = max_y - min_y + 20;
        grid_bounds->valid = true;
        
        printf("     → Grid bounds: (%d,%d) %dx%d\n", 
               grid_bounds->x, grid_bounds->y, 
               grid_bounds->width, grid_bounds->height);
        printf("     → Grid contains: %d letters\n", grid_count);
    } else {
        printf("\n  ✗ No grid structure detected\n");
        grid_bounds->valid = false;
    }

    // 6. Liste de mots = tout le reste
    printf("\n  📝 Detecting word list (remaining letters)...\n");
    
    int min_x = INT_MAX, max_x = 0;
    int min_y = INT_MAX, max_y = 0;
    int list_count = 0;
    
    for (int i = 0; i < comps->count; i++) {
        if (!comps->components[i].is_grid) {
            list_count++;
            
            Component* c = &comps->components[i];
            if (c->min_x < min_x) min_x = c->min_x;
            if (c->max_x > max_x) max_x = c->max_x;
            if (c->min_y < min_y) min_y = c->min_y;
            if (c->max_y > max_y) max_y = c->max_y;
        }
    }
    
    if (list_count > 0) {
        list_bounds->x = min_x - 10;
        list_bounds->y = min_y - 10;
        list_bounds->width = max_x - min_x + 20;
        list_bounds->height = max_y - min_y + 20;
        list_bounds->valid = true;
        
        printf("     ✓ Word list: %d letters\n", list_count);
        printf("     → List bounds: (%d,%d) %dx%d\n", 
               list_bounds->x, list_bounds->y, 
               list_bounds->width, list_bounds->height);
    } else {
        list_bounds->valid = false;
    }

    printf("\n  📊 Final summary:\n");
    printf("     • Total components: %d\n", comps->count);
    printf("     • Grid letters: %d\n", comps->count - list_count);
    printf("     • Word list letters: %d\n", list_count);

    free_component_list(comps);
    return (grid_bounds->valid || list_bounds->valid) ? 1 : 0;
}

// ============================================================================
// FONCTIONS LEGACY
// ============================================================================

GridBounds detect_grid_smart(BinaryImage* bin) {
    GridBounds grid = {0}, list = {0};
    detect_major_zones_improved(bin, &grid, &list);
    return grid;
}

GridBounds detect_wordlist_smart(BinaryImage* bin, GridBounds grid) {
    GridBounds unused_grid = {0}, list = {0};
    detect_major_zones_improved(bin, &unused_grid, &list);
    return list;
}

// ============================================================================
// UTILITAIRES
// ============================================================================

void draw_rectangle_color(SDL_Surface* surface, int x, int y, int w, int h, 
                         Uint8 r, Uint8 g, Uint8 b) {
    if (!surface) return;
    
    SDL_LockSurface(surface);
    
    for (int i = 0; i < 3; i++) {
        for (int px = x; px < x + w && px < surface->w; px++) {
            int py = y + i;
            if (py >= 0 && py < surface->h && px >= 0) {
                Uint32* pixel = (Uint32*)((Uint8*)surface->pixels + py * surface->pitch + px * 4);
                *pixel = SDL_MapRGB(surface->format, r, g, b);
            }
        }
        
        for (int px = x; px < x + w && px < surface->w; px++) {
            int py = y + h - 1 - i;
            if (py >= 0 && py < surface->h && px >= 0) {
                Uint32* pixel = (Uint32*)((Uint8*)surface->pixels + py * surface->pitch + px * 4);
                *pixel = SDL_MapRGB(surface->format, r, g, b);
            }
        }
        
        for (int py = y; py < y + h && py < surface->h; py++) {
            int px = x + i;
            if (px >= 0 && px < surface->w && py >= 0) {
                Uint32* pixel = (Uint32*)((Uint8*)surface->pixels + py * surface->pitch + px * 4);
                *pixel = SDL_MapRGB(surface->format, r, g, b);
            }
        }
        
        for (int py = y; py < y + h && py < surface->h; py++) {
            int px = x + w - 1 - i;
            if (px >= 0 && px < surface->w && py >= 0) {
                Uint32* pixel = (Uint32*)((Uint8*)surface->pixels + py * surface->pitch + px * 4);
                *pixel = SDL_MapRGB(surface->format, r, g, b);
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
    // Rien à libérer
}

void free_word_list(WordList* list) {
    if (list && list->words) {
        for (int i = 0; i < list->count; i++) {
            free(list->words[i]);
        }
        free(list->words);
    }
}

