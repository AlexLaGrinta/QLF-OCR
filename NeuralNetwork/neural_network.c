#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <time.h>
#include "neural_network.h"
#include "bmp_converter.h"

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
}


static inline double mse_sample(const double* y, const double* yhat, int n) {
    double s = 0.0;
    for (int i = 0; i < n; ++i) {
        double d = y[i] - yhat[i];
        s += d*d;
    }
    return s / n;
}

void train_network(NeuralNetwork* network, SampleData** dataset, size_t dataset_size) {
    double* hidden  = (double*)malloc(HIDDEN_SIZE * sizeof(double));
    double* outputs = (double*)malloc(OUTPUT_SIZE * sizeof(double));

    initialize_network(network);

    clock_t start_time = clock();  

    for (int epoch = 0; epoch < EPOCHS; ++epoch) {
        clock_t epoch_start = clock();  

        double total_mse = 0.0;
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
            total_mse += mse_sample(y, outputs, OUTPUT_SIZE);

            int pred = 0;
            double vmax = outputs[0];
            for (int j = 1; j < OUTPUT_SIZE; ++j)
                if (outputs[j] > vmax) { vmax = outputs[j]; pred = j; }
            if (pred == s->expected_value) ++correct;
        }

        clock_t epoch_end = clock();

        // ETA
        double elapsed_total = (double)(epoch_end - start_time) / CLOCKS_PER_SEC;
        double avg_time_per_epoch = elapsed_total / (epoch + 1);
        double eta_seconds = avg_time_per_epoch * (EPOCHS - epoch - 1);
        int eta_minutes = (int)eta_seconds / 60;
        int eta_seconds_remaining = (int)eta_seconds % 60;

        // print
        double avg_mse = total_mse / (double)dataset_size;
        double acc = (double)correct / (double)dataset_size * 100.0;

        printf(
            "Epoch %d/%d  |  MSE: %.6f  |  Acc: %.2f%%  |  ETA: %02dm%02ds\n",
            epoch + 1, EPOCHS, avg_mse, acc, eta_minutes, eta_seconds_remaining
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



static void u8_to_double_array(const uint8_t* in, size_t n, double* out)
{
    for (size_t i = 0; i < n; ++i)
        out[i] = (double)(in[i] != 0); // 0 -> 0.0, 1 -> 1.0 
}


void print_matrixx(double* matrix)
{
    for (int y = 0; y < 28; y++)
    {
        for (int x = 0; x < 28; x++)
        {
            size_t i = (size_t)y * (size_t)28 + (size_t)x;
            printf("%f ", matrix[i]);
        }
        printf("\n");
    }
}


void start_training_with_dataset(NeuralNetwork* network, const char* path)
{

    LetterDataset raw_dataset = load_dataset(path);

    int dataset_size = 0;

    for (int i = 0; i < OUTPUT_SIZE; i++)
    {
        dataset_size += raw_dataset.samples_per_letter[i];
    }



    SampleData** dataset = malloc(sizeof(*dataset) * dataset_size);
    for (int k = 0; k < dataset_size; ++k) {
        dataset[k] = malloc(sizeof(**dataset));
        dataset[k]->pixels = malloc(sizeof(double) * INPUT_SIZE);
    }


    int count = 0;

    for (int i = 0; i < OUTPUT_SIZE; i++)
    {
        for (int j = 0; j < raw_dataset.samples_per_letter[i]; j++)
        {
            dataset[count]->expected_value = i;
            u8_to_double_array(raw_dataset.matrices[i][j]->data, INPUT_SIZE, dataset[count]->pixels);
            count++;
        }
    }

    train_network(network, dataset, dataset_size);

    for (int k = 0; k < dataset_size; ++k) {
        free(dataset[k]->pixels);
        free(dataset[k]);
    }
    free(dataset);
    free_dataset(&raw_dataset);
}



int main()
{

    NeuralNetwork network;

    start_training_with_dataset(&network, "../dataset/dataset_uppercase");

    return EXIT_SUCCESS;
}