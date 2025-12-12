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
  printf("=== TESTING NEW NEURAL NETWORK ===\n");

  // Seed random for reproducibility
  srand(42);

  NeuralNetwork network;

  printf("\n1. Training Network...\n");
  // Training on the uppercase dataset (using relative path to dataset root)
  // Ensure the path is correct. User has 'dataset_oat' and 'dataset_uppercase'
  // Let's use dataset_uppercase as per previous attempts or ask user?
  // User context showed 'dataset_uppercase' in .gitignore.
  // The commented out code said "../NeuralNetwork/dataset_oat", but usually
  // it's "../dataset/dataset_uppercase" Let's stick to
  // "../dataset/dataset_uppercase" as in the original file I wrote. Wait, the
  // viewed file has "../NeuralNetwork/dataset_oat". Let's check where the
  // dataset actually is.
  start_training_with_dataset(&network, "../NeuralNetwork/dataset_oat");

  printf("\n2. Saving Weights...\n");
  save_network_weights(&network, "new_model_weights.bin");

  // test_letter(&network, "letter_0334j.png");

  printf("\n3. Testing on Training Set (Sanity Check)...\n");
  test_score_dataset(&network, "../NeuralNetwork/dataset_oat");

  printf("\n=== TEST COMPLETED ===\n");
  return 0;
}
