#ifndef NEURAL_NETWORK_H
#define NEURAL_NETWORK_H

#include "bmp_converter.h"

#define INPUT_SIZE 784      
#define HIDDEN_SIZE 128     
#define OUTPUT_SIZE 26  


typedef struct {
    double weights_hidden[INPUT_SIZE][HIDDEN_SIZE];
    double biases_hidden[HIDDEN_SIZE];
} NeuralNetwork;

double* initialize_hidden_layer(NeuralNetwork* network, BinaryMatrix* matrix);

#endif