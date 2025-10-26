#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <time.h>
#include "xor_network.h"



double rand_weight() {
    return ((double)rand() / RAND_MAX) * 2.0 - 1.0;
}


double sigmoid(double x) {
    return 1.0 / (1.0 + exp(-x));
}

double dsigmoid(double y) {
    return y * (1.0 - y);
}


void initialize_network(NeuralNetwork* network)
{
    for (int i = 0; i < INPUT_SIZE; i++)
    {
        for (int j = 0; j < HIDDEN_SIZE; j++)
        {
            network->weights_input_hidden[i][j] = rand_weight();    // init weights of input->hidden links
        }
    }

    for (int i = 0; i < HIDDEN_SIZE; i++)
    {
        network->biases_hidden[i] = rand_weight();   // init hidden biases
    }


    for (int i = 0; i < HIDDEN_SIZE; i++)
    {
        for (int j = 0; j < OUTPUT_SIZE; j++)
        {
            network->weights_hidden_output[i][j] = rand_weight(); // init weights of hidden->output biases
        }
    }

    for (int i = 0; i < OUTPUT_SIZE; i++)
    {
        network->biases_output[i] = rand_weight(); // init output biases
    }
    
}

void forward_neural(NeuralNetwork* network, double* inputs, double* hidden, double* outputs)
{
    for (int i = 0; i < HIDDEN_SIZE; i++)
    {
        double weights_input_sum = 0;
        for (int j = 0; j < INPUT_SIZE; j++)
        {
            weights_input_sum += inputs[j] * network->weights_input_hidden[j][i];
        }
        hidden[i] = sigmoid(weights_input_sum + network->biases_hidden[i]); 
    }

    for (int i = 0; i < OUTPUT_SIZE; i++)
    {
        double weights_hidden_sum = 0;
        for (int j = 0; j < HIDDEN_SIZE; j++)
        {
            weights_hidden_sum += hidden[j] * network->weights_hidden_output[j][i];
        }
        outputs[i] = sigmoid(weights_hidden_sum + network->biases_output[i]); 
    }


    //softmax(outputs,outputs,OUTPUT_SIZE);
}

double cross_entropy(double *probabilities, int true_class) {
    double epsilon = 1e-10;
    double p_true = probabilities[true_class];
    p_true = fmax(fmin(p_true, 1.0 - epsilon), epsilon); 
    return -log(p_true); 
}



void train_network(NeuralNetwork* network, SampleData** dataset, size_t dataset_size) {

    double* hidden  = (double*)malloc(HIDDEN_SIZE * sizeof(double));
    double* outputs = (double*)malloc(OUTPUT_SIZE * sizeof(double));

    initialize_network(network);

    for (int epoch = 0; epoch < EPOCHS; ++epoch) {

        double total_loss = 0.0;
        size_t correct = 0;

        for (size_t sample_index = 0; sample_index < dataset_size; ++sample_index) {
            SampleData* s = dataset[sample_index];
            forward_neural(network, s->pixels, hidden, outputs);

            // Expected output 
            double y[OUTPUT_SIZE];
            for (int j = 0; j < OUTPUT_SIZE; ++j) y[j] = 0.0;
            y[s->expected_value] = 1.0;

            // Backpropagation
            double delta_out[OUTPUT_SIZE];
            for (int j = 0; j < OUTPUT_SIZE; ++j) {
                double err = y[j] - outputs[j];
                delta_out[j] = err * dsigmoid(outputs[j]);
            }

            double delta_hidden[HIDDEN_SIZE];
            for (int i = 0; i < HIDDEN_SIZE; ++i) {
                double back = 0.0;
                for (int j = 0; j < OUTPUT_SIZE; ++j)
                    back += delta_out[j] * network->weights_hidden_output[i][j];
                delta_hidden[i] = back * dsigmoid(hidden[i]);
            }

            // Weights update
            for (int i = 0; i < HIDDEN_SIZE; ++i) {
                for (int j = 0; j < OUTPUT_SIZE; ++j)
                    network->weights_hidden_output[i][j] += LEARNING_RATE * delta_out[j] * hidden[i];
            }
            for (int j = 0; j < OUTPUT_SIZE; ++j)
                network->biases_output[j] += LEARNING_RATE * delta_out[j];

            for (int i = 0; i < INPUT_SIZE; ++i) {
                for (int j = 0; j < HIDDEN_SIZE; ++j)
                    network->weights_input_hidden[i][j] += LEARNING_RATE * delta_hidden[j] * s->pixels[i];
            }
            for (int i = 0; i < HIDDEN_SIZE; ++i)
                network->biases_hidden[i] += LEARNING_RATE * delta_hidden[i];

            // Stats
            total_loss += cross_entropy(outputs, s->expected_value);

            int result = outputs[0] > outputs[1] ? 0 : 1;
            correct += (result == s->expected_value);

        }

        // print
        double avg_mse = total_loss / (double)dataset_size;
        double acc = (double)correct / (double)dataset_size * 100.0;

        printf(
            "Epoch %d/%d  |  Loss: %.6f  |  Acc: %.2f%%\n",
            epoch + 1, EPOCHS, avg_mse, acc
        );
    }

    free(hidden);
    free(outputs);
}




