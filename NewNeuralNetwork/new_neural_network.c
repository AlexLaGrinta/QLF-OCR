#include "new_neural_network.h"
#include "bmp_converter.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// ============================================================================
// ACTIVATION FUNCTIONS
// ============================================================================

double relu(double x) { return x > 0 ? x : 0; }

double d_relu(double y) { return y > 0 ? 1.0 : 0.0; }

// Stable Softmax
void softmax(double *logits, double *output, int output_size) {
  double max_logit = logits[0];
  for (int i = 1; i < output_size; i++) {
    if (logits[i] > max_logit) {
      max_logit = logits[i];
    }
  }

  double sum = 0.0;
  for (int i = 0; i < output_size; i++) {
    output[i] = exp(logits[i] - max_logit);
    sum += output[i];
  }

  for (int i = 0; i < output_size; i++) {
    output[i] /= sum;
  }
}

// ============================================================================
// INITIALIZATION (Xavier/Glorot)
// ============================================================================

double rand_xavier(int fan_in, int fan_out) {
  double limit = sqrt(6.0 / (fan_in + fan_out));
  return ((double)rand() / RAND_MAX) * 2.0 * limit - limit;
}

void initialize_network(NeuralNetwork *network) {
  // Input -> H1
  for (int i = 0; i < INPUT_SIZE; i++) {
    for (int j = 0; j < HIDDEN_SIZE_1; j++) {
      network->w_input_h1[i][j] = rand_xavier(INPUT_SIZE, HIDDEN_SIZE_1);
      network->v_w_input_h1[i][j] = 0.0;
    }
  }
  for (int i = 0; i < HIDDEN_SIZE_1; i++) {
    network->b_h1[i] = 0.0;
    network->v_b_h1[i] = 0.0;
  }

  // H1 -> H2
  for (int i = 0; i < HIDDEN_SIZE_1; i++) {
    for (int j = 0; j < HIDDEN_SIZE_2; j++) {
      network->w_h1_h2[i][j] = rand_xavier(HIDDEN_SIZE_1, HIDDEN_SIZE_2);
      network->v_w_h1_h2[i][j] = 0.0;
    }
  }
  for (int i = 0; i < HIDDEN_SIZE_2; i++) {
    network->b_h2[i] = 0.0;
    network->v_b_h2[i] = 0.0;
  }

  // H2 -> Output
  for (int i = 0; i < HIDDEN_SIZE_2; i++) {
    for (int j = 0; j < OUTPUT_SIZE; j++) {
      network->w_h2_output[i][j] = rand_xavier(HIDDEN_SIZE_2, OUTPUT_SIZE);
      network->v_w_h2_output[i][j] = 0.0;
    }
  }
  for (int i = 0; i < OUTPUT_SIZE; i++) {
    network->b_output[i] = 0.0;
    network->v_b_output[i] = 0.0;
  }
}

// ============================================================================
// FORWARD PASS
// ============================================================================

void forward_neural(NeuralNetwork *network, double *inputs, double *h1,
                    double *h2, double *outputs) {
  // Input -> H1
  for (int i = 0; i < HIDDEN_SIZE_1; i++) {
    double sum = 0;
    for (int j = 0; j < INPUT_SIZE; j++) {
      sum += inputs[j] * network->w_input_h1[j][i];
    }
    h1[i] = relu(sum + network->b_h1[i]);
  }

  // H1 -> H2
  for (int i = 0; i < HIDDEN_SIZE_2; i++) {
    double sum = 0;
    for (int j = 0; j < HIDDEN_SIZE_1; j++) {
      sum += h1[j] * network->w_h1_h2[j][i];
    }
    h2[i] = relu(sum + network->b_h2[i]);
  }

  // H2 -> Output
  for (int i = 0; i < OUTPUT_SIZE; i++) {
    double sum = 0;
    for (int j = 0; j < HIDDEN_SIZE_2; j++) {
      sum += h2[j] * network->w_h2_output[j][i];
    }
    outputs[i] = sum + network->b_output[i]; // Logits
  }

  // Softmax
  softmax(outputs, outputs, OUTPUT_SIZE);
}

// ============================================================================
// TRAINING
// ============================================================================

double cross_entropy(double *probabilities, int true_class) {
  double epsilon = 1e-10;
  double p = probabilities[true_class];
  if (p < epsilon)
    p = epsilon;
  return -log(p);
}

