#include "image_segmentation.h"
#include "../Preprocessing/preprocessing.h"
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// STRUCTURES
// ============================================================================

typedef struct {
  int min_x, max_x;
  int min_y, max_y;
  int center_x, center_y;
  int pixel_count;
  int cluster_id;
  bool is_grid;
  bool is_wordlist;
} Component;

typedef struct {
  Component *components;
  int count;
  int capacity;
} ComponentList;

typedef struct {
  int cluster_id;
  int letter_count;
  float density;
  float compactness;
  float vertical_alignment;
  int num_columns;
  GridBounds bounds;
} WordListCandidate;

typedef struct {
  int x;
  int count;
} ColumnInfo;

// ============================================================================
// CREATION OF A BINARY IMAGE
// ============================================================================

BinaryImage *create_binary_image_from_surface(SDL_Surface *surface) {
  if (!surface)
    return NULL;

  BinaryImage *bin = malloc(sizeof(BinaryImage));
  if (!bin)
    return NULL;

  bin->width = surface->w;
  bin->height = surface->h;
  bin->data = malloc(bin->width * bin->height);

  if (!bin->data) {
    free(bin);
    return NULL;
  }

  SDL_Surface *rgb_surface =
      SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGB24, 0);
  if (!rgb_surface) {
    free(bin->data);
    free(bin);
    return NULL;
  }

  SDL_LockSurface(rgb_surface);
  uint8_t *pixels = (uint8_t *)rgb_surface->pixels;

  // Calculate Otsu threshold for the rotated/processed image
  uint8_t th = otsu_threshold(rgb_surface);
  if (th < 10)
    th = 128; // Fallback if Otsu fails or image is weird
  printf("  Segments Otsu threshold = %d\n", th);

  for (int y = 0; y < bin->height; y++) {
    for (int x = 0; x < bin->width; x++) {
      int idx = y * rgb_surface->pitch + x * 3;
      uint8_t r = pixels[idx];
      uint8_t g = pixels[idx + 1];
      uint8_t b = pixels[idx + 2];

      uint8_t luminance = (uint8_t)(0.299 * r + 0.587 * g + 0.114 * b);
      bin->data[y * bin->width + x] = (luminance < th) ? 0 : 255;
    }
  }

  SDL_UnlockSurface(rgb_surface);
  SDL_FreeSurface(rgb_surface);

  return bin;
}

void free_binary_image(BinaryImage *bin) {
  if (bin) {
    free(bin->data);
    free(bin);
  }
}

// ============================================================================
// MANAGEMENT OF COMPONENT LISTS
// ============================================================================

static ComponentList *create_component_list(int initial_capacity) {
  ComponentList *list = malloc(sizeof(ComponentList));
  if (!list)
    return NULL;

  list->components = malloc(initial_capacity * sizeof(Component));
  if (!list->components) {
    free(list);
    return NULL;
  }

  list->count = 0;
  list->capacity = initial_capacity;
  return list;
}

static void add_component(ComponentList *list, Component comp) {
  if (list->count >= list->capacity) {
    list->capacity *= 2;
    list->components =
        realloc(list->components, list->capacity * sizeof(Component));
  }
  list->components[list->count++] = comp;
}

static void free_component_list(ComponentList *list) {
  if (list) {
    free(list->components);
    free(list);
  }
}

// ============================================================================
// EXTRACTION OF COMPONENTS (BFS)
// ============================================================================

