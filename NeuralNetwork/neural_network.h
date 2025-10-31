#ifndef NEURAL_NETWORK_H
#define NEURAL_NETWORK_H

#include <stddef.h>

#define INPUT_SIZE 784      
#define HIDDEN_SIZE 128     
#define OUTPUT_SIZE 26  
#define EPOCHS 200
#define LEARNING_RATE 0.01

typedef struct {
    double weights_input_hidden[INPUT_SIZE][HIDDEN_SIZE];
    double biases_hidden[HIDDEN_SIZE];
    double weights_hidden_output[HIDDEN_SIZE][OUTPUT_SIZE];
    double biases_output[OUTPUT_SIZE];
} NeuralNetwork;

typedef struct
{
    double* pixels;
    int expected_value;
} SampleData;


int predict(NeuralNetwork* network, double* inputs);

double rand_weight();
double sigmoid(double x);
double dsigmoid(double y);
void initialize_network(NeuralNetwork* network);
void softmax(double *logits, double *output, int output_size);
void forward_neural(NeuralNetwork* network, double* inputs, double* hidden, double* outputs);
double cross_entropy(double *probabilities, int true_class, int output_size);
void train_network(NeuralNetwork* network, SampleData** dataset, size_t dataset_size);
void save_network_weights(NeuralNetwork* network, const char* path);
void load_network_weights(NeuralNetwork* network, const char* path);
void start_training_with_dataset(NeuralNetwork* network, const char* path);
void test_score_dataset(NeuralNetwork* network, const char* path);



#endif