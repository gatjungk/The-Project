#include <iostream>
#include <cstdlib>
#include <ctime>
#include "perceptron.h"

void generate_matrix(std::vector<std::vector<double>> &matrix, int rows, int columns);
void generate_input(std::vector<std::vector<double>> &matrix, int rows); 
void generate_bias(std::vector<std::vector<double>> &matrix, int rows);
void randomize_input(matrix &in, matrix &target);
double rand_gen();
double normalRandom();

int main(void){
    srand(time(NULL));

    double learning_rate = 0.3; 
    int no_of_input_nodes = 2;
    int no_of_hidden_nodes = 2;
    int no_of_output_nodes = 1;

    std::vector<std::vector<double>> wih;
    std::vector<std::vector<double>> who;
    std::vector<std::vector<double>> bih; 
    std::vector<std::vector<double>> bho; 
    std::vector<std::vector<double>> input_vec; 
    std::vector<std::vector<double>> target_vec; 

    matrix weight_who1, weight_wih, bias_bho, bias_bih; 
    matrix output_error, hidden_error; 
    matrix gradient_output, gradient_hidden, delta_who, delta_wih, delta_bho, delta_bih; 

    //random normal distribution generated weights 
    generate_matrix(who, no_of_output_nodes, no_of_hidden_nodes);
    generate_matrix(wih, no_of_hidden_nodes, no_of_input_nodes);
    generate_matrix(bho, no_of_output_nodes, no_of_hidden_nodes); 
    generate_matrix(bih, no_of_hidden_nodes, no_of_input_nodes); 

    /*
    //need to generate biases as equal to zero
    generate_bias(bih, no_of_hidden_nodes);
    generate_bias(bho, no_of_output_nodes);
    */
    
    //input values into input vector
    generate_input(input_vec, no_of_input_nodes); 

    weight_who1 = matrix(who); 
    weight_wih = matrix(wih); 
    bias_bho = matrix(bho);
    bias_bih = matrix(bih);

    matrix input = matrix(input_vec); 
    perceptron hidden1_node = perceptron(weight_wih, bias_bih, input); 
    perceptron output_node = perceptron(weight_who1, bias_bho, hidden1_node.feedforward()) ;
    matrix output = output_node.feedforward();

    std::cout << "WIH :" << std::endl; weight_wih.matrix_print(); 
    std::cout << "WHO :" << std::endl; weight_who1.matrix_print();
    std::cout << "================================================================" << std::endl; 
    std::cout << "HIDDEN BIAS :" << std::endl; bias_bih.matrix_print(); 
    std::cout << "OUTPUT BIAS :" << std::endl; bias_bho.matrix_print(); 
    std::cout << "================================================================" << std::endl;
    std::cout << "Input Matrix :" << std::endl; input.matrix_print();
    std::cout << "Output Matrix :" << std::endl; output.matrix_print();
    std::cout << "================================================================" << std::endl;

    //Target 
    std::vector<std::vector<double>> target_vector{{0.0}}; //A^B = 1 
    matrix target = matrix(target_vector); 

    for(int i = 0; i < 10; i++){
        std::cout << "================================================================" << std::endl;
        std::cout << "================================================================" << std::endl;
        std::cout << "================================================================" << std::endl;
        //error calculations
        output_error =  target - output; 
        hidden_error = weight_who1.transpose() * output_error;
        std::cout << "Output ERROR :" << std::endl; output_error.matrix_print();
        std::cout << "Hidden ERROR :" << std::endl; hidden_error.matrix_print();
        std::cout << "================================================================" << std::endl;

        //gradient descent
        gradient_output = output * (1 - output); //gradient = x * (1 - X)
        gradient_hidden = hidden1_node.feedforward() * (1 - hidden1_node.feedforward());
        //std::cout << "OUTPUT GRADIENT :" << std::endl; gradient_output.matrix_print();
        //std::cout << "gradient_hidden :" << std::endl; gradient_hidden.matrix_print();
       // std::cout << "================================================================" << std::endl;

        //delta weight calculations
        delta_who = (learning_rate * (gradient_output * output_error)) * hidden1_node.feedforward().transpose(); 
        delta_wih = (learning_rate * (gradient_hidden * hidden_error)) * input.transpose();
        //std::cout << "DELTA WHO :" << std::endl; delta_who.matrix_print();
        //std::cout << "DELTA WIH :" << std::endl; delta_wih.matrix_print();
        //std::cout << "================================================================" << std::endl;

        //delta bias 
        delta_bih = learning_rate * (gradient_hidden * hidden_error);
        delta_bho = learning_rate * (gradient_output * output_error);
        //std::cout << "delta_bih:" << std::endl; delta_bih.matrix_print();
        //std::cout << "delta_bho :" << std::endl; delta_bho.matrix_print();
        //std::cout << "================================================================" << std::endl;

        weight_who1 = weight_who1 + delta_who;
        weight_wih = weight_wih + delta_wih;   
        //std::cout << "weight_who1:" << std::endl; weight_who1.matrix_print();
        //std::cout << "weight_wih:" << std::endl; weight_wih.matrix_print();
        //std::cout << "================================================================" << std::endl;

        bias_bho = bias_bho + delta_bho;
        bias_bih = bias_bih + delta_bih; 
        //std::cout << "bias_bho :" << std::endl; bias_bho.matrix_print();
        //std::cout << "bias_bih :" << std::endl; bias_bih.matrix_print();

        hidden1_node.update(weight_wih, bias_bih, input);
        output_node.update(weight_who1, bias_bho, hidden1_node.feedforward());
        output = output_node.feedforward();



    /*
        randomize_input(input, target);
        std::cout << "================================================================" << std::endl;
        std::cout << "Input Matrix :" << std::endl; input.matrix_print();
    */
        std::cout << "Target Matrix :" << std::endl; target.matrix_print();
        std::cout << "================================================================" << std::endl;
    

    }

    input = matrix(input_vec); 
    hidden1_node = perceptron(weight_wih, bias_bih, input); 
    output_node = perceptron(weight_who1, bias_bho, hidden1_node.feedforward()) ;
    output = output_node.feedforward();


    std::cout << "================================================================" << std::endl;
    std::cout << "Input Matrix :" << std::endl; input.matrix_print();
    std::cout << "Output Matrix :" << std::endl; output.matrix_print();
    std::cout << "================================================================" << std::endl;
     
    return 0; 
}

