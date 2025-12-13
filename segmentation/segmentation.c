#include "segmentation.h"
#include "char_segmentation.h"
#include "image_segmentation.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

// ============================================================================
// STRUCTURES FOR ROW GROUPING AND PATH STORAGE
// ============================================================================

typedef struct {
  int *indices;
  int count;
  int capacity;
  int avg_y;
} Row;

// Structure pour stocker un chemin de lettre

// ============================================================================
// FUNCTIONS FOR RESULT MANAGEMENT
// ============================================================================

SegmentationResult *create_segmentation_result() {
  SegmentationResult *result = malloc(sizeof(SegmentationResult));
  if (!result)
    return NULL;

  result->grid_lines = NULL;
  result->grid_line_count = 0;
  result->words = NULL;
  result->word_count = 0;

  return result;
}

void free_segmentation_result(SegmentationResult *result) {
  if (!result)
    return;

  if (result->grid_lines) {
    for (int i = 0; i < result->grid_line_count; i++) {
      if (result->grid_lines[i].letters) {
        free(result->grid_lines[i].letters);
      }
    }
    free(result->grid_lines);
  }

  if (result->words) {
    for (int i = 0; i < result->word_count; i++) {
      if (result->words[i].letters) {
        free(result->words[i].letters);
      }
    }
    free(result->words);
  }

  free(result);
}

void print_segmentation_result(SegmentationResult *result) {
  if (!result) {
    printf("No result to display\n");
    return;
  }

  printf("\n========================================\n");
  printf("SEGMENTATION RESULT\n");
  printf("========================================\n\n");

  // Afficher la grille
  printf("GRID (%d lines):\n", result->grid_line_count);
  printf("[\n");
  for (int i = 0; i < result->grid_line_count; i++) {
    printf("  Line %d (%d letters): [\n", i,
           result->grid_lines[i].letter_count);
    for (int j = 0; j < result->grid_lines[i].letter_count; j++) {
      printf("    \"%s\"", result->grid_lines[i].letters[j].path);
      if (j < result->grid_lines[i].letter_count - 1)
        printf(",\n");
      else
        printf("\n");
    }
    printf("  ]");
    if (i < result->grid_line_count - 1)
      printf(",\n");
    else
      printf("\n");
  }
  printf("]\n\n");

  // Afficher les mots
  printf("WORDS TO FIND (%d words):\n", result->word_count);
  printf("[\n");
  for (int w = 0; w < result->word_count; w++) {
    printf("  Word %d (%d letters): [\n", w, result->words[w].letter_count);
    for (int l = 0; l < result->words[w].letter_count; l++) {
      printf("    \"%s\"", result->words[w].letters[l].path);
      if (l < result->words[w].letter_count - 1) {
        printf(",\n");
      } else {
        printf("\n");
      }
    }
    printf("  ]");
    if (w < result->word_count - 1) {
      printf(",\n");
    } else {
      printf("\n");
    }
  }
  printf("]\n");
  printf("========================================\n\n");
}

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

static int compare_chars_by_position(const void *a, const void *b) {
  Character *c1 = (Character *)a;
  Character *c2 = (Character *)b;

  // D'abord trier par ligne (y), puis par colonne (x)
  int y_diff = c1->y - c2->y;
  if (abs(y_diff) > 20) { // Tolérance pour la même ligne
    return y_diff;
  }
  return c1->x - c2->x;
}

static float calculate_avg_letter_width(Character *chars, int count) {
  if (count == 0)
    return 0;

  float total = 0;
  for (int i = 0; i < count; i++) {
    total += chars[i].width;
  }
  return total / count;
}

static float calculate_avg_letter_height(Character *chars, int count) {
  if (count == 0)
    return 0;

  float total = 0;
  for (int i = 0; i < count; i++) {
    total += chars[i].height;
  }
  return total / count;
}

