#ifndef NEURAL_NETWORK_H
#define NEURAL_NETWORK_H

#include "bmp_converter.h"

#define INPUT_SIZE 784      
#define HIDDEN_SIZE 128     
#define OUTPUT_SIZE 26  
#define EPOCHS 1000
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


double* initialize_hidden_layer(NeuralNetwork* network, BinaryMatrix* matrix);

#endif