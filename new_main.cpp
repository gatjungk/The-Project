#include <iostream>
#include <cstdlib>
#include <ctime>
#include "perceptron.h"

void fill_input(matrix &input); 

int main(void){
    srand(time(NULL));
    //perceptron parameters
    double learning_rate = 0.3; 
    int no_of_input_nodes = 3;
    int no_of_hidden_nodes = 16;
    int no_of_output_nodes = 8; 

    //weights and biases
    matrix wih, who;
    matrix bih, bho; 

    //NODE VALUES
    matrix input, target; 

    //TRAINING VALUES
    matrix output_error, hidden_error;
    matrix output_gradient, hidden_gradient; 
    matrix delta_wih, delta_who; 
    matrix delta_bih, delta_bho;
    
    //generating weights and biases (normally distributed)
    wih.gen_norm(no_of_hidden_nodes, no_of_input_nodes);
    who.gen_norm(no_of_output_nodes, no_of_hidden_nodes);
    bih.gen_norm(no_of_hidden_nodes, 1); 
    bho.gen_norm(no_of_output_nodes, 1); 

    //initialization of perceptron
    perceptron hidden_layer = perceptron(wih, bih, input); 
    perceptron output_layer = perceptron(who, bho, hidden_layer.feedforward());

    //temporary target and data sets
    std::vector<std::vector<std::vector<double>>> list_of_inputs   {{{0},{0},{0}}, 
                                                                    {{0}, {0}, {1}}, 
                                                                    {{0}, {1}, {0}}, 
                                                                    {{0}, {1}, {1}},
                                                                    {{1}, {0}, {0}},
                                                                    {{1}, {0}, {1}},
                                                                    {{1}, {1}, {0}},
                                                                    {{1}, {1}, {1}}};
    std::vector<std::vector<std::vector<double>>> list_of_targets{{{1.0},{0.0},{0.0},{0.0},{0.0},{0.0},{0.0},{0.0}},
                                                                   {{0.0},{1.0},{0.0},{0.0},{0.0},{0.0},{0.0},{0.0}},
                                                                   {{0.0},{0.0},{1.0},{0.0},{0.0},{0.0},{0.0},{0.0}},
                                                                   {{0.0},{0.0},{0.0},{1.0},{0.0},{0.0},{0.0},{0.0}},
                                                                   {{0.0},{0.0},{0.0},{0.0},{1.0},{0.0},{0.0},{0.0}},
                                                                   {{0.0},{0.0},{0.0},{0.0},{0.0},{1.0},{0.0},{0.0}},
                                                                   {{0.0},{0.0},{0.0},{0.0},{0.0},{0.0},{1.0},{0.0}}, 
                                                                   {{0.0},{0.0},{0.0},{0.0},{0.0},{0.0},{0.0},{1.0}}}; 

    //train perceptron
    for(int i = 0; i < 10000; i++){
        for(int j = 0; j < 8; j++){
            //RUN THE TEST INPUTS HERE
            input = matrix(list_of_inputs[j]);
            target = list_of_targets[j];

            //running 
            hidden_layer.update(wih, bih, input); 
            output_layer.update(who, bho, hidden_layer.feedforward());
            
            //error calculations 
            output_error = target - output_layer.feedforward(); 
            hidden_error = who.transpose() * output_error; 

            //gradient calculations
            output_gradient = div_sigmoid(output_layer.feedforward()); 
            output_gradient = learning_rate * element_multiply(output_gradient, output_error); 
            
            hidden_gradient = div_sigmoid(hidden_layer.feedforward());
            hidden_gradient = learning_rate * element_multiply(hidden_gradient, hidden_error);

            //delta calculations
            delta_who = output_gradient * hidden_layer.feedforward().transpose();
            delta_wih = hidden_gradient * input.transpose(); 
            delta_bho = output_gradient; 
            delta_bih = hidden_gradient;

            //adjust weights and biases 
            wih = wih + delta_wih; 
            who = who + delta_who;
            bih = bih + delta_bih;
            bho = bho + delta_bho; 
        }
    }

    for(int i = 0; i < 8; i++){
        input = list_of_inputs[i]; 
        hidden_layer.update(wih, bih, input); 
        output_layer.update(who, bho, hidden_layer.feedforward());

        std::cout << "AFTER Input Matrix :" << std::endl; input.matrix_print();
        std::cout << "AFTER Output Matrix :" << std::endl; output_layer.feedforward().matrix_print();
        std::cout << "================================================================" << std::endl;
    }


    return 0;
}

void fill_input(matrix &input){
    std::vector<std::vector<double>> temp; 
    std::vector<double> temp_row; 
    int rows = 2; 
    double input_array[] = {1.0,1.0}; 
    for(int i=0; i<rows; i++){
        temp_row.push_back(input_array[i]);
        temp.push_back(temp_row); 
        temp_row.clear(); 
    }
    input = matrix(temp); 
}