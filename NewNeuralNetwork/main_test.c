#include "bmp_converter.h"
#include "new_neural_network.h"
#include <stdio.h>
#include <stdlib.h>

void test_letter(NeuralNetwork *network, const char *path) {
  load_network_weights(network, "new_model_weights.bin");

  double *inputs = img_to_binary_matrix(path);

  int pred = predict(network, inputs);
  printf("%c\n", pred + 'A');
}

int main() {
  printf("=== NEW NEURAL NETWORK ===\n");

  srand(42);

  NeuralNetwork network;

  printf("\n1. Training Network...\n");

  start_training_with_dataset(&network, "../NeuralNetwork/dataset_oat");

  printf("\n2. Saving Weights...\n");
  save_network_weights(&network, "final_model_weights.bin");

  // test_letter(&network, "letter_0334j.png");

  printf("\n3. Testing on Training Set (Sanity Check)...\n");
  test_score_dataset(&network, "../NeuralNetwork/dataset_oat");

  printf("\n=== TEST COMPLETED ===\n");
  return 0;
}