SegmentationResult *perform_segmentation(const char *path) {
  SegmentationResult *result = create_segmentation_result();
  if (!result) {
    fprintf(stderr, "Failed to allocate result structure\n");
    return NULL;
  }

  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
    free_segmentation_result(result);
    return NULL;
  }

  if (IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG) == 0) {
    fprintf(stderr, "IMG_Init Error: %s\n", IMG_GetError());
    SDL_Quit();
    free_segmentation_result(result);
    return NULL;
  }

  SDL_Surface *img = IMG_Load(path);
  if (!img) {
    fprintf(stderr, "Failed to load: %s\n", IMG_GetError());
    IMG_Quit();
    SDL_Quit();
    free_segmentation_result(result);
    return NULL;
  }

  // ========================================================================
  // STEP 1: CONVERSION TO BINARY IMAGE
  // ========================================================================

  BinaryImage *bin = create_binary_image_from_surface(img);
  if (!bin) {
    fprintf(stderr, "✗ Binary conversion failed\n");
    SDL_FreeSurface(img);
    IMG_Quit();
    SDL_Quit();
    free_segmentation_result(result);
    return NULL;
  }

  // ========================================================================
  // STEP 2: DÉTECTING ZONES (GRID + LIST)
  // ========================================================================

  GridBounds grid_bounds = {0};
  GridBounds list_bounds = {0};

  detect_major_zones_improved(bin, &grid_bounds, &list_bounds);

  if (!grid_bounds.valid) {
    fprintf(stderr, "✗ Grid detection failed\n");
    free_binary_image(bin);
    SDL_FreeSurface(img);
    IMG_Quit();
    SDL_Quit();
    free_segmentation_result(result);
    return NULL;
  }

  // ========================================================================
  // STEP 3: CHARACTERS EXTRACTION
  // ========================================================================

  int num_chars = 0;
  Character *chars = find_characters_in_grid(bin, &num_chars, 0, 0);

  if (!chars) {
    fprintf(stderr, "✗ Character extraction failed\n");
    free_binary_image(bin);
    SDL_FreeSurface(img);
    IMG_Quit();
    SDL_Quit();
    free_segmentation_result(result);
    return NULL;
  }

  // ========================================================================
  // STEP 4: ASSIGNMENT OF ZONES
  // ========================================================================

  assign_character_zones(chars, num_chars, grid_bounds, list_bounds);

  // ========================================================================
  // STEP 5: FILTERING (optionnel)
  // ========================================================================

  chars = filter_letters(chars, &num_chars);

  // ========================================================================
  // STEP 6: CREATION OF THE OUTPUT FILES
  // ========================================================================

#ifdef _WIN32
  _mkdir("output");
  _mkdir("output\\grid_letters");
  _mkdir("output\\wordlist_letters");
#else
  mkdir("output", 0755);
  mkdir("output/grid_letters", 0755);
  mkdir("output/wordlist_letters", 0755);
