#ifndef NEW_NEURAL_NETWORK_H
#define NEW_NEURAL_NETWORK_H

#include <stddef.h>

#define INPUT_SIZE 784
#define HIDDEN_SIZE_1 128
#define HIDDEN_SIZE_2 128
#define OUTPUT_SIZE 26

#define EPOCHS 50 
#define INITIAL_LEARNING_RATE 0.001
#define DECAY_RATE 0.001
#define LAMBDA 0.0
#define MOMENTUM 0.90 // Momentum factor

typedef struct {
  // Weights
  double w_input_h1[INPUT_SIZE][HIDDEN_SIZE_1];
  double w_h1_h2[HIDDEN_SIZE_1][HIDDEN_SIZE_2];
  double w_h2_output[HIDDEN_SIZE_2][OUTPUT_SIZE];

  // Biases
  double b_h1[HIDDEN_SIZE_1];
  double b_h2[HIDDEN_SIZE_2];
  double b_output[OUTPUT_SIZE];

  // Momentum (Previous updates)
  double v_w_input_h1[INPUT_SIZE][HIDDEN_SIZE_1];
  double v_w_h1_h2[HIDDEN_SIZE_1][HIDDEN_SIZE_2];
  double v_w_h2_output[HIDDEN_SIZE_2][OUTPUT_SIZE];

  double v_b_h1[HIDDEN_SIZE_1];
  double v_b_h2[HIDDEN_SIZE_2];
  double v_b_output[OUTPUT_SIZE];
} NeuralNetwork;

typedef struct {
  double *pixels;
  int expected_value;
} SampleData;

void initialize_network(NeuralNetwork *network);
void train_network(NeuralNetwork *network, SampleData **dataset,
                   size_t dataset_size);
int predict(NeuralNetwork *network, double *inputs);

double relu(double x);
double d_relu(double y); 

void save_network_weights(NeuralNetwork *network, const char *path);
void load_network_weights(NeuralNetwork *network, const char *path);

void start_training_with_dataset(NeuralNetwork *network, const char *path);
void test_score_dataset(NeuralNetwork *network, const char *path);

#endif