int predict(NeuralNetwork* network, double* inputs)
{
    double* hidden = malloc(HIDDEN_SIZE * sizeof(double));
    double* outputs = malloc(OUTPUT_SIZE * sizeof(double));

    forward_neural(network, inputs, hidden, outputs);
    size_t max_index = 0;
    double max_value = *outputs;
    for (size_t i = 0; i < OUTPUT_SIZE; i++)
    {
        //printf("%c : %f\n", (char)i +'A' ,outputs[i]);
        if (outputs[i] > max_value)
        {
            max_index = i;
            max_value = outputs[i];
        }
    }

    free(hidden);
    free(outputs);

    return max_index; 
}


void save_network_weights(NeuralNetwork* network, const char* path)
{
    char file_path[32];

    snprintf(file_path, sizeof(file_path), "../temp/%s.bin", path);


    FILE* f = fopen(file_path, "wb");

    for (int i = 0; i < INPUT_SIZE; i++)
    {
        for (int j = 0; j < HIDDEN_SIZE; j++)
        {
            double data = network->weights_input_hidden[i][j];
            fwrite(&data, sizeof(double), 1, f);
        }
    }

    for (int j = 0; j < HIDDEN_SIZE; j++)
    {
        double data = network->biases_hidden[j];
        fwrite(&data, sizeof(double), 1, f);
    }

    for (int i = 0; i < HIDDEN_SIZE; i++)
    {
        for (int j = 0; j < OUTPUT_SIZE; j++)
        {
            double data = network->weights_hidden_output[i][j];
            fwrite(&data, sizeof(double), 1, f);
        }
    }

    for (int j = 0; j < OUTPUT_SIZE; j++)
    {
        double data = network->biases_output[j];
        fwrite(&data, sizeof(double), 1, f);
    }

    fclose(f);

    printf("Weights saved to : %s\n", file_path);
}


void load_network_weights(NeuralNetwork* network, const char* path)
{
    FILE* f = fopen(path, "rb");

    for (int i = 0; i < INPUT_SIZE; i++)
    {
        for (int j = 0; j < HIDDEN_SIZE; j++)
        {
            fread(&network->weights_input_hidden[i][j], sizeof(double), 1, f);
        }
    }

    for (int j = 0; j < HIDDEN_SIZE; j++)
    {
        fread(&network->biases_hidden[j], sizeof(double), 1, f);
    }

    for (int i = 0; i < HIDDEN_SIZE; i++)
    {
        for (int j = 0; j < OUTPUT_SIZE; j++)
        {
            fread(&network->weights_hidden_output[i][j], sizeof(double), 1, f);
        }
    }

    for (int j = 0; j < OUTPUT_SIZE; j++)
    {
        fread(&network->biases_output[j], sizeof(double), 1, f);
    }

    fclose(f);

    printf("Weights successfully loaded from : %s\n", path);


}


SampleData** initialize_sample()
{
    SampleData** dataset = malloc(sizeof(SampleData*)*4);

    SampleData* data = malloc(sizeof(SampleData));
    data->expected_value = 1;
    data->pixels = malloc(sizeof(double)*2);
    data->pixels[0] = 0;
    data->pixels[1] = 1;
    dataset[0] = data;

    SampleData* data1 = malloc(sizeof(SampleData));
    data1->expected_value = 1;
    data1->pixels = malloc(sizeof(double)*2);
    data1->pixels[0] = 1;
    data1->pixels[1] = 0;
    dataset[1] = data1;

    SampleData* data2 = malloc(sizeof(SampleData));
    data2->expected_value = 0;
    data2->pixels = malloc(sizeof(double)*2);
    data2->pixels[0] = 1;
    data2->pixels[1] = 1;
    dataset[2] = data2;

    SampleData* data3 = malloc(sizeof(SampleData));
    data3->expected_value = 0;
    data3->pixels = malloc(sizeof(double)*2);
    data3->pixels[0] = 0;
    data3->pixels[1] = 0;
    dataset[3] = data3;

    return dataset;
}


void free_sample(SampleData** data)
{
    for (int i = 0; i < 4; i++)
    {
        free(data[i]->pixels);  
        free(data[i]);          
    }
    free(data);                
}


void test_xor_network(NeuralNetwork* network)
{
    double acc = 0;

    double inputs[2] = {1,0};
    int pred = predict(network, inputs);
    printf("%i xor %i = %i\n", (int)inputs[0], (int)inputs[1] ,pred);
    acc += (pred == 1);

    inputs[0] = 0;
    inputs[1] = 1;
    pred = predict(network, inputs);
    printf("%i xor %i = %i\n", (int)inputs[0], (int)inputs[1] ,pred);
    acc += (pred == 1);

    inputs[0] = 0;
    inputs[1] = 0;
    pred = predict(network, inputs);
    printf("%i xor %i = %i\n", (int)inputs[0], (int)inputs[1] ,pred);
    acc += (pred == 0);

    inputs[0] = 1;
    inputs[1] = 1;
    pred = predict(network, inputs);
    printf("%i xor %i = %i\n", (int)inputs[0], (int)inputs[1] ,pred);
    acc += (pred == 0);

    printf("Accuracy : %.2f%%", (acc/4)*100);

}




int main()
{
    
    SampleData** data = initialize_sample();

    NeuralNetwork network;


    /*
    load_network_weights(&network, "../temp/xor.bin");
    */

    train_network(&network, data,4);

    free_sample(data);

    test_xor_network(&network);

    //save_network_weights(&network,"xor");

    return EXIT_SUCCESS;
}