#endif

  // ========================================================================
  // STEP 7: SAVE GRID LETTERS (ordered by rows and columns)
  // ========================================================================

  // Compter les caractères de la grille
  int grid_count = 0;
  for (int i = 0; i < num_chars; i++) {
    if (chars[i].zone == ZONE_GRID)
      grid_count++;
  }

  if (grid_count > 0) {
    printf("\n✓ Processing %d grid characters...\n", grid_count);

    // Extraire les caractères de la grille
    Character *grid_chars = malloc(grid_count * sizeof(Character));
    int g_idx = 0;

    for (int i = 0; i < num_chars; i++) {
      if (chars[i].zone == ZONE_GRID) {
        grid_chars[g_idx++] = chars[i];
      }
    }

    // Trier par position (y puis x)
    qsort(grid_chars, grid_count, sizeof(Character), compare_chars_by_position);

    // Calculer la hauteur moyenne des lettres pour détecter les lignes
    float avg_height = calculate_avg_letter_height(grid_chars, grid_count);
    printf("  Average letter height: %.1f pixels\n", avg_height);

    // Grouper par lignes
    typedef struct {
      int *indices;
      int count;
      int capacity;
      int avg_y;
    } GridRow;

    GridRow *rows = malloc(grid_count * sizeof(GridRow));
    int num_rows = 0;

    // Tolérance pour considérer deux lettres sur la même ligne
    float row_tolerance = avg_height * 0.3f;

    // Grouper les lettres par ligne
    for (int i = 0; i < grid_count; i++) {
      bool found_row = false;

      // Chercher si la lettre appartient à une ligne existante
      for (int r = 0; r < num_rows; r++) {
        if (abs(grid_chars[i].y - rows[r].avg_y) < row_tolerance) {
          // Ajouter à cette ligne
          if (rows[r].count >= rows[r].capacity) {
            rows[r].capacity =
                (rows[r].capacity == 0) ? 10 : rows[r].capacity * 2;
            rows[r].indices =
                realloc(rows[r].indices, rows[r].capacity * sizeof(int));
          }
          rows[r].indices[rows[r].count++] = i;

          // Mettre à jour la position Y moyenne de la ligne
          int total_y = 0;
          for (int j = 0; j < rows[r].count; j++) {
            total_y += grid_chars[rows[r].indices[j]].y;
          }
          rows[r].avg_y = total_y / rows[r].count;

          found_row = true;
          break;
        }
      }

      // Si aucune ligne trouvée, créer une nouvelle ligne
      if (!found_row) {
        rows[num_rows].indices = malloc(10 * sizeof(int));
        rows[num_rows].capacity = 10;
        rows[num_rows].count = 1;
        rows[num_rows].indices[0] = i;
        rows[num_rows].avg_y = grid_chars[i].y;
        num_rows++;
      }
    }

    printf("  Detected %d rows in grid\n", num_rows);

    // Trier les lignes de haut en bas
    for (int i = 0; i < num_rows - 1; i++) {
      for (int j = i + 1; j < num_rows; j++) {
        if (rows[i].avg_y > rows[j].avg_y) {
          GridRow temp = rows[i];
          rows[i] = rows[j];
          rows[j] = temp;
        }
      }
    }
    // Initialize grid lines in result
    result->grid_lines = malloc(num_rows * sizeof(GridLine));
    result->grid_line_count = num_rows;

    // Pour chaque ligne, trier les lettres de gauche à droite et sauvegarder
    int letter_counter = 0;

    for (int r = 0; r < num_rows; r++) {
      // Trier les indices par position X (gauche à droite)
      for (int i = 0; i < rows[r].count - 1; i++) {
        for (int j = i + 1; j < rows[r].count; j++) {
          int idx_i = rows[r].indices[i];
          int idx_j = rows[r].indices[j];

          if (grid_chars[idx_i].x > grid_chars[idx_j].x) {
            int temp = rows[r].indices[i];
            rows[r].indices[i] = rows[r].indices[j];
            rows[r].indices[j] = temp;
          }
        }
      }

      // Sauvegarder les lettres de cette ligne dans l'ordre
      printf("  Row %d: %d letters at y=%d\n", r, rows[r].count, rows[r].avg_y);

      // Allocate for this line in result
      result->grid_lines[r].letters =
          malloc(rows[r].count * sizeof(LetterPath));
      result->grid_lines[r].letter_count = rows[r].count;

      for (int i = 0; i < rows[r].count; i++) {
        int idx = rows[r].indices[i];
        char filename[512];
        snprintf(filename, sizeof(filename),
                 "output/grid_letters/letter_%04d.bmp", letter_counter);
        save_character_bmp(&grid_chars[idx], filename, 28);

        // Stocker le chemin dans le résultat
        snprintf(result->grid_lines[r].letters[i].path, 512, "%s", filename);
        result->grid_lines[r].letters[i].x = grid_chars[idx].x;
        result->grid_lines[r].letters[i].y = grid_chars[idx].y;
        result->grid_lines[r].letters[i].width = grid_chars[idx].width;
        result->grid_lines[r].letters[i].height = grid_chars[idx].height;
        letter_counter++;
      }
    }

    // result->grid_letter_count = letter_counter; // Removed
    printf("\n✓ Saved %d grid letters in order to output/grid_letters/\n",
           letter_counter);

    // Libérer la mémoire
    for (int r = 0; r < num_rows; r++) {
      free(rows[r].indices);
    }
    free(rows);
    free(grid_chars);
  } else {
    printf("  No grid characters detected\n");
  }

  // ========================================================================
  // STEP 8: GROUP WORDLIST CHARACTERS INTO WORDS (IMPROVED - HORIZONTAL)
  // ========================================================================

  // Compter les caractères de la wordlist
  int list_count = 0;
  for (int i = 0; i < num_chars; i++) {
    if (chars[i].zone == ZONE_WORDLIST)
      list_count++;
  }

  if (list_count > 0) {
    printf("\n✓ Processing %d wordlist characters...\n", list_count);

    // Extraire les caractères de la wordlist
    Character *list_chars = malloc(list_count * sizeof(Character));
    int l_idx = 0;

    for (int i = 0; i < num_chars; i++) {
      if (chars[i].zone == ZONE_WORDLIST) {
        list_chars[l_idx++] = chars[i];
      }
    }

    // Trier par position (y puis x pour lecture de gauche à droite)
    qsort(list_chars, list_count, sizeof(Character), compare_chars_by_position);

    // Calculer les statistiques pour la détection des mots
    float avg_width = calculate_avg_letter_width(list_chars, list_count);
    float avg_height = calculate_avg_letter_height(list_chars, list_count);

    // Distance maximale entre deux lettres du même mot (horizontalement)
    float max_letter_spacing = avg_width * 1.5f;

    // Tolérance verticale pour considérer deux lettres sur la même ligne
    float row_tolerance = avg_height * 0.4f;

    printf("  Average letter width: %.1f, height: %.1f\n", avg_width,
           avg_height);
    printf("  Max spacing between letters: %.1f\n", max_letter_spacing);
    printf("  Row tolerance: %.1f\n", row_tolerance);

    // Grouper les lettres par lignes horizontales
    typedef struct {
      int *indices;
      int count;
      int capacity;
      int avg_y;
    } WordlistRow;

    WordlistRow *rows = malloc(list_count * sizeof(WordlistRow));
    int num_rows = 0;

    for (int i = 0; i < list_count; i++) {
      bool found_row = false;

      // Chercher une ligne existante pour cette lettre
      for (int r = 0; r < num_rows; r++) {
        int first_idx = rows[r].indices[0];
        float y_diff = abs(list_chars[i].y - list_chars[first_idx].y);

        if (y_diff <= row_tolerance) {
          // Ajouter à cette ligne
          if (rows[r].count >= rows[r].capacity) {
            rows[r].capacity *= 2;
            rows[r].indices =
                realloc(rows[r].indices, rows[r].capacity * sizeof(int));
          }
          rows[r].indices[rows[r].count++] = i;
          found_row = true;
          break;
        }
      }

      // Créer une nouvelle ligne si nécessaire
      if (!found_row) {
        rows[num_rows].indices = malloc(10 * sizeof(int));
        rows[num_rows].capacity = 10;
        rows[num_rows].count = 1;
        rows[num_rows].indices[0] = i;
        rows[num_rows].avg_y = list_chars[i].y;
        num_rows++;
      }
    }

    printf("  Found %d rows in wordlist\n", num_rows);

    // Trier les lignes de haut en bas
    for (int i = 0; i < num_rows - 1; i++) {
      for (int j = i + 1; j < num_rows; j++) {
        int y1 = list_chars[rows[i].indices[0]].y;
        int y2 = list_chars[rows[j].indices[0]].y;
        if (y1 > y2) {
          WordlistRow temp = rows[i];
          rows[i] = rows[j];
          rows[j] = temp;
        }
      }
    }

    // Pour chaque ligne, trier les lettres de gauche à droite
    for (int r = 0; r < num_rows; r++) {
      for (int i = 0; i < rows[r].count - 1; i++) {
        for (int j = i + 1; j < rows[r].count; j++) {
          int idx_i = rows[r].indices[i];
          int idx_j = rows[r].indices[j];

          if (list_chars[idx_i].x > list_chars[idx_j].x) {
            int temp = rows[r].indices[i];
            rows[r].indices[i] = rows[r].indices[j];
            rows[r].indices[j] = temp;
          }
        }
      }
    }

    // Traiter chaque ligne pour extraire les mots
    int word_num = 0;

    // Allouer un tableau temporaire pour les mots (taille max = nombre de
    // lettres)
    Word2 *temp_words = malloc(list_count * sizeof(Word2));

    for (int r = 0; r < num_rows; r++) {
      printf("\n  Processing row %d (%d letters at y=%d):\n", r, rows[r].count,
             list_chars[rows[r].indices[0]].y);

      // Dans chaque ligne, grouper les lettres en mots basé sur l'espacement
      // horizontal
      int word_start = 0;

      for (int i = 1; i <= rows[r].count; i++) {
        bool new_word = false;

        if (i == rows[r].count) {
          // Fin de la ligne = fin du dernier mot
          new_word = true;
        } else {
          // Vérifier l'espacement horizontal entre deux lettres consécutives
          int idx_prev = rows[r].indices[i - 1];
          int idx_curr = rows[r].indices[i];

          int prev_right = list_chars[idx_prev].x + list_chars[idx_prev].width;
          int curr_left = list_chars[idx_curr].x;
          float horizontal_gap = curr_left - prev_right;

          if (horizontal_gap > max_letter_spacing) {
            new_word = true;
          }
        }

        if (new_word) {
          int word_len = i - word_start;

          if (word_len > 2) {
            // Créer le dossier pour ce mot
            char word_dir[512];
            snprintf(word_dir, sizeof(word_dir),
                     "output/wordlist_letters/word_%03d", word_num);

#ifdef _WIN32
            _mkdir(word_dir);
#else
            mkdir(word_dir, 0755);
#endif

            // Initialiser le mot dans le résultat
            temp_words[word_num].letter_count = word_len;
            temp_words[word_num].letters =
                malloc(word_len * sizeof(LetterPath));

            // Calculer les coordonnées du mot
            int first_idx = rows[r].indices[word_start];
            int last_idx = rows[r].indices[i - 1];
            int word_x = list_chars[first_idx].x;
            int word_y = list_chars[first_idx].y;
            int word_width =
                (list_chars[last_idx].x + list_chars[last_idx].width) - word_x;

            // Sauvegarder chaque lettre du mot
            for (int j = 0; j < word_len; j++) {
              int letter_idx = rows[r].indices[word_start + j];
              char filename[512];
              snprintf(filename, sizeof(filename), "%s/letter_%02d.bmp",
                       word_dir, j);
              save_character_bmp(&list_chars[letter_idx], filename, 28);

              // Stocker le chemin dans le résultat
              snprintf(temp_words[word_num].letters[j].path, 512, "%s",
                       filename);
              temp_words[word_num].letters[j].x = list_chars[letter_idx].x;
              temp_words[word_num].letters[j].y = list_chars[letter_idx].y;
              temp_words[word_num].letters[j].width =
                  list_chars[letter_idx].width;
              temp_words[word_num].letters[j].height =
                  list_chars[letter_idx].height;
            }

            printf("    Word %d: %d letters at position (%d, %d), width=%d -> "
                   "%s\n",
                   word_num, word_len, word_x, word_y, word_width, word_dir);

            word_num++;
          }

          word_start = i;
        }
      }
    }

    // Copier les mots dans le résultat final
    result->word_count = word_num;
    if (word_num > 0) {
      result->words = malloc(word_num * sizeof(Word2));
      memcpy(result->words, temp_words, word_num * sizeof(Word2));
    }
    free(temp_words);

    printf("\n✓ Saved %d words from wordlist to output/wordlist_letters/\n",
           word_num);

    // Libérer la mémoire
    for (int r = 0; r < num_rows; r++) {
      free(rows[r].indices);
    }
    free(rows);
    free(list_chars);
  } else {
    printf("  No wordlist characters detected\n");
  }

  // ========================================================================
  // STEP 9: VISUALIZATION
  // ========================================================================

  SDL_Surface *rgba_img =
      SDL_ConvertSurfaceFormat(img, SDL_PIXELFORMAT_RGBA32, 0);
  if (rgba_img) {

    draw_rectangle_color(rgba_img, grid_bounds.x, grid_bounds.y,
                         grid_bounds.width, grid_bounds.height, 0, 0, 255);

    if (list_bounds.valid) {
      draw_rectangle_color(rgba_img, list_bounds.x, list_bounds.y,
                           list_bounds.width, list_bounds.height, 0, 255, 0);
    }

    SDL_LockSurface(rgba_img);

    for (int i = 0; i < num_chars; i++) {
      Uint8 r = (chars[i].zone == ZONE_GRID) ? 255 : 255;
      Uint8 g = (chars[i].zone == ZONE_GRID) ? 0 : 255;
      Uint8 b = 0;

      int x = chars[i].x;
      int y = chars[i].y;
      int w = chars[i].width;
      int h = chars[i].height;

      for (int px = x; px < x + w && px < rgba_img->w; px++) {
        if (y >= 0 && y < rgba_img->h) {
          Uint32 *pixel = (Uint32 *)((Uint8 *)rgba_img->pixels +
                                     y * rgba_img->pitch + px * 4);
          *pixel = SDL_MapRGB(rgba_img->format, r, g, b);
        }
      }

      for (int px = x; px < x + w && px < rgba_img->w; px++) {
        int py = y + h - 1;
        if (py >= 0 && py < rgba_img->h) {
          Uint32 *pixel = (Uint32 *)((Uint8 *)rgba_img->pixels +
                                     py * rgba_img->pitch + px * 4);
          *pixel = SDL_MapRGB(rgba_img->format, r, g, b);
        }
      }

      for (int py = y; py < y + h && py < rgba_img->h; py++) {
        if (x >= 0 && x < rgba_img->w) {
          Uint32 *pixel = (Uint32 *)((Uint8 *)rgba_img->pixels +
                                     py * rgba_img->pitch + x * 4);
          *pixel = SDL_MapRGB(rgba_img->format, r, g, b);
        }
      }

      for (int py = y; py < y + h && py < rgba_img->h; py++) {
        int px_right = x + w - 1;
        if (px_right >= 0 && px_right < rgba_img->w) {
          Uint32 *pixel = (Uint32 *)((Uint8 *)rgba_img->pixels +
                                     py * rgba_img->pitch + px_right * 4);
          *pixel = SDL_MapRGB(rgba_img->format, r, g, b);
        }
      }
    }

    SDL_UnlockSurface(rgba_img);

    save_surface("output/visualization.bmp", rgba_img);
    SDL_FreeSurface(rgba_img);
  }

  // ========================================================================
  // CLEANING
  // ========================================================================

  free_characters(chars, num_chars);
  free_binary_image(bin);
  SDL_FreeSurface(img);

  IMG_Quit();
  SDL_Quit();

  return result;
}

/*
int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <image.bmp>\n", argv[0]);
    return EXIT_FAILURE;
  }

  SegmentationResult *result = perform_segmentation(argv[1]);

  if (!result) {
    fprintf(stderr, "Segmentation failed\n");
    return EXIT_FAILURE;
  }

  // Afficher les résultats
  print_segmentation_result(result);

  // Libérer la mémoire
  free_segmentation_result(result);

  return EXIT_SUCCESS;
}
*/
