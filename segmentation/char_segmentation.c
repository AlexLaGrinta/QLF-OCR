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
// QUEUE FOR BFS
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

static bool is_empty(Queue* q) {
    return q->front >= q->rear;
}

// ============================================================================
// BFS FOR COMPONENT DETECTION
// ============================================================================

static void bfs_component(BinaryImage* bin, bool* visited, int start_x, int start_y, 
                         Component* comp, Queue* q) {
    comp->min_x = start_x;
    comp->max_x = start_x;
    comp->min_y = start_y;
    comp->max_y = start_y;
    comp->pixel_count = 0;
    comp->valid = true;
    
    q->front = 0;
    q->rear = 0;
    enqueue(q, start_x, start_y);
    visited[start_y * bin->width + start_x] = true;
    
    int dx[] = {-1, 1, 0, 0, -1, -1, 1, 1};
    int dy[] = {0, 0, -1, 1, -1, 1, -1, 1};
    
    while (!is_empty(q)) {
        int x, y;
        dequeue(q, &x, &y);
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
                    enqueue(q, nx, ny);
                }
            }
        }
    }
}

Character* find_characters_in_grid(BinaryImage* bin, int* num_chars, 
                                   int offset_x, int offset_y) {
    if (!bin || !num_chars) return NULL;

    bool* visited = calloc(bin->width * bin->height, sizeof(bool));
    if (!visited) return NULL;

    Component* temp_comps = malloc(1000 * sizeof(Component));
    if (!temp_comps) {
        free(visited);
        return NULL;
    }

    int comp_count = 0;
    Queue* q = create_queue(bin->width * bin->height);
    if (!q) {
        free(visited);
        free(temp_comps);
        return NULL;
    }

    for (int y = 0; y < bin->height; y++) {
        for (int x = 0; x < bin->width; x++) {
            int idx = y * bin->width + x;

            if (!visited[idx] && bin->data[idx] == 0) {
                if (comp_count >= 1000) break;

                Component comp;
                bfs_component(bin, visited, x, y, &comp, q);

                temp_comps[comp_count++] = comp;
            }
        }
    }

    free_queue(q);

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
// ASSIGNMENT OF ZONES
// ============================================================================

void assign_character_zones(Character* chars, int num_chars, 
                            GridBounds grid_bounds, GridBounds list_bounds) {
    if (!chars) return;

    int grid_count = 0;
    int list_count = 0;

    for (int i = 0; i < num_chars; i++) {
        int cx = chars[i].x + chars[i].width / 2;
        int cy = chars[i].y + chars[i].height / 2;

        bool in_grid = (cx >= grid_bounds.x && 
                       cx < grid_bounds.x + grid_bounds.width &&
                       cy >= grid_bounds.y && 
                       cy < grid_bounds.y + grid_bounds.height);

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
}


// ============================================================================
// FILTRAGE DES LETTRES (SUPPRESSION DES PARASITES) - VERSION ASSOUPLIE
// ============================================================================

Character* filter_letters(Character* chars, int* num_chars) {
    if (!chars || *num_chars == 0) return chars;

    printf("\n🔍 Filtering characters (removing noise)...\n");
    printf("  Initial count: %d characters\n", *num_chars);

    // ========================================================================
    // ÉTAPE 1: Calculer les statistiques globales
    // ========================================================================
    
    float total_width = 0, total_height = 0, total_area = 0;
    int valid_for_stats = 0;

    // Première passe : calculer les moyennes en excluant les valeurs extrêmes
    for (int i = 0; i < *num_chars; i++) {
        int w = chars[i].width;
        int h = chars[i].height;
        int area = w * h;
        
        // Ignorer les très petits et très grands pour les stats
        if (w >= 3 && h >= 5 && w <= 200 && h <= 200 && area >= 15) {
            total_width += w;
            total_height += h;
            total_area += area;
            valid_for_stats++;
        }
    }

    if (valid_for_stats == 0) {
        printf("  ⚠ No valid characters for statistics\n");
        return chars;
    }

    float avg_width = total_width / valid_for_stats;
    float avg_height = total_height / valid_for_stats;
    float avg_area = total_area / valid_for_stats;

    printf("  Statistics: avg_width=%.1f, avg_height=%.1f, avg_area=%.1f\n",
           avg_width, avg_height, avg_area);

    // ========================================================================
    // ÉTAPE 2: Calculer l'écart-type pour détecter les outliers
    // ========================================================================
    
    float var_width = 0, var_height = 0, var_area = 0;
    
    for (int i = 0; i < *num_chars; i++) {
        int w = chars[i].width;
        int h = chars[i].height;
        int area = w * h;
        
        if (w >= 3 && h >= 5 && w <= 200 && h <= 200 && area >= 15) {
            var_width += (w - avg_width) * (w - avg_width);
            var_height += (h - avg_height) * (h - avg_height);
            var_area += (area - avg_area) * (area - avg_area);
        }
    }
    
    float std_width = sqrtf(var_width / valid_for_stats);
    float std_height = sqrtf(var_height / valid_for_stats);
    float std_area = sqrtf(var_area / valid_for_stats);

    printf("  Std deviation: width=%.1f, height=%.1f, area=%.1f\n",
           std_width, std_height, std_area);

    // ========================================================================
    // ÉTAPE 3: Fonction de calcul de densité (remplissage)
    // ========================================================================
    
    auto float calculate_density(Character* ch) {
        if (!ch->data || ch->width == 0 || ch->height == 0) return 0.0f;
        
        int black_pixels = 0;
        int total_pixels = ch->width * ch->height;
        
        for (int i = 0; i < total_pixels; i++) {
            if (ch->data[i] < 128) {  // Pixel sombre
                black_pixels++;
            }
        }
        
        return (float)black_pixels / total_pixels;
    }

    // ========================================================================
    // ÉTAPE 4: Fonction de détection de composante connectée principale
    // ========================================================================
    
    auto int count_connected_components(Character* ch) {
        if (!ch->data || ch->width == 0 || ch->height == 0) return 0;
        
        int w = ch->width;
        int h = ch->height;
        bool* visited = calloc(w * h, sizeof(bool));
        int num_components = 0;
        
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                int idx = y * w + x;
                
                if (ch->data[idx] < 128 && !visited[idx]) {
                    // Nouvelle composante trouvée
                    num_components++;
                    
                    // BFS simple pour marquer la composante
                    int* queue_x = malloc(w * h * sizeof(int));
                    int* queue_y = malloc(w * h * sizeof(int));
                    int front = 0, rear = 0;
                    
                    queue_x[rear] = x;
                    queue_y[rear] = y;
                    rear++;
                    visited[idx] = true;
                    
                    while (front < rear) {
                        int cx = queue_x[front];
                        int cy = queue_y[front];
                        front++;
                        
                        // Vérifier les 8 voisins (connectivité 8)
                        int dx[] = {-1, 1, 0, 0, -1, -1, 1, 1};
                        int dy[] = {0, 0, -1, 1, -1, 1, -1, 1};
                        
                        for (int d = 0; d < 8; d++) {
                            int nx = cx + dx[d];
                            int ny = cy + dy[d];
                            
                            if (nx >= 0 && nx < w && ny >= 0 && ny < h) {
                                int nidx = ny * w + nx;
                                
                                if (ch->data[nidx] < 128 && !visited[nidx]) {
                                    visited[nidx] = true;
                                    queue_x[rear] = nx;
                                    queue_y[rear] = ny;
                                    rear++;
                                }
                            }
                        }
                    }
                    
                    free(queue_x);
                    free(queue_y);
                }
            }
        }
        
        free(visited);
        return num_components;
    }

    // ========================================================================
    // ÉTAPE 5: Filtrage avec critères multiples ASSOUPLIS
    // ========================================================================
    
    int kept = 0;
    int filtered_size = 0;
    int filtered_aspect = 0;
    int filtered_density = 0;
    int filtered_components = 0;
    int filtered_outlier = 0;

    for (int i = 0; i < *num_chars; i++) {
        bool valid = true;
        int w = chars[i].width;
        int h = chars[i].height;
        int area = w * h;
        float aspect = (h > 0) ? (float)w / h : 0;

        // ----------------------------------------------------------------
        // CRITÈRE 1: Taille minimale et maximale absolue (ASSOUPLI)
        // ----------------------------------------------------------------
        if (w < 2 || h < 5) {  // Permet les I très fins (2px de large min)
            valid = false;
            filtered_size++;
        }
        
        if (w > 200 || h > 200) {
            valid = false;
            filtered_size++;
        }

        // ----------------------------------------------------------------
        // CRITÈRE 2: Surface minimale et maximale (ASSOUPLI)
        // ----------------------------------------------------------------
        if (area < 12) {  // Réduit de 20 à 12 pour accepter les I fins
            valid = false;
            filtered_size++;
        }
        
        if (area > 15000) {  // Augmenté pour être moins strict
            valid = false;
            filtered_size++;
        }

        // ----------------------------------------------------------------
        // CRITÈRE 3: Ratio hauteur/largeur (TRÈS ASSOUPLI pour I)
        // ----------------------------------------------------------------
        if (aspect > 0) {
            // Accepte des ratios très extrêmes pour lettres comme I, l, i
            if (aspect < 0.08 || aspect > 10.0) {  // Était 0.15/6.0
                valid = false;
                filtered_aspect++;
            }
        }

        // ----------------------------------------------------------------
        // CRITÈRE 4: Outliers statistiques (ASSOUPLI)
        // ----------------------------------------------------------------
        if (valid && valid_for_stats > 10) {
            float z_width = fabsf(w - avg_width) / (std_width + 1);
            float z_height = fabsf(h - avg_height) / (std_height + 1);
            float z_area = fabsf(area - avg_area) / (std_area + 1);
            
            // Tolérance augmentée à 4.5 écarts-types
            if (z_width > 4.5 || z_height > 4.5 || z_area > 5.0) {
                valid = false;
                filtered_outlier++;
            }
        }

        // ----------------------------------------------------------------
        // CRITÈRE 5: Densité de pixels (ASSOUPLI)
        // ----------------------------------------------------------------
        if (valid && chars[i].data) {
            float density = calculate_density(&chars[i]);
            
            // Plage élargie : 0.05 à 0.98 (au lieu de 0.08 à 0.95)
            if (density < 0.05 || density > 0.98) {
                valid = false;
                filtered_density++;
            }
        }

        // ----------------------------------------------------------------
        // CRITÈRE 6: Nombre de composantes connectées (ASSOUPLI)
        // ----------------------------------------------------------------
        if (valid && chars[i].data) {
            int num_comp = count_connected_components(&chars[i]);
            
            // Accepte jusqu'à 5 composantes (i, j, %, etc.)
            if (num_comp > 5 || num_comp == 0) {
                valid = false;
                filtered_components++;
            }
        }

        // ----------------------------------------------------------------
        // CRITÈRE BONUS: Détection spéciale pour lettres fines (I, l, i, 1)
        // ----------------------------------------------------------------
        if (!valid && w >= 2 && h >= 8) {
            float thin_aspect = (float)h / w;
            
            // Si très fin et vertical (rapport h/w > 3)
            if (thin_aspect > 3.0 && area >= 12) {
                // Vérifier si c'est une forme verticale cohérente
                if (chars[i].data) {
                    float density = calculate_density(&chars[i]);
                    int num_comp = count_connected_components(&chars[i]);
                    
                    // Accepter si densité raisonnable et peu de composantes
                    if (density >= 0.15 && density <= 0.90 && num_comp <= 3) {
                        valid = true;  // Récupérer ce caractère
                        filtered_size--;  // Corriger les stats
                    }
                }
            }
        }

        // ----------------------------------------------------------------
        // Garder ou rejeter le caractère
        // ----------------------------------------------------------------
        if (valid) {
            if (kept != i) {
                chars[kept] = chars[i];
            }
            kept++;
        } else {
            // Libérer la mémoire du caractère filtré
            free(chars[i].data);
        }
    }

    *num_chars = kept;

    // ========================================================================
    // RAPPORT DE FILTRAGE
    // ========================================================================
    
    int total_filtered = filtered_size + filtered_aspect + filtered_density + 
                         filtered_components + filtered_outlier;
    
    printf("\n  Filtering results:\n");
    printf("    ✓ Kept: %d characters\n", kept);
    printf("    ✗ Filtered: %d characters\n", total_filtered);
    
    if (total_filtered > 0) {
        printf("      - Size issues: %d\n", filtered_size);
        printf("      - Aspect ratio: %d\n", filtered_aspect);
        printf("      - Density: %d\n", filtered_density);
        printf("      - Components: %d\n", filtered_components);
        printf("      - Outliers: %d\n", filtered_outlier);
    }

    return realloc(chars, kept * sizeof(Character));
}