static void extract_component_bfs(BinaryImage *bin, bool *visited, int start_x,
                                  int start_y, Component *comp) {
  comp->min_x = start_x;
  comp->max_x = start_x;
  comp->min_y = start_y;
  comp->max_y = start_y;
  comp->pixel_count = 0;
  comp->cluster_id = -1;
  comp->is_grid = false;
  comp->is_wordlist = false;

  int capacity = 1000;
  int *qx = malloc(capacity * sizeof(int));
  int *qy = malloc(capacity * sizeof(int));
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
    if (x < comp->min_x)
      comp->min_x = x;
    if (x > comp->max_x)
      comp->max_x = x;
    if (y < comp->min_y)
      comp->min_y = y;
    if (y > comp->max_y)
      comp->max_y = y;

    for (int d = 0; d < 8; d++) {
      int nx = x + dx[d];
      int ny = y + dy[d];

      if (nx >= 0 && nx < bin->width && ny >= 0 && ny < bin->height) {
        int idx = ny * bin->width + nx;

        if (!visited[idx] && bin->data[idx] == 0) {
          visited[idx] = true;

          if (rear >= capacity) {
            capacity *= 2;
            qx = realloc(qx, capacity * sizeof(int));
            qy = realloc(qy, capacity * sizeof(int));
          }

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

static ComponentList *find_all_components(BinaryImage *bin) {
  bool *visited = calloc(bin->width * bin->height, sizeof(bool));
  if (!visited)
    return NULL;

  ComponentList *list = create_component_list(500);
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

        if (w >= 2 && h >= 2 && w <= 200 && h <= 200 && comp.pixel_count >= 3) {
          add_component(list, comp);
        }
      }
    }
  }

  free(visited);
  return list;
}

// ============================================================================
// DBSCAN CLUSTERING
// ============================================================================

static float distance_between_components(Component *a, Component *b) {
  int dx = a->center_x - b->center_x;
  int dy = a->center_y - b->center_y;
  return sqrtf(dx * dx + dy * dy);
}

static int dbscan_cluster(ComponentList *comps, int *indices, int count,
                          float eps, int min_pts, int start_cluster_id) {
  if (count == 0)
    return 0;

  bool *visited = calloc(count, sizeof(bool));
  int cluster_id = start_cluster_id;

  for (int i = 0; i < count; i++) {
    if (visited[i])
      continue;

    visited[i] = true;
    int *neighbors = malloc(count * sizeof(int));
    int neighbor_count = 0;

    for (int j = 0; j < count; j++) {
      if (i == j)
        continue;
      float dist = distance_between_components(&comps->components[indices[i]],
                                               &comps->components[indices[j]]);
      if (dist <= eps) {
        neighbors[neighbor_count++] = j;
      }
    }

    if (neighbor_count < min_pts) {
      comps->components[indices[i]].cluster_id = -1;
      free(neighbors);
      continue;
    }

    comps->components[indices[i]].cluster_id = cluster_id;

    int n_idx = 0;
    while (n_idx < neighbor_count) {
      int neighbor_local_idx = neighbors[n_idx++];
      int neighbor_idx = indices[neighbor_local_idx];

      if (!visited[neighbor_local_idx]) {
        visited[neighbor_local_idx] = true;

        int *new_neighbors = malloc(count * sizeof(int));
        int new_count = 0;

        for (int j = 0; j < count; j++) {
          if (neighbor_local_idx == j)
            continue;
          float dist = distance_between_components(
              &comps->components[neighbor_idx], &comps->components[indices[j]]);
          if (dist <= eps) {
            new_neighbors[new_count++] = j;
          }
        }

        if (new_count >= min_pts) {
          for (int k = 0; k < new_count; k++) {
            bool already_in = false;
            for (int m = 0; m < neighbor_count; m++) {
              if (neighbors[m] == new_neighbors[k]) {
                already_in = true;
                break;
              }
            }
            if (!already_in) {
              neighbors[neighbor_count++] = new_neighbors[k];
            }
          }
        }

        free(new_neighbors);
        comps->components[neighbor_idx].cluster_id = cluster_id;

        for (int j = 0; j < count; j++) {
          if (j != neighbor_local_idx) {
            float dist =
                distance_between_components(&comps->components[neighbor_idx],
                                            &comps->components[indices[j]]);
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
  return cluster_id - start_cluster_id;
}

// ============================================================================
// GRID ANALYSIS
// ============================================================================

static int compare_by_y(const void *a, const void *b) {
  Component *c1 = (Component *)a;
  Component *c2 = (Component *)b;
  return c1->center_y - c2->center_y;
}

static float analyze_grid_structure(ComponentList *comps, int cluster_id) {
  int count = 0;
  for (int i = 0; i < comps->count; i++) {
    if (comps->components[i].cluster_id == cluster_id) {
      count++;
    }
  }

  if (count < 20)
    return 0.0f;

  Component *cluster_comps = malloc(count * sizeof(Component));
  int idx = 0;
  for (int i = 0; i < comps->count; i++) {
    if (comps->components[i].cluster_id == cluster_id) {
      cluster_comps[idx++] = comps->components[i];
    }
  }

  qsort(cluster_comps, count, sizeof(Component), compare_by_y);

  typedef struct {
    int y;
    int count;
    int min_x, max_x;
  } Line;

  Line *lines = malloc(count * sizeof(Line));
  int num_lines = 0;
  int current_y = cluster_comps[0].center_y;
  int current_count = 1;
  int current_min_x = cluster_comps[0].center_x;
  int current_max_x = cluster_comps[0].center_x;

  for (int i = 1; i < count; i++) {
    if (abs(cluster_comps[i].center_y - current_y) <
        30) { // Increased from 15 to 30 for rotated lines
      current_count++;
      current_y =
          (current_y * (current_count - 1) + cluster_comps[i].center_y) /
          current_count;
      if (cluster_comps[i].center_x < current_min_x)
        current_min_x = cluster_comps[i].center_x;
      if (cluster_comps[i].center_x > current_max_x)
        current_max_x = cluster_comps[i].center_x;
    } else {
      lines[num_lines].y = current_y;
      lines[num_lines].count = current_count;
      lines[num_lines].min_x = current_min_x;
      lines[num_lines].max_x = current_max_x;
      num_lines++;

      current_y = cluster_comps[i].center_y;
      current_count = 1;
      current_min_x = cluster_comps[i].center_x;
      current_max_x = cluster_comps[i].center_x;
    }
  }
  lines[num_lines].y = current_y;
  lines[num_lines].count = current_count;
  lines[num_lines].min_x = current_min_x;
  lines[num_lines].max_x = current_max_x;
  num_lines++;

  if (num_lines < 3) {
    free(cluster_comps);
    free(lines);
    return 0.0f;
  }

  // FORCE ACCEPT LARGE CLUSTERS (Level 2 Robustness)
  // If we have > 10 lines and > 150 letters, it's definitely the grid.
  int total_letters_check = 0;
  for (int i = 0; i < num_lines; i++)
    total_letters_check += lines[i].count;

  if (num_lines >= 10 &&
      total_letters_check >
          80) { // Lowered to 80 for pre-rotation pipeline robustness
    free(cluster_comps);
    free(lines);
    return 1.0f;
  }

  int min_letters = lines[0].count;
  int max_letters = lines[0].count;
  float total_letters = 0;

  for (int i = 0; i < num_lines; i++) {
    if (lines[i].count < min_letters)
      min_letters = lines[i].count;
    if (lines[i].count > max_letters)
      max_letters = lines[i].count;
    total_letters += lines[i].count;
  }

  float avg_letters = total_letters / num_lines;
  float letter_regularity = 1.0f - ((max_letters - min_letters) / avg_letters);

  float total_v_spacing = 0;
  for (int i = 1; i < num_lines; i++) {
    total_v_spacing += lines[i].y - lines[i - 1].y;
  }
  float avg_v_spacing = total_v_spacing / (num_lines - 1);

  float v_variance = 0;
  for (int i = 1; i < num_lines; i++) {
    float diff = (lines[i].y - lines[i - 1].y) - avg_v_spacing;
    v_variance += diff * diff;
  }
  float v_std = sqrtf(v_variance / (num_lines - 1));
  float spacing_regularity = 1.0f - (v_std / avg_v_spacing);
  if (spacing_regularity < 0)
    spacing_regularity = 0;

  float regularity = (letter_regularity * 0.6f + spacing_regularity * 0.4f);

  free(cluster_comps);
  free(lines);

  return regularity;
}

// ============================================================================
// ANALYSIS LIST OF WORDS - LOW DENSITY
// ============================================================================

// ============================================================================
// DÉTECTION MAIN ZONES
// ============================================================================

int detect_major_zones_improved(BinaryImage *bin, GridBounds *grid_bounds,
                                GridBounds *list_bounds) {
  if (!bin || !grid_bounds || !list_bounds)
    return 0;

  ComponentList *comps = find_all_components(bin);
  if (!comps || comps->count < 10) {
    printf("  ✗ Not enough components found\n");
    if (comps)
      free_component_list(comps);
    return 0;
  }

  float total_dist = 0;
  int dist_samples = fminf(50, comps->count);

  for (int i = 0; i < dist_samples; i++) {
    float min_dist = FLT_MAX;
    for (int j = 0; j < comps->count; j++) {
      if (i != j) {
        float dist = distance_between_components(&comps->components[i],
                                                 &comps->components[j]);
        if (dist < min_dist)
          min_dist = dist;
      }
    }
    total_dist += min_dist;
  }

  // ADAPTIVE 2-PASS GRID DETECTION
  // Pass 0: Strict EPS (60) to separate close zones (Level 2 Image 2)
  // Pass 1: Relaxed EPS (120) to connect fragmented/rotated grids (Level 2
  // Image 1)

  int best_cluster = -1;
  float best_regularity = 0;
  int num_clusters = 0;

  int *all_indices = malloc(comps->count * sizeof(int));
  for (int i = 0; i < comps->count; i++) {
    all_indices[i] = i;
  }

  for (int pass = 0; pass < 2; pass++) {
    // 1. Reset Clusters
    for (int i = 0; i < comps->count; i++)
      comps->components[i].cluster_id = -1;

    // 2. Set EPS
    float current_eps = 60.0f;
    if (pass == 1) {
      current_eps = 120.0f;
    } else {
      // Use calculated but cap strictly
      current_eps = (total_dist / dist_samples) * 2.0f;
      if (current_eps < 30)
        current_eps = 30;
      if (current_eps > 60)
        current_eps = 60; // Strict cap
    }

    printf("  🔍 Grid Detection Pass %d: EPS=%.1f\n", pass, current_eps);

    // 3. Run Clustering
    num_clusters =
        dbscan_cluster(comps, all_indices, comps->count, current_eps, 3, 0);

    // 4. Analyze Clusters
    int pass_best_cluster = -1;
    float pass_best_reg = 0;

    for (int c = 0; c < num_clusters; c++) {
      float regularity = analyze_grid_structure(comps, c);

      if (regularity > pass_best_reg) {
        pass_best_reg = regularity;
        pass_best_cluster = c;
      }
    }

    // Decision
    if (pass == 0) {
      if (pass_best_reg > 0.6f) {
        // Found a great grid, stop here
        best_cluster = pass_best_cluster;
        best_regularity = pass_best_reg;
        printf("    ✓ High quality grid found significantly (Reg=%.2f). "
               "Stopping.\n",
               pass_best_reg);
        break;
      } else {
        printf("    ✗ No high quality grid in Pass 0 (Best=%.2f). Trying "
               "relaxed pass.\n",
               pass_best_reg);
      }
    } else {
      // Pass 1: Take whatever is best if valid
      if (pass_best_reg > 0.3f) { // 0.3 matches the lowered threshold
        best_cluster = pass_best_cluster;
        best_regularity = pass_best_reg;
      }
    }
  }

  free(all_indices);

  // Re-run analyze to get bounds? No, we need to extract it now.
  // Actually, wait. We need the cluster IDs to be set to the *winning* pass
  // state. If use Pass 0, comps are set. If failed Pass 0, comps are
  // overwritten by Pass 1. So the loop structure works naturally: comps will
  // have Pass 1 IDs if we run Pass 1. BUT: if we break from Pass 0, comps has
  // Pass 0 IDs. Perfect.

  if (best_cluster != -1) {
    // Recalculate component list for bounds (logic below expects best_cluster
    // to exist) We just need to ensure `best_cluster` index matches the current
    // `comps` state. Since `dbscan_cluster` resets numbering from 0 each run,
    // `c` indices match.
  }

  // Resume original logic...
  // Just need to ensure the variables below map correctly.

  if (best_cluster >= 0) {
    // Logic to extract bounds... using best_cluster ID
    // ... (Original logic continues)

    // We need to re-extract the components because previous code did it inside
    // loop? No, previous code did NOT extract bounds inside the search loop. It
    // did:
    /*
    for (int c = 0; c < num_clusters; c++) {
       if (reg > best) {
           best_cluster = c;
           // calculate best_rows (removed)
           // allocate cluster_comps (freed)
       }
    }
    if (best_cluster >= 0) { extract bounds }
    */

    // So my structure determines best_cluster.
    // Logic below loop extracts bounds.
    // Correct.
  }

  int grid_count = 0;
  if (best_cluster >= 0) {

    int min_x = INT_MAX, max_x = 0;
    int min_y = INT_MAX, max_y = 0;

    for (int i = 0; i < comps->count; i++) {
      if (comps->components[i].cluster_id == best_cluster) {
        comps->components[i].is_grid = true;
        grid_count++;

        Component *c = &comps->components[i];
        if (c->min_x < min_x)
          min_x = c->min_x;
        if (c->max_x > max_x)
          max_x = c->max_x;
        if (c->min_y < min_y)
          min_y = c->min_y;
        if (c->max_y > max_y)
          max_y = c->max_y;
      }
    }

    grid_bounds->x = min_x - 10;
    grid_bounds->y = min_y - 10;
    grid_bounds->width = max_x - min_x + 20;
    grid_bounds->height = max_y - min_y + 20;
    grid_bounds->valid = true;

  } else {
    printf("\n  ✗ No grid structure detected\n");
    grid_bounds->valid = false;
  }

  int remaining_count = 0;
  for (int i = 0; i < comps->count; i++) {
    if (!comps->components[i].is_grid) {
      remaining_count++;
    }
  }

  if (remaining_count < 3) {
    printf("     ✗ Not enough letters for word list\n");
    list_bounds->valid = false;
    free_component_list(comps);
    return grid_bounds->valid ? 1 : 0;
  }

  int *remaining_indices = malloc(remaining_count * sizeof(int));
  int idx = 0;
  int grid_top_y = grid_bounds->valid ? grid_bounds->y : INT_MAX;

  for (int i = 0; i < comps->count; i++) {
    if (!comps->components[i].is_grid) {
      if (comps->components[i].center_y < grid_top_y) {
        remaining_indices[idx++] = i;
      }
    }
  }

  if (idx < 3) {
    idx = 0;
    for (int i = 0; i < comps->count; i++) {
      if (!comps->components[i].is_grid) {
        remaining_indices[idx++] = i;
      }
    }
  }

  remaining_count = idx;

  total_dist = 0;
  dist_samples = fminf(20, remaining_count);

  for (int i = 0; i < dist_samples && i < remaining_count; i++) {
    float min_dist = FLT_MAX;
    for (int j = 0; j < remaining_count; j++) {
      if (i != j) {
        float dist = distance_between_components(
            &comps->components[remaining_indices[i]],
            &comps->components[remaining_indices[j]]);
        if (dist < min_dist)
          min_dist = dist;
      }
    }
    total_dist += min_dist;
  }

  float eps_wordlist =
      (total_dist / dist_samples) * 3.0f; // Increased multiplier
  if (eps_wordlist < 40)
    eps_wordlist = 40;
  if (eps_wordlist > 300)
    eps_wordlist = 300; // Increased max limit

  int num_wordlist_clusters = dbscan_cluster(
      comps, remaining_indices, remaining_count, eps_wordlist, 2, num_clusters);
  free(remaining_indices);

  // ============================================================================
  // CHOOSE ALL SIGNIFICANT CLUSTERS (SIZE BASED)
  // ============================================================================
  if (num_wordlist_clusters > 0) {
    WordListCandidate *candidates =
        malloc(num_wordlist_clusters * sizeof(WordListCandidate));
    int candidate_count = 0;

    for (int c = num_clusters; c < num_clusters + num_wordlist_clusters; c++) {
      int cluster_size = 0;
      for (int i = 0; i < comps->count; i++) {
        if (comps->components[i].cluster_id == c) {
          cluster_size++;
        }
      }

      if (cluster_size >= 3) {
        // Simpler init
        candidates[candidate_count].cluster_id = c;
        candidates[candidate_count].letter_count = cluster_size;
        candidate_count++;
      }
    }

    if (candidate_count > 0) {
      // Find largest cluster
      int max_letters = 0;
      for (int i = 0; i < candidate_count; i++) {
        if (candidates[i].letter_count > max_letters) {
          max_letters = candidates[i].letter_count;
        }
      }

      int selected_count = 0;
      int min_x = INT_MAX, max_x = 0;
      int min_y = INT_MAX, max_y = 0;

      // Threshold: Keep clusters that are substantial relative to the main one,
      // or just absolute size > 5 to catch separated words
      int threshold = 5;
      if (max_letters < 10)
        threshold = 3;

      printf("     🔍 WordList Analysis: Found %d clusters. Max size: %d. "
             "Threshold: %d\n",
             candidate_count, max_letters, threshold);

      int grid_bottom = grid_bounds->y + grid_bounds->height;
      int bottom_margin = 20;

      printf("DEBUG: Grid Bottom = %d. Image Height = %d. Checking for "
             "components below y=%d\n",
             grid_bottom, comps->count > 0 ? 0 : 0,
             grid_bottom + bottom_margin);
      // Oops, I can't access bin->height here easily unless I pass it or infer
      // it. But I can guess max detected y?
      int max_detected_y = 0;
      for (int i = 0; i < comps->count; i++)
        if (comps->components[i].max_y > max_detected_y)
          max_detected_y = comps->components[i].max_y;
      printf("DEBUG: Max Detected Y = %d\n", max_detected_y);

      // FORCE INCLUDE BELOW GRID logic:
      int forced_count = 0; // Declare it here in valid scope

      for (int i = 0; i < comps->count; i++) {
        Component *c = &comps->components[i];

        if (!comps->components[i].is_grid &&
            !comps->components[i].is_wordlist) {
          // Check position: below grid
          if (c->min_y > grid_bottom + bottom_margin) {
            // Check size: significant enough to be text
            int c_width = c->max_x - c->min_x;
            int c_height = c->max_y - c->min_y;

            if (c_width > 3 && c_height > 3 && (c_width * c_height) > 20) {
              c->is_wordlist = true;
              forced_count++;

              // Update global wordlist bounds
              if (list_bounds->width == 0) {
                list_bounds->x = c->min_x - 10;
                list_bounds->y = c->min_y - 10;
                list_bounds->width = c_width + 20;
                list_bounds->height = c_height + 20;
                // Initialize min/max trackers
                min_x = c->min_x;
                max_x = c->max_x;
                min_y = c->min_y;
                max_y = c->max_y;
              } else {
                if (c->min_x < min_x)
                  min_x = c->min_x;
                if (c->min_y < min_y)
                  min_y = c->min_y;
                if (c->max_x > max_x)
                  max_x = c->max_x;
                if (c->max_y > max_y)
                  max_y = c->max_y;

                list_bounds->x = min_x - 10;
                list_bounds->y = min_y - 10;
                list_bounds->width = (max_x - min_x) + 20;
                list_bounds->height = (max_y - min_y) + 20;
              }
            }
          }
        }
      } // Close component loop here properly

      for (int i = 0; i < candidate_count; i++) {
        // Keep if large enough
        if (candidates[i].letter_count >= threshold) {
          selected_count++;

          for (int j = 0; j < comps->count; j++) {
            if (comps->components[j].cluster_id == candidates[i].cluster_id) {
              comps->components[j].is_wordlist = true;
            }
          }
        }
      }

      // ============================================================================
      // STEP 3: META-CLUSTERING & DISTINCT ZONE SELECTION
      // ============================================================================
      // Group all 'is_wordlist' components into spatially distinct
      // Meta-Clusters. Choose the LARGEST Meta-Cluster and discard the rest.

      int *meta_labels = malloc(comps->count * sizeof(int));
      for (int i = 0; i < comps->count; i++)
        meta_labels[i] = -1;

      int meta_cluster_count = 0;
      int best_meta_id = -1;
      int max_meta_letters = 0;

      for (int i = 0; i < comps->count; i++) {
        if (comps->components[i].is_wordlist && meta_labels[i] == -1) {
          // New Meta Cluster Found
          int current_meta_id = meta_cluster_count++;
          meta_labels[i] = current_meta_id;

          // BFS to find all connected components (using permissive distance)
          int current_letters = 1;
          int *queue = malloc(comps->count * sizeof(int));
          int head = 0, tail = 0;
          queue[tail++] = i;

          while (head < tail) {
            int curr_idx = queue[head++];
            Component *c1 = &comps->components[curr_idx];

            for (int j = 0; j < comps->count; j++) {
              if (comps->components[j].is_wordlist && meta_labels[j] == -1) {
                Component *c2 = &comps->components[j];
                // Distance Check (Center to Center or Edge to Edge)
                // Using Edge to Edge approx
                int dist_x = 0;
                if (c2->max_x < c1->min_x)
                  dist_x = c1->min_x - c2->max_x;
                else if (c2->min_x > c1->max_x)
                  dist_x = c2->min_x - c1->max_x;

                int dist_y = 0;
                if (c2->max_y < c1->min_y)
                  dist_y = c1->min_y - c2->max_y;
                else if (c2->min_y > c1->max_y)
                  dist_y = c2->min_y - c1->max_y;

                // Threshold: Relaxed Horizontal (500) to bridge columns. Strict
                // Vertical (150) to separate title.
                if (dist_x < 500 && dist_y < 150) {
                  meta_labels[j] = current_meta_id;
                  queue[tail++] = j;
                  current_letters++;
                }
              }
            }
          }
          free(queue);

          // Check if this is the best cluster
          if (current_letters > max_meta_letters) {
            max_meta_letters = current_letters;
            best_meta_id = current_meta_id;
          }
        }
      }

      // APPLY SELECTION: Keep only the best Meta-Cluster and calculate bounds
      if (best_meta_id != -1) {
        int min_x = INT_MAX, max_x = 0;
        int min_y = INT_MAX, max_y = 0;
        int kept_count = 0;

        for (int i = 0; i < comps->count; i++) {
          if (comps->components[i].is_wordlist) {
            // STRICT CHECK: Overlap with Grid?
            // Ensure no overlap with grid_bounds
            int overlap_grid = 0;
            if (grid_bounds->valid) {
              Component *c = &comps->components[i];
              if (c->max_x > grid_bounds->x &&
                  c->min_x < grid_bounds->x + grid_bounds->width &&
                  c->max_y > grid_bounds->y &&
                  c->min_y < grid_bounds->y + grid_bounds->height) {
                overlap_grid = 1;
              }
            }

            if (meta_labels[i] == best_meta_id && !overlap_grid) {
              // KEEP
              kept_count++;
              if (comps->components[i].min_x < min_x)
                min_x = comps->components[i].min_x;
              if (comps->components[i].max_x > max_x)
                max_x = comps->components[i].max_x;
              if (comps->components[i].min_y < min_y)
                min_y = comps->components[i].min_y;
              if (comps->components[i].max_y > max_y)
                max_y = comps->components[i].max_y;
            } else {
              // DISCARD (Not in best cluster OR overlaps grid)
              comps->components[i].is_wordlist = false;
            }
          }
        }

        if (kept_count > 0) {
          list_bounds->x = min_x - 10;
          list_bounds->y = min_y - 10;
          list_bounds->width = max_x - min_x + 20;
          list_bounds->height = max_y - min_y + 20;
          list_bounds->valid = true;
        } else {
          printf("     ✗ All wordlist candidates filtered (overlap/noise)\n");
          list_bounds->valid = false;
        }

      } else {
        printf("     ✗ No valid word list meta-cluster found\n");
        list_bounds->valid = false;
      }

      free(meta_labels);
    } else {
      // No clusters found initially
    }

    free(candidates);
  } else {
    list_bounds->valid = false;
  }

  int wordlist_count = 0;
  int noise_count = 0;

  for (int i = 0; i < comps->count; i++) {
    if (comps->components[i].is_wordlist) {
      wordlist_count++;
    } else if (!comps->components[i].is_grid) {
      noise_count++;
    }
  }

  free_component_list(comps);
  return (grid_bounds->valid || list_bounds->valid) ? 1 : 0;
}

// ============================================================================
// FONCTIONS LEGACY
// ============================================================================

GridBounds detect_grid_smart(BinaryImage *bin) {
  GridBounds grid = {0}, list = {0};
  detect_major_zones_improved(bin, &grid, &list);
  return grid;
}

GridBounds detect_wordlist_smart(BinaryImage *bin, GridBounds grid) {
  (void)grid;
  GridBounds unused_grid = {0}, list = {0};
  detect_major_zones_improved(bin, &unused_grid, &list);
  return list;
}

// ============================================================================
// UTILITIES
// ============================================================================

void draw_rectangle_color(SDL_Surface *surface, int x, int y, int w, int h,
                          Uint8 r, Uint8 g, Uint8 b) {
  if (!surface)
    return;

  SDL_LockSurface(surface);

  for (int i = 0; i < 3; i++) {
    for (int px = x; px < x + w && px < surface->w; px++) {
      int py = y + i;
      if (py >= 0 && py < surface->h && px >= 0) {
        Uint32 *pixel =
            (Uint32 *)((Uint8 *)surface->pixels + py * surface->pitch + px * 4);
        *pixel = SDL_MapRGB(surface->format, r, g, b);
      }
    }

    for (int px = x; px < x + w && px < surface->w; px++) {
      int py = y + h - 1 - i;
      if (py >= 0 && py < surface->h && px >= 0) {
        Uint32 *pixel =
            (Uint32 *)((Uint8 *)surface->pixels + py * surface->pitch + px * 4);
        *pixel = SDL_MapRGB(surface->format, r, g, b);
      }
    }

    for (int py = y; py < y + h && py < surface->h; py++) {
      int px = x + i;
      if (px >= 0 && px < surface->w && py >= 0) {
        Uint32 *pixel =
            (Uint32 *)((Uint8 *)surface->pixels + py * surface->pitch + px * 4);
        *pixel = SDL_MapRGB(surface->format, r, g, b);
      }
    }

    for (int py = y; py < y + h && py < surface->h; py++) {
      int px = x + w - 1 - i;
      if (px >= 0 && px < surface->w && py >= 0) {
        Uint32 *pixel =
            (Uint32 *)((Uint8 *)surface->pixels + py * surface->pitch + px * 4);
        *pixel = SDL_MapRGB(surface->format, r, g, b);
      }
    }
  }

  SDL_UnlockSurface(surface);
}

void save_surface(const char *filename, SDL_Surface *surface) {
  if (surface) {
    SDL_SaveBMP(surface, filename);
  }
}

void free_grid_zone(GridZone *grid) { (void)grid; }

void free_word_list(WordList *list) {
  if (list && list->words) {
    for (int i = 0; i < list->count; i++) {
      free(list->words[i]);
    }
    free(list->words);
  }
}
