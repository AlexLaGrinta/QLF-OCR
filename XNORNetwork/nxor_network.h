#ifndef NXOR_NETWORK_H
#define NXOR_NETWORK_H

#define INPUT_SIZE 2      
#define HIDDEN_SIZE 2     
#define OUTPUT_SIZE 2
#define EPOCHS 10000
#define LEARNING_RATE 0.5

typedef struct {
    double weights_input_hidden[INPUT_SIZE][HIDDEN_SIZE];
    double biases_hidden[HIDDEN_SIZE];
    double weights_hidden_output[HIDDEN_SIZE][OUTPUT_SIZE];
    double biases_output[OUTPUT_SIZE];
} NeuralNetwork;

typedef struct
{
    double* elements;
    int expected_value;
} SampleData;


#endif