// ============================================================================
// GROUPING CHARACTERS INTO WORDS
// ============================================================================

static int compare_chars_by_position(const void* a, const void* b) {
    Character* c1 = (Character*)a;
    Character* c2 = (Character*)b;
    
    // D'abord trier par ligne (y), puis par colonne (x)
    int y_diff = c1->y - c2->y;
    if (abs(y_diff) > 20) { // Tolérance pour la même ligne
        return y_diff;
    }
    return c1->x - c2->x;
}

static float distance_between_chars(Character* a, Character* b) {
    int dx = (a->x + a->width/2) - (b->x + b->width/2);
    int dy = (a->y + a->height/2) - (b->y + b->height/2);
    return sqrtf(dx * dx + dy * dy);
}

Word* group_characters_into_words(Character* chars, int num_chars, int* num_words) {
    if (!chars || num_chars == 0 || !num_words) {
        *num_words = 0;
        return NULL;
    }

    // Séparer les caractères par zone
    int grid_count = 0, list_count = 0;
    for (int i = 0; i < num_chars; i++) {
        if (chars[i].zone == ZONE_GRID) grid_count++;
        else if (chars[i].zone == ZONE_WORDLIST) list_count++;
    }

    Character* grid_chars = malloc(grid_count * sizeof(Character));
    Character* list_chars = malloc(list_count * sizeof(Character));
    
    int g_idx = 0, l_idx = 0;
    for (int i = 0; i < num_chars; i++) {
        if (chars[i].zone == ZONE_GRID) {
            grid_chars[g_idx++] = chars[i];
        } else if (chars[i].zone == ZONE_WORDLIST) {
            list_chars[l_idx++] = chars[i];
        }
    }

    // Trier les caractères par position
    qsort(grid_chars, grid_count, sizeof(Character), compare_chars_by_position);
    qsort(list_chars, list_count, sizeof(Character), compare_chars_by_position);

    // Allouer de l'espace pour les mots
    Word* words = malloc((grid_count + list_count) * sizeof(Word));
    int word_count = 0;

    // ========================================================================
    // Grouper les caractères de la GRILLE en mots
    // ========================================================================
    if (grid_count > 0) {
        // Calculer l'espacement moyen entre caractères
        float total_spacing = 0;
        int spacing_count = 0;
        
        for (int i = 1; i < grid_count; i++) {
            if (abs(grid_chars[i].y - grid_chars[i-1].y) < 20) {
                int spacing = grid_chars[i].x - (grid_chars[i-1].x + grid_chars[i-1].width);
                if (spacing > 0 && spacing < 100) {
                    total_spacing += spacing;
                    spacing_count++;
                }
            }
        }
        
        float avg_spacing = (spacing_count > 0) ? (total_spacing / spacing_count) : 15.0f;
        float word_threshold = avg_spacing * 2.5f; // Espace entre mots > 2.5x l'espace entre lettres
        
        if (word_threshold < 20) word_threshold = 20;

        // Créer les mots de la grille
        int current_word_start = 0;
        
        for (int i = 1; i <= grid_count; i++) {
            bool new_word = false;
            
            if (i == grid_count) {
                new_word = true;
            } else {
                int y_diff = abs(grid_chars[i].y - grid_chars[i-1].y);
                int x_spacing = grid_chars[i].x - (grid_chars[i-1].x + grid_chars[i-1].width);
                
                // Nouvelle ligne ou grand espace horizontal
                if (y_diff > 20 || x_spacing > word_threshold) {
                    new_word = true;
                }
            }
            
            if (new_word) {
                int word_len = i - current_word_start;
                
                if (word_len > 0) {
                    words[word_count].word_id = word_count;
                    words[word_count].num_letters = word_len;
                    words[word_count].letters = malloc(word_len * sizeof(Character));
                    
                    int min_x = INT_MAX, min_y = INT_MAX;
                    int max_x = 0, max_y = 0;
                    
                    for (int j = 0; j < word_len; j++) {
                        words[word_count].letters[j] = grid_chars[current_word_start + j];
                        
                        Character* ch = &grid_chars[current_word_start + j];
                        if (ch->x < min_x) min_x = ch->x;
                        if (ch->y < min_y) min_y = ch->y;
                        if (ch->x + ch->width > max_x) max_x = ch->x + ch->width;
                        if (ch->y + ch->height > max_y) max_y = ch->y + ch->height;
                    }
                    
                    words[word_count].x = min_x;
                    words[word_count].y = min_y;
                    words[word_count].width = max_x - min_x;
                    words[word_count].height = max_y - min_y;
                    
                    word_count++;
                }
                
                current_word_start = i;
            }
        }
    }

    // ========================================================================
    // Grouper les caractères de la LISTE en mots
    // ========================================================================
    if (list_count > 0) {
        // Pour la liste de mots, supposer qu'il y a un mot par ligne
        int current_word_start = 0;
        
        for (int i = 1; i <= list_count; i++) {
            bool new_word = false;
            
            if (i == list_count) {
                new_word = true;
            } else {
                int y_diff = abs(list_chars[i].y - list_chars[i-1].y);
                
                // Nouvelle ligne = nouveau mot
                if (y_diff > 15) {
                    new_word = true;
                }
            }
            
            if (new_word) {
                int word_len = i - current_word_start;
                
                if (word_len > 0) {
                    words[word_count].word_id = word_count;
                    words[word_count].num_letters = word_len;
                    words[word_count].letters = malloc(word_len * sizeof(Character));
                    
                    int min_x = INT_MAX, min_y = INT_MAX;
                    int max_x = 0, max_y = 0;
                    
                    for (int j = 0; j < word_len; j++) {
                        words[word_count].letters[j] = list_chars[current_word_start + j];
                        
                        Character* ch = &list_chars[current_word_start + j];
                        if (ch->x < min_x) min_x = ch->x;
                        if (ch->y < min_y) min_y = ch->y;
                        if (ch->x + ch->width > max_x) max_x = ch->x + ch->width;
                        if (ch->y + ch->height > max_y) max_y = ch->y + ch->height;
                    }
                    
                    words[word_count].x = min_x;
                    words[word_count].y = min_y;
                    words[word_count].width = max_x - min_x;
                    words[word_count].height = max_y - min_y;
                    
                    word_count++;
                }
                
                current_word_start = i;
            }
        }
    }

    free(grid_chars);
    free(list_chars);

    *num_words = word_count;
    return words;
}

void free_words(Word* words, int num_words) {
    if (!words) return;
    
    for (int i = 0; i < num_words; i++) {
        free(words[i].letters);
    }
    free(words);
}


// ============================================================================
// SAVE BMP
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