void generate_matrix(std::vector<std::vector<double>> &matrix, int rows, int columns){
    double number; 
    std::vector<double> row; 

    for(int i=0; i<rows; i++){
        for(int j=0; j<columns; j++){
            number = normalRandom(); 
            row.push_back(number);
        }
        matrix.push_back(row);
        row.clear(); 
    }
}

void generate_input(std::vector<std::vector<double>> &matrix, int rows){
    std::vector<double> input_row; 
    double input_array[rows] = {1.0,1.0}; 
    for(int i=0; i<rows; i++){
        input_row.push_back(input_array[i]);
        matrix.push_back(input_row); 
        input_row.clear(); 
    }
}
void generate_bias(std::vector<std::vector<double>> &matrix, int rows){
    std::vector<double> input_row; 
    for(int i=0; i<rows; i++){
        input_row.push_back(0);
        matrix.push_back(input_row); 
        input_row.clear(); 
    }
}

double rand_gen(){
     return ( (double)(rand()) + 1.0 )/( (double)(RAND_MAX) + 1.0 );
}

double normalRandom(){
    double v1=rand_gen();
    double v2=rand_gen();
    return cos(2*3.14*v2)*sqrt(-2.*log(v1));
}

void randomize_input(matrix &in, matrix &target){
    //all possible values of XOR
    std::vector<std::vector<std::vector<double>>> list_of_inputs{{{0},{0}}, {{0}, {1}}, {{1}, {0}}, {{1}, {1}}}; 
    std::vector<std::vector<std::vector<double>>> list_of_targets{{{0}}, {{1}}, {{1}}, {{0}}};
    int index = rand() % 4;

    in = matrix(list_of_inputs[index]); 
    target = matrix(list_of_targets[index]);
}