void train_network(NeuralNetwork *network, SampleData **dataset,
                   size_t dataset_size) {
  double *h1 = malloc(HIDDEN_SIZE_1 * sizeof(double));
  double *h2 = malloc(HIDDEN_SIZE_2 * sizeof(double));
  double *outputs = malloc(OUTPUT_SIZE * sizeof(double));

  size_t *indices = malloc(dataset_size * sizeof(size_t));
  for (size_t i = 0; i < dataset_size; ++i)
    indices[i] = i;

  initialize_network(network);

  printf("Starting training (Momentum=%.2f, 2 Hidden Layers)...\n", MOMENTUM);

  double current_lr = INITIAL_LEARNING_RATE;

  for (int epoch = 0; epoch < EPOCHS; ++epoch) {
    current_lr = INITIAL_LEARNING_RATE / (1.0 + DECAY_RATE * epoch);

    // Shuffle
    for (size_t i = 0; i < dataset_size - 1; i++) {
      size_t j = i + rand() / (RAND_MAX / (dataset_size - i) + 1);
      size_t t = indices[j];
      indices[j] = indices[i];
      indices[i] = t;
    }

    double total_loss = 0.0;
    size_t correct = 0;

    for (size_t k = 0; k < dataset_size; ++k) {
      SampleData *s = dataset[indices[k]];

      // Forward
      forward_neural(network, s->pixels, h1, h2, outputs);

      // Accuracy & Loss
      int pred = 0;
      double max_val = outputs[0];
      for (int j = 1; j < OUTPUT_SIZE; ++j) {
        if (outputs[j] > max_val) {
          max_val = outputs[j];
          pred = j;
        }
      }
      if (pred == s->expected_value)
        correct++;
      total_loss += cross_entropy(outputs, s->expected_value);

      // Expected vector
      double expected[OUTPUT_SIZE] = {0};
      expected[s->expected_value] = 1.0;

      // --- Backprop ---

      // Delta Output
      double delta_out[OUTPUT_SIZE];
      for (int j = 0; j < OUTPUT_SIZE; ++j) {
        delta_out[j] = outputs[j] - expected[j];
      }

      // Delta H2
      double delta_h2[HIDDEN_SIZE_2];
      for (int i = 0; i < HIDDEN_SIZE_2; ++i) {
        double error = 0.0;
        for (int j = 0; j < OUTPUT_SIZE; ++j) {
          error += delta_out[j] * network->w_h2_output[i][j];
        }
        delta_h2[i] = error * d_relu(h2[i]);
      }

      // Delta H1
      double delta_h1[HIDDEN_SIZE_1];
      for (int i = 0; i < HIDDEN_SIZE_1; ++i) {
        double error = 0.0;
        for (int j = 0; j < HIDDEN_SIZE_2; ++j) {
          error += delta_h2[j] * network->w_h1_h2[i][j];
        }
        delta_h1[i] = error * d_relu(h1[i]);
      }

      // --- Update with Momentum & L2 ---

      // Update H2 -> Output
      for (int i = 0; i < HIDDEN_SIZE_2; i++) {
        for (int j = 0; j < OUTPUT_SIZE; j++) {
          double grad = delta_out[j] * h2[i];
          double velocity =
              MOMENTUM * network->v_w_h2_output[i][j] -
              current_lr * (grad + LAMBDA * network->w_h2_output[i][j]);
          network->v_w_h2_output[i][j] = velocity;
          network->w_h2_output[i][j] += velocity;
        }
      }
      for (int j = 0; j < OUTPUT_SIZE; j++) {
        double velocity =
            MOMENTUM * network->v_b_output[j] - current_lr * delta_out[j];
        network->v_b_output[j] = velocity;
        network->b_output[j] += velocity;
      }

      // Update H1 -> H2
      for (int i = 0; i < HIDDEN_SIZE_1; i++) {
        for (int j = 0; j < HIDDEN_SIZE_2; j++) {
          double grad = delta_h2[j] * h1[i];
          double velocity =
              MOMENTUM * network->v_w_h1_h2[i][j] -
              current_lr * (grad + LAMBDA * network->w_h1_h2[i][j]);
          network->v_w_h1_h2[i][j] = velocity;
          network->w_h1_h2[i][j] += velocity;
        }
      }
      for (int j = 0; j < HIDDEN_SIZE_2; j++) {
        double velocity =
            MOMENTUM * network->v_b_h2[j] - current_lr * delta_h2[j];
        network->v_b_h2[j] = velocity;
        network->b_h2[j] += velocity;
      }

      // Update Input -> H1
      for (int i = 0; i < INPUT_SIZE; i++) {
        for (int j = 0; j < HIDDEN_SIZE_1; j++) {
          double grad = delta_h1[j] * s->pixels[i];
          double velocity =
              MOMENTUM * network->v_w_input_h1[i][j] -
              current_lr * (grad + LAMBDA * network->w_input_h1[i][j]);
          network->v_w_input_h1[i][j] = velocity;
          network->w_input_h1[i][j] += velocity;
        }
      }
      for (int j = 0; j < HIDDEN_SIZE_1; j++) {
        double velocity =
            MOMENTUM * network->v_b_h1[j] - current_lr * delta_h1[j];
        network->v_b_h1[j] = velocity;
        network->b_h1[j] += velocity;
      }
    }

    // Logging
    if ((epoch + 1) % 5 == 0 || epoch == 0 || epoch == EPOCHS - 1) {
      printf("Epoch %3d/%d | Loss: %.4f | Acc: %.2f%% | LR: %.4f\n", epoch + 1,
             EPOCHS, total_loss / dataset_size,
             (double)correct / dataset_size * 100.0, current_lr);
    }
  }

  free(h1);
  free(h2);
  free(outputs);
  free(indices);
}

// ============================================================================
// PREDICTION & IO
// ============================================================================

int predict(NeuralNetwork *network, double *inputs) {
  double *h1 = malloc(HIDDEN_SIZE_1 * sizeof(double));
  double *h2 = malloc(HIDDEN_SIZE_2 * sizeof(double));
  double *outputs = malloc(OUTPUT_SIZE * sizeof(double));

  forward_neural(network, inputs, h1, h2, outputs);

  int max_index = 0;
  double max_value = outputs[0];

  for (int i = 1; i < OUTPUT_SIZE; i++) {
    if (outputs[i] > max_value) {
      max_value = outputs[i];
      max_index = i;
    }
  }

  free(h1);
  free(h2);
  free(outputs);
  return max_index;
}

// Struct serialization (memory compatible only if struct padding same)
void save_network_weights(NeuralNetwork *network, const char *path) {
  FILE *f = fopen(path, "wb");
  if (f) {
    fwrite(network, sizeof(NeuralNetwork), 1, f);
    fclose(f);
    printf("Weights saved to %s\n", path);
  } else {
    perror("Save failed");
  }
}

void load_network_weights(NeuralNetwork *network, const char *path) {
  FILE *f = fopen(path, "rb");
  if (f) {
    if (fread(network, sizeof(NeuralNetwork), 1, f) != 1) {
      fprintf(stderr, "Warning: failed to read full network struct\n");
    }
    fclose(f);
    printf("Weights loaded from %s\n", path);
  } else {
    perror("Load failed");
  }
}

void start_training_with_dataset(NeuralNetwork *network, const char *path) {
  DoubleDataset raw_dataset = load_double_dataset(path);
  size_t dataset_size = 0;
  SampleData **dataset = double_dataset_to_samples(&raw_dataset, &dataset_size);

  if (dataset) {
    train_network(network, dataset, dataset_size);
    for (size_t i = 0; i < dataset_size; i++) {
      free(dataset[i]->pixels);
      free(dataset[i]);
    }
    free(dataset);
  }
  free_double_dataset(&raw_dataset);
}

void test_score_dataset(NeuralNetwork *network, const char *path) {
  DoubleDataset raw_dataset = load_double_dataset(path);
  size_t dataset_size = 0;
  SampleData **dataset = double_dataset_to_samples(&raw_dataset, &dataset_size);

  if (!dataset)
    return;

  int correct = 0;
  for (size_t i = 0; i < dataset_size; i++) {
    if (predict(network, dataset[i]->pixels) == dataset[i]->expected_value)
      correct++;
  }

  printf("Test Score: %.2f%% (%d/%zu)\n",
         (double)correct / dataset_size * 100.0, correct, dataset_size);

  for (size_t i = 0; i < dataset_size; i++) {
    free(dataset[i]->pixels);
    free(dataset[i]);
  }
  free(dataset);
  free_double_dataset(&raw_dataset);
}
