#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include "neural_network.h"
#include "bmp_converter.h"

double rand_weight() {
    return ((double)rand() / RAND_MAX) * 2.0 - 1.0;
}


double sigmoid(double x) {
    return 1.0 / (1.0 + exp(-x));
}


double* initialize_hidden_layer(NeuralNetwork* network, BinaryMatrix* matrix)
{
    for (int i = 0; i < INPUT_SIZE; i++)
    {
        for (int j = 0; j < HIDDEN_SIZE; j++)
        {
            network->weights_hidden[i][j] = rand_weight();
        }
    }

    for (int i = 0; i < HIDDEN_SIZE; i++)
    {
        network->biases_hidden[i] = rand_weight();
    }

    double* hidden_layer = malloc(HIDDEN_SIZE * sizeof(double));

    for (int i = 0; i < HIDDEN_SIZE; i++)
    {
        double weights_input = 0;
        for (int j = 0; j < INPUT_SIZE; j++)
        {
            weights_input += (double)matrix->data[j] * network->weights_hidden[j][i];
        }
        hidden_layer[i] = sigmoid(weights_input) + network->biases_hidden[i];
    }

    
    return hidden_layer;

}