#include "char_segmentation.h"
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  int min_x, max_x;
  int min_y, max_y;
  int pixel_count;
  bool valid;
} Component;

typedef struct {
  int *x;
  int *y;
  int front;
  int rear;
  int capacity;
} Queue;

static Queue *create_queue(int capacity) {
  Queue *q = malloc(sizeof(Queue));
  if (!q)
    return NULL;

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

static void free_queue(Queue *q) {
  if (q) {
    free(q->x);
    free(q->y);
    free(q);
  }
}

static void enqueue(Queue *q, int x, int y) {
  if (q->rear < q->capacity) {
    q->x[q->rear] = x;
    q->y[q->rear] = y;
    q->rear++;
  }
}

static bool dequeue(Queue *q, int *x, int *y) {
  if (q->front >= q->rear)
    return false;
  *x = q->x[q->front];
  *y = q->y[q->front];
  q->front++;
  return true;
}

static bool is_empty(Queue *q) { return q->front >= q->rear; }

static void bfs_component(BinaryImage *bin, bool *visited, int start_x,
                          int start_y, Component *comp, Queue *q) {
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

    if (x < comp->min_x)
      comp->min_x = x;
    if (x > comp->max_x)
      comp->max_x = x;
    if (y < comp->min_y)
      comp->min_y = y;
    if (y > comp->max_y)
      comp->max_y = y;

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

Character *find_characters_in_grid(BinaryImage *bin, int *num_chars,
                                   int offset_x, int offset_y) {
  if (!bin || !num_chars)
    return NULL;

  bool *visited = calloc(bin->width * bin->height, sizeof(bool));
  if (!visited)
    return NULL;

  Component *temp_comps = malloc(1000 * sizeof(Component));
  if (!temp_comps) {
    free(visited);
    return NULL;
  }

  int comp_count = 0;
  Queue *q = create_queue(bin->width * bin->height);
  if (!q) {
    free(visited);
    free(temp_comps);
    return NULL;
  }

  for (int y = 0; y < bin->height; y++) {
    for (int x = 0; x < bin->width; x++) {
      int idx = y * bin->width + x;

      if (!visited[idx] && bin->data[idx] == 0) {
        if (comp_count >= 1000)
          break;

        Component comp;
        bfs_component(bin, visited, x, y, &comp, q);

        temp_comps[comp_count++] = comp;
      }
    }
  }

  free_queue(q);

  Character *chars = malloc(comp_count * sizeof(Character));
  if (!chars) {
    free(visited);
    free(temp_comps);
    return NULL;
  }

  *num_chars = comp_count;

  for (int i = 0; i < comp_count; i++) {
    Component *comp = &temp_comps[i];

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

void assign_character_zones(Character *chars, int num_chars,
                            GridBounds grid_bounds, GridBounds list_bounds) {
  if (!chars)
    return;

  int grid_count = 0;
  int list_count = 0;

  for (int i = 0; i < num_chars; i++) {
    int cx = chars[i].x + chars[i].width / 2;
    int cy = chars[i].y + chars[i].height / 2;

    bool in_grid =
        (cx >= grid_bounds.x && cx < grid_bounds.x + grid_bounds.width &&
         cy >= grid_bounds.y && cy < grid_bounds.y + grid_bounds.height);

    bool in_list = false;
    if (list_bounds.valid) {
      in_list =
          (cx >= list_bounds.x && cx < list_bounds.x + list_bounds.width &&
           cy >= list_bounds.y && cy < list_bounds.y + list_bounds.height);
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

Character *split_merged_characters(Character *chars, int *num_chars) {
  if (!chars || *num_chars == 0)
    return chars;

  float grid_total_width = 0;
  int grid_count = 0;
  float list_total_width = 0;
  int list_count = 0;

  for (int i = 0; i < *num_chars; i++) {
    if (chars[i].zone == ZONE_GRID) {
      grid_total_width += chars[i].width;
      grid_count++;
    } else if (chars[i].zone == ZONE_WORDLIST) {
      list_total_width += chars[i].width;
      list_count++;
    }
  }

  float grid_avg_w = (grid_count > 0) ? grid_total_width / grid_count : 0;
  float list_avg_w = (list_count > 0) ? list_total_width / list_count : 0;

  int capacity = (*num_chars) * 2 + 100;
  Character *new_chars = malloc(capacity * sizeof(Character));
  int new_count = 0;

  int split_count = 0;

  for (int i = 0; i < *num_chars; i++) {
    Character ch = chars[i];
    bool did_split = false;

    float avg_w = (ch.zone == ZONE_GRID) ? grid_avg_w : list_avg_w;

    if (avg_w == 0)
      avg_w = (grid_avg_w > 0) ? grid_avg_w : list_avg_w;

    if (avg_w > 0 && ch.width > 2.0 * avg_w && ch.width > 15) {
      int start_search = ch.width * 0.25;
      int end_search = ch.width * 0.75;

      int best_split_x = -1;
      int min_black_pixels = INT_MAX;

      for (int x = start_search; x <= end_search; x++) {
        int black_count = 0;
        for (int y = 0; y < ch.height; y++) {
          if (ch.data[y * ch.width + x] < 128) {
            black_count++;
          }
        }

        if (black_count < min_black_pixels) {
          min_black_pixels = black_count;
          best_split_x = x;
        }
      }

      if (best_split_x != -1) {
        Character left;
        left.x = ch.x;
        left.y = ch.y;
        left.width = best_split_x;
        left.height = ch.height;
        left.zone = ch.zone;
        left.data = malloc(left.width * left.height);

        for (int y = 0; y < left.height; y++) {
          memcpy(left.data + y * left.width, ch.data + y * ch.width,
                 left.width);
        }

        Character right;
        right.x = ch.x + best_split_x;
        right.y = ch.y;
        right.width = ch.width - best_split_x;
        right.height = ch.height;
        right.zone = ch.zone;
        right.data = malloc(right.width * right.height);

        for (int y = 0; y < right.height; y++) {
          memcpy(right.data + y * right.width,
                 ch.data + y * ch.width + best_split_x, right.width);
        }

        new_chars[new_count++] = left;
        new_chars[new_count++] = right;
        split_count++;
        did_split = true;

        free(ch.data);
      }
    }

    if (!did_split) {
      new_chars[new_count++] = ch;
    }
  }

  free(chars);

  *num_chars = new_count;
  return new_chars;
}

Character *filter_letters(Character *chars, int *num_chars) {
  if (!chars || *num_chars == 0)
    return chars;

  int grid_count = 0, wordlist_count = 0, unknown_count = 0;
  for (int i = 0; i < *num_chars; i++) {
    if (chars[i].zone == ZONE_GRID)
      grid_count++;
    else if (chars[i].zone == ZONE_WORDLIST)
      wordlist_count++;
    else
      unknown_count++;
  }

  float calculate_density(Character * ch) {
    if (!ch->data || ch->width == 0 || ch->height == 0)
      return 0.5f;

    int black_pixels = 0;
    int total_pixels = ch->width * ch->height;

    for (int i = 0; i < total_pixels; i++) {
      if (ch->data[i] < 128)
        black_pixels++;
    }

    return (float)black_pixels / total_pixels;
  }

  int count_connected_components(Character * ch) {
    if (!ch->data || ch->width == 0 || ch->height == 0)
      return 0;

    int w = ch->width;
    int h = ch->height;
    bool *visited = calloc(w * h, sizeof(bool));
    int num_components = 0;

    for (int y = 0; y < h; y++) {
      for (int x = 0; x < w; x++) {
        int idx = y * w + x;

        if (ch->data[idx] < 128 && !visited[idx]) {
          num_components++;

          int *queue_x = malloc(w * h * sizeof(int));
          int *queue_y = malloc(w * h * sizeof(int));
          int front = 0, rear = 0;

          queue_x[rear] = x;
          queue_y[rear] = y;
          rear++;
          visited[idx] = true;

          while (front < rear) {
            int cx = queue_x[front];
            int cy = queue_y[front];
            front++;

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

  float total_width = 0, total_height = 0, total_area = 0;
  int valid_for_stats = 0;

  for (int i = 0; i < *num_chars; i++) {
    if (chars[i].zone != ZONE_WORDLIST)
      continue;

    int w = chars[i].width;
    int h = chars[i].height;
    int area = w * h;

    if (w >= 1 && h >= 3 && w <= 300 && h <= 300 && area >= 5) {
      total_width += w;
      total_height += h;
      total_area += area;
      valid_for_stats++;
    }
  }

  float avg_width = (valid_for_stats > 0) ? total_width / valid_for_stats : 0;
  float avg_height = (valid_for_stats > 0) ? total_height / valid_for_stats : 0;
  float avg_area = (valid_for_stats > 0) ? total_area / valid_for_stats : 0;

  float var_width = 0, var_height = 0, var_area = 0;

  if (valid_for_stats > 0) {
    for (int i = 0; i < *num_chars; i++) {
      if (chars[i].zone != ZONE_WORDLIST)
        continue;

      int w = chars[i].width;
      int h = chars[i].height;
      int area = w * h;

      if (w >= 1 && h >= 3 && w <= 300 && h <= 300 && area >= 5) {
        var_width += (w - avg_width) * (w - avg_width);
        var_height += (h - avg_height) * (h - avg_height);
        var_area += (area - avg_area) * (area - avg_area);
      }
    }

    var_width /= valid_for_stats;
    var_height /= valid_for_stats;
    var_area /= valid_for_stats;
  }

  float std_width = sqrtf(var_width);
  float std_height = sqrtf(var_height);
  float std_area = sqrtf(var_area);

  int grid_filtered_null = 0;
  int grid_filtered_single = 0;
  int grid_filtered_gigantic = 0;
  int grid_filtered_invalid = 0;
  int grid_kept = 0;

  int wordlist_filtered_size = 0;
  int wordlist_filtered_aspect = 0;
  int wordlist_filtered_density = 0;
  int wordlist_filtered_components = 0;
  int wordlist_filtered_outlier = 0;
  int wordlist_filtered_cluster = 0;
  int wordlist_kept = 0;

  int unknown_kept = 0;

  int kept = 0;

  for (int i = 0; i < *num_chars; i++) {
    bool valid = true;
    int w = chars[i].width;
    int h = chars[i].height;
    int area = w * h;
    float aspect = (h > 0) ? (float)w / h : 0;

    if (chars[i].zone == ZONE_GRID) {
      if (w <= 0 || h <= 0) {
        valid = false;
        grid_filtered_null++;
      }

      if (valid) {
        if (h < 5 && w < 3) {
          valid = false;
          grid_filtered_single++;
        }

        if (w < 1 || h < 3) {
          valid = false;
          grid_filtered_single++;
        }
      }

      if (valid && area < 6) {
        valid = false;
        grid_filtered_single++;
      }

      if (valid && h < 5) {
        if (w < 10) {
          valid = false;
          grid_filtered_single++;
        }
      }

      if (valid && h >= 6 && w < 2 && area < 10) {
        valid = false;
        grid_filtered_single++;
      }

      if (valid && chars[i].data) {
        int num_comp = count_connected_components(&chars[i]);

        if (num_comp > 0 && area > 0) {
          float comp_density = (float)num_comp / area;
          if (comp_density > 0.20 || num_comp > 20) {
            valid = false;
            grid_filtered_invalid++;
          }

          if (area < 25 && num_comp > 2) {
            valid = false;
            grid_filtered_invalid++;
          }

          if (area >= 25 && area < 50 && num_comp > 4) {
            valid = false;
            grid_filtered_invalid++;
          }
        }
      }

      if (valid) {
        int bbox_area = w * h;

        if (bbox_area > 0 && area > 0) {
          float fill_ratio = (float)area / bbox_area;

          if (fill_ratio < 0.03 && area > 10) {
            valid = false;
            grid_filtered_invalid++;
          }

          if (fill_ratio < 0.10 && area < 30) {
            valid = false;
            grid_filtered_invalid++;
          }

          if (fill_ratio < 0.08 && area < 60) {
            valid = false;
            grid_filtered_invalid++;
          }

          if (fill_ratio < 0.06 && area < 100) {
            valid = false;
            grid_filtered_invalid++;
          }
        }
      }

      if (valid && chars[i].data) {
        float density = calculate_density(&chars[i]);

        if (density < 0.03) {
          valid = false;
          grid_filtered_invalid++;
        }

        if (area < 25 && density < 0.25) {
          valid = false;
          grid_filtered_invalid++;
        }

        if (area >= 25 && area < 40 && density < 0.18) {
          valid = false;
          grid_filtered_invalid++;
        }

        if (area >= 40 && area < 80 && density < 0.15) {
          valid = false;
          grid_filtered_invalid++;
        }
      }

      if (valid && w > 0 && h > 0) {
        float aspect = (float)w / h;

        if (aspect > 6.0 && h < 7) {
          if (chars[i].data) {
            int num_comp = count_connected_components(&chars[i]);
            if (num_comp > 2) {
              valid = false;
              grid_filtered_invalid++;
            }
          } else {
            valid = false;
            grid_filtered_invalid++;
          }
        }

        if (aspect > 10.0 && h < 5) {
          valid = false;
          grid_filtered_invalid++;
        }

        if (aspect > 15.0 && h < 6) {
          valid = false;
          grid_filtered_invalid++;
        }
      }

      if (valid && area > 35000) {
        if (chars[i].data) {
          float density = calculate_density(&chars[i]);

          if (density < 0.04) {
            valid = false;
            grid_filtered_gigantic++;
          }
        } else {
          valid = false;
          grid_filtered_gigantic++;
        }
      }

      if (valid && (w > 1000 || h > 1000)) {
        valid = false;
        grid_filtered_gigantic++;
      }

      if (valid && w < 6 && h < 8) {
        valid = false;
        grid_filtered_single++;
      }

      if (valid && area >= 8 && area < 20 && h < 7) {
        valid = false;
        grid_filtered_single++;
      }

      if (valid && w > 0 && h > 0) {
        float aspect = (w > h) ? (float)w / h : (float)h / w;

        if (aspect < 1.5 && area < 35) {
          valid = false;
          grid_filtered_single++;
        }

        if (aspect < 1.3 && area < 50) {
          valid = false;
          grid_filtered_single++;
        }
      }

      if (valid && area >= 20 && area < 100 && h < 6) {
        valid = false;
        grid_filtered_single++;
      }

      if (valid && chars[i].data) {
        int bbox_area = w * h;

        if (bbox_area > 100 && area < 20) {
          valid = false;
          grid_filtered_invalid++;
        }

        if (bbox_area > 50 && area < 10) {
          valid = false;
          grid_filtered_invalid++;
        }
      }

      if (valid && w <= 3 && h < 7) {
        valid = false;
        grid_filtered_single++;
      }

      if (valid && area < 30 && chars[i].data) {
        float density = calculate_density(&chars[i]);
        int bbox_area = w * h;
        float fill_ratio = (float)area / bbox_area;

        if (density < 0.30 || fill_ratio < 0.20) {
          valid = false;
          grid_filtered_invalid++;
        }
      }

      if (!valid && w >= 1 && w <= 2 && h >= 8 && area >= 8) {
        if (chars[i].data) {
          float density = calculate_density(&chars[i]);
          int num_comp = count_connected_components(&chars[i]);
          int bbox_area = w * h;
          float fill_ratio = (float)area / bbox_area;

          if (density >= 0.35 && num_comp <= 1 && fill_ratio >= 0.65) {
            valid = true;
            if (grid_filtered_invalid > 0)
              grid_filtered_invalid--;
            else if (grid_filtered_single > 0)
              grid_filtered_single--;
          }
        }
      }

      if (!valid && w >= 2 && w <= 4 && h >= 9 && area >= 14) {
        if (chars[i].data) {
          float density = calculate_density(&chars[i]);
          int num_comp = count_connected_components(&chars[i]);
          int bbox_area = w * h;
          float fill_ratio = (float)area / bbox_area;

          if (density >= 0.35 && num_comp <= 2 && fill_ratio >= 0.50) {
            valid = true;
            if (grid_filtered_invalid > 0)
              grid_filtered_invalid--;
            else if (grid_filtered_single > 0)
              grid_filtered_single--;
          }
        }
      }

      if (!valid && w > 0 && h >= 10 && w <= 12) {
        float aspect = (float)h / w;

        if (aspect > 3.5 && aspect < 35) {
          if (chars[i].data) {
            float density = calculate_density(&chars[i]);
            int num_comp = count_connected_components(&chars[i]);
            int bbox_area = w * h;
            float fill_ratio = (float)area / bbox_area;

            if (density >= 0.25 && num_comp <= 3 && fill_ratio >= 0.30) {
              valid = true;
              if (grid_filtered_invalid > 0)
                grid_filtered_invalid--;
              else if (grid_filtered_single > 0)
                grid_filtered_single--;
            }
          }
        }
      }

      if (!valid && w >= 8 && w <= 40 && h >= 8 && h <= 40) {
        if (chars[i].data) {
          float density = calculate_density(&chars[i]);
          int num_comp = count_connected_components(&chars[i]);
          int bbox_area = w * h;
          float fill_ratio = (float)area / bbox_area;

          if (density >= 0.18 && num_comp <= 7 && fill_ratio >= 0.18) {
            valid = true;
            if (grid_filtered_invalid > 0)
              grid_filtered_invalid--;
            else if (grid_filtered_gigantic > 0)
              grid_filtered_gigantic--;
          }
        }
      }

      if (!valid && area >= 8 && area <= 18 && h >= 2 && h <= 6 && w >= 2 &&
          w <= 6) {
        if (chars[i].data) {
          float density = calculate_density(&chars[i]);
          int num_comp = count_connected_components(&chars[i]);
          int bbox_area = w * h;
          float fill_ratio = (float)area / bbox_area;

          if (density >= 0.50 && num_comp <= 1 && fill_ratio >= 0.45) {
            valid = true;
            if (grid_filtered_single > 0)
              grid_filtered_single--;
            else if (grid_filtered_invalid > 0)
              grid_filtered_invalid--;
          }
        }
      }

      if (!valid && area >= 40 && area <= 3500 && h >= 8) {
        if (chars[i].data) {
          float density = calculate_density(&chars[i]);
          int num_comp = count_connected_components(&chars[i]);
          int bbox_area = w * h;
          float fill_ratio = (float)area / bbox_area;

          if (density >= 0.15 && num_comp <= 10 && fill_ratio >= 0.15) {
            valid = true;
            if (grid_filtered_invalid > 0)
              grid_filtered_invalid--;
            else if (grid_filtered_single > 0)
              grid_filtered_single--;
          }
        }
      }

      if (!valid && w >= 10 && h >= 3 && h <= 5 && area >= 18) {
        float aspect = (float)w / h;

        if (aspect > 3.0 && aspect < 10.0) {
          if (chars[i].data) {
            float density = calculate_density(&chars[i]);
            int num_comp = count_connected_components(&chars[i]);
            int bbox_area = w * h;
            float fill_ratio = (float)area / bbox_area;

            if (density >= 0.35 && num_comp <= 2 && fill_ratio >= 0.40) {
              valid = true;
              if (grid_filtered_single > 0)
                grid_filtered_single--;
            }
          }
        }
      }

      if (!valid && w >= 6 && w <= 15 && h >= 8 && h <= 35) {
        if (chars[i].data) {
          float density = calculate_density(&chars[i]);
          int num_comp = count_connected_components(&chars[i]);
          int bbox_area = w * h;
          float fill_ratio = (float)area / bbox_area;

          if (density >= 0.20 && num_comp <= 5 && fill_ratio >= 0.20) {
            valid = true;
            if (grid_filtered_invalid > 0)
              grid_filtered_invalid--;
            else if (grid_filtered_single > 0)
              grid_filtered_single--;
          }
        }
      }

      if (!valid && w >= 15 && w <= 40 && h >= 8 && h <= 40) {
        if (chars[i].data) {
          float density = calculate_density(&chars[i]);
          int num_comp = count_connected_components(&chars[i]);
          int bbox_area = w * h;
          float fill_ratio = (float)area / bbox_area;

          if (density >= 0.15 && num_comp <= 8 && fill_ratio >= 0.15) {
            valid = true;
            if (grid_filtered_invalid > 0)
              grid_filtered_invalid--;
            else if (grid_filtered_single > 0)
              grid_filtered_single--;
          }
        }
      }

      if (valid)
        grid_kept++;
    }

    else if (chars[i].zone == ZONE_WORDLIST) {
      if (w < 1 || h < 3) {
        valid = false;
        wordlist_filtered_size++;
      }

      if (w > 500 || h > 500) {
        valid = false;
        wordlist_filtered_size++;
      }

      if (valid && area < 5) {
        valid = false;
        wordlist_filtered_size++;
      }

      if (valid && area > 50000) {
        valid = false;
        wordlist_filtered_size++;
      }

      if (valid && aspect > 0) {
        if (aspect < 0.02 || aspect > 50.0) {
          valid = false;
          wordlist_filtered_aspect++;
        }
      }

      if (valid && valid_for_stats > 20) {
        float z_width = fabsf(w - avg_width) / (std_width + 1);
        float z_height = fabsf(h - avg_height) / (std_height + 1);
        float z_area = fabsf(area - avg_area) / (std_area + 1);

        if (z_width > 6.0 || z_height > 6.0 || z_area > 7.0) {
          valid = false;
          wordlist_filtered_outlier++;
        }
      }

      if (valid && chars[i].data) {
        float density = calculate_density(&chars[i]);

        if (density < 0.02 || density > 0.99) {
          valid = false;
          wordlist_filtered_density++;
        }
      }

      if (valid && chars[i].data) {
        int num_comp = count_connected_components(&chars[i]);

        if (num_comp > 10) {
          valid = false;
          wordlist_filtered_components++;
        }
      }

      if (!valid && area >= 8) {
        float thin_aspect = (h > w) ? ((float)h / w) : ((float)w / h);

        if (thin_aspect > 2.0) {
          if (chars[i].data) {
            float density = calculate_density(&chars[i]);
            int num_comp = count_connected_components(&chars[i]);

            if (density >= 0.05 && density <= 0.95 && num_comp <= 5) {
              valid = true;

              if (wordlist_filtered_size > 0)
                wordlist_filtered_size--;
              if (wordlist_filtered_aspect > 0)
                wordlist_filtered_aspect--;
            }
          }
        }
      }

      if (!valid && area >= 5 && area <= 30) {
        if (chars[i].data) {
          float density = calculate_density(&chars[i]);

          if (density >= 0.20 && density <= 0.95) {
            int num_comp = count_connected_components(&chars[i]);

            if (num_comp <= 2) {
              valid = true;
              if (wordlist_filtered_size > 0)
                wordlist_filtered_size--;
              if (wordlist_filtered_density > 0)
                wordlist_filtered_density--;
            }
          }
        }
      }

      if (valid)
        wordlist_kept++;
    }

    else {
      if (w < 1 || h < 3 || w > 500 || h > 500 || area < 5 || area > 50000) {
        valid = false;
      }

      if (valid && aspect > 0 && (aspect < 0.02 || aspect > 50.0)) {
        valid = false;
      }

      if (valid && chars[i].data) {
        float density = calculate_density(&chars[i]);
        if (density < 0.02 || density > 0.99) {
          valid = false;
        }
      }

      if (valid)
        unknown_kept++;
    }

    if (valid) {
      if (kept != i) {
        chars[kept] = chars[i];
      }
      kept++;
    } else {
      if (chars[i].data) {
        free(chars[i].data);
      }
    }
  }

  *num_chars = kept;

  int current_wl_count = 0;
  for (int i = 0; i < kept; i++) {
    if (chars[i].zone == ZONE_WORDLIST)
      current_wl_count++;
  }

  if (current_wl_count > 0) {
    int *heights = malloc(kept * sizeof(int));
    int h_count = 0;

    for (int i = 0; i < kept; i++) {
      if (chars[i].zone == ZONE_WORDLIST && chars[i].data != NULL) {
        heights[h_count++] = chars[i].height;
      }
    }

    if (h_count > 0) {
      for (int i = 0; i < h_count - 1; i++) {
        for (int j = 0; j < h_count - i - 1; j++) {
          if (heights[j] > heights[j + 1]) {
            int temp = heights[j];
            heights[j] = heights[j + 1];
            heights[j + 1] = temp;
          }
        }
      }

      float median_h = heights[h_count / 2];
      float min_h_threshold = median_h * 0.30f;
      float max_h_threshold = median_h * 2.00f;

      if (min_h_threshold < 4)
        min_h_threshold = 4;

      int outlier_count = 0;
      for (int i = 0; i < kept; i++) {
        if (chars[i].zone == ZONE_WORDLIST && chars[i].data != NULL) {
          if (chars[i].height < min_h_threshold ||
              chars[i].height > max_h_threshold) {
            if (chars[i].data)
              free(chars[i].data);
            chars[i].zone = ZONE_UNKNOWN;
            chars[i].data = NULL;
            outlier_count++;
          }
        }
      }
      if (outlier_count > 0) {
        wordlist_filtered_outlier += outlier_count;
        wordlist_kept -= outlier_count;
      }
    }
    free(heights);

    float wl_total_w = 0, wl_total_h = 0;
    int wl_valid = 0;
    for (int i = 0; i < kept; i++) {
      if (chars[i].zone == ZONE_WORDLIST && chars[i].data != NULL) {
        wl_total_w += chars[i].width;
        wl_total_h += chars[i].height;
        wl_valid++;
      }
    }
    float wl_avg_h = (wl_valid > 0) ? wl_total_h / wl_valid : 20.0f;

    float dist_threshold = wl_avg_h * 4.0;
    if (dist_threshold < 50)
      dist_threshold = 50;

    int *component = malloc(kept * sizeof(int));
    for (int i = 0; i < kept; i++)
      component[i] = -1;

    int comp_id = 0;
    for (int i = 0; i < kept; i++) {
      if (chars[i].zone != ZONE_WORDLIST)
        continue;
      if (component[i] != -1)
        continue;
      if (chars[i].data == NULL)
        continue;

      component[i] = comp_id;

      int *queue = malloc(kept * sizeof(int));
      int head = 0, tail = 0;
      queue[tail++] = i;

      while (head < tail) {
        int curr = queue[head++];
        Character *c1 = &chars[curr];

        for (int j = 0; j < kept; j++) {
          if (chars[j].zone != ZONE_WORDLIST)
            continue;
          if (component[j] != -1)
            continue;
          if (chars[j].data == NULL)
            continue;

          Character *c2 = &chars[j];

          float dx = (c1->x + c1->width / 2.0f) - (c2->x + c2->width / 2.0f);
          float dy = (c1->y + c1->height / 2.0f) - (c2->y + c2->height / 2.0f);
          float dist = sqrtf(dx * dx + dy * dy);

          if (dist < dist_threshold) {
            component[j] = comp_id;
            queue[tail++] = j;
          }
        }
      }

      free(queue);
      comp_id++;
    }

    if (comp_id > 0) {
      int *comp_sizes = calloc(comp_id, sizeof(int));
      for (int i = 0; i < kept; i++) {
        if (chars[i].zone == ZONE_WORDLIST && component[i] != -1) {
          comp_sizes[component[i]]++;
        }
      }

      int max_size = 0;
      for (int c = 0; c < comp_id; c++) {
        if (comp_sizes[c] > max_size) {
          max_size = comp_sizes[c];
        }
      }

      int cluster_filtered = 0;
      for (int i = 0; i < kept; i++) {
        if (chars[i].zone == ZONE_WORDLIST && chars[i].data != NULL) {
          if (component[i] != -1) {
            int size = comp_sizes[component[i]];
            if (size < 1) {
              if (chars[i].data)
                free(chars[i].data);
              chars[i].zone = ZONE_UNKNOWN;
              chars[i].data = NULL;
              cluster_filtered++;
            }
          }
        }
      }

      if (cluster_filtered > 0) {
        wordlist_kept -= cluster_filtered;
        wordlist_filtered_cluster += cluster_filtered;
      }

      free(comp_sizes);
    }

    free(component);

    int new_kept = 0;
    for (int i = 0; i < kept; i++) {
      if (chars[i].zone == ZONE_UNKNOWN && chars[i].data == NULL) {
        continue;
      }

      if (new_kept != i) {
        chars[new_kept] = chars[i];
      }
      new_kept++;
    }
    kept = new_kept;
    *num_chars = kept;
  }

  int grid_total_filtered = grid_filtered_null + grid_filtered_single +
                            grid_filtered_gigantic + grid_filtered_invalid;

  int wordlist_total_filtered =
      wordlist_filtered_size + wordlist_filtered_aspect +
      wordlist_filtered_density + wordlist_filtered_components +
      wordlist_filtered_outlier + wordlist_filtered_cluster;

  if (kept > 0) {
    Character *resized = realloc(chars, kept * sizeof(Character));
    return resized ? resized : chars;
  }

  free(chars);
  return NULL;
}

static int compare_chars_by_position(const void *a, const void *b) {
  Character *c1 = (Character *)a;
  Character *c2 = (Character *)b;

  int y_diff = c1->y - c2->y;
  if (abs(y_diff) > 20) {
    return y_diff;
  }
  return c1->x - c2->x;
}

Word *group_characters_into_words(Character *chars, int num_chars,
                                  int *num_words) {
  if (!chars || num_chars == 0 || !num_words) {
    *num_words = 0;
    return NULL;
  }

  int grid_count = 0, list_count = 0;
  for (int i = 0; i < num_chars; i++) {
    if (chars[i].zone == ZONE_GRID)
      grid_count++;
    else if (chars[i].zone == ZONE_WORDLIST)
      list_count++;
  }

  Character *grid_chars = malloc(grid_count * sizeof(Character));
  Character *list_chars = malloc(list_count * sizeof(Character));

  int g_idx = 0, l_idx = 0;
  for (int i = 0; i < num_chars; i++) {
    if (chars[i].zone == ZONE_GRID) {
      grid_chars[g_idx++] = chars[i];
    } else if (chars[i].zone == ZONE_WORDLIST) {
      list_chars[l_idx++] = chars[i];
    }
  }

  qsort(grid_chars, grid_count, sizeof(Character), compare_chars_by_position);
  qsort(list_chars, list_count, sizeof(Character), compare_chars_by_position);

  Word *words = malloc((grid_count + list_count) * sizeof(Word));
  int word_count = 0;

  if (grid_count > 0) {
    float total_spacing = 0;
    int spacing_count = 0;

    for (int i = 1; i < grid_count; i++) {
      if (abs(grid_chars[i].y - grid_chars[i - 1].y) < 20) {
        int spacing =
            grid_chars[i].x - (grid_chars[i - 1].x + grid_chars[i - 1].width);
        if (spacing > 0 && spacing < 100) {
          total_spacing += spacing;
          spacing_count++;
        }
      }
    }

    float avg_spacing =
        (spacing_count > 0) ? (total_spacing / spacing_count) : 15.0f;
    float word_threshold = avg_spacing * 2.5f;

    if (word_threshold < 20)
      word_threshold = 20;

    int current_word_start = 0;

    for (int i = 1; i <= grid_count; i++) {
      bool new_word = false;

      if (i == grid_count) {
        new_word = true;
      } else {
        int y_diff = abs(grid_chars[i].y - grid_chars[i - 1].y);
        int x_spacing =
            grid_chars[i].x - (grid_chars[i - 1].x + grid_chars[i - 1].width);

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

            Character *ch = &grid_chars[current_word_start + j];
            if (ch->x < min_x)
              min_x = ch->x;
            if (ch->y < min_y)
              min_y = ch->y;
            if (ch->x + ch->width > max_x)
              max_x = ch->x + ch->width;
            if (ch->y + ch->height > max_y)
              max_y = ch->y + ch->height;
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

  if (list_count > 0) {
    int current_word_start = 0;

    for (int i = 1; i <= list_count; i++) {
      bool new_word = false;

      if (i == list_count) {
        new_word = true;
      } else {
        int y_diff = abs(list_chars[i].y - list_chars[i - 1].y);

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

            Character *ch = &list_chars[current_word_start + j];
            if (ch->x < min_x)
              min_x = ch->x;
            if (ch->y < min_y)
              min_y = ch->y;
            if (ch->x + ch->width > max_x)
              max_x = ch->x + ch->width;
            if (ch->y + ch->height > max_y)
              max_y = ch->y + ch->height;
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

void free_words(Word *words, int num_words) {
  if (!words)
    return;

  for (int i = 0; i < num_words; i++) {
    free(words[i].letters);
  }
  free(words);
}

void save_character_bmp(Character *ch, const char *filename, int target_size) {
  if (!ch || !ch->data)
    return;

  SDL_Surface *surf = SDL_CreateRGBSurfaceWithFormat(
      0, ch->width, ch->height, 32, SDL_PIXELFORMAT_RGBA32);

  if (!surf)
    return;

  SDL_LockSurface(surf);
  uint8_t *pixels = (uint8_t *)surf->pixels;

  for (int y = 0; y < ch->height; y++) {
    for (int x = 0; x < ch->width; x++) {
      uint8_t val = ch->data[y * ch->width + x];
      uint32_t color = SDL_MapRGBA(surf->format, val, val, val, 255);
      *(uint32_t *)&pixels[y * surf->pitch + x * 4] = color;
    }
  }

  SDL_UnlockSurface(surf);

  if (target_size > 0 &&
      (ch->width != target_size || ch->height != target_size)) {
    SDL_Surface *resized = SDL_CreateRGBSurfaceWithFormat(
        0, target_size, target_size, 32, SDL_PIXELFORMAT_RGBA32);

    if (resized) {
      SDL_FillRect(resized, NULL, SDL_MapRGB(resized->format, 255, 255, 255));

      SDL_Rect dst;
      float scale = fminf((float)target_size / ch->width,
                          (float)target_size / ch->height);
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

void free_characters(Character *chars, int num_chars) {
  if (!chars)
    return;
  for (int i = 0; i < num_chars; i++) {
    free(chars[i].data);
  }
  free(chars);
}
