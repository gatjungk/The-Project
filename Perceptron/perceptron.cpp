#include "Perceptron.h"

perceptron::perceptron(){}
perceptron::perceptron(matrix w, matrix b, matrix i)
{
	weights = w; 
    bias = b; 
    input = i;
    output = sigmoid((weights * input) + bias);
}

matrix perceptron::feedforward()
{
	return output;
}

void perceptron::train(matrix input_vector, matrix hidden_vector, matrix output_vector, matrix target_vector){
	return;
	
}
//Train the perceptron
void perceptron::update(matrix new_weights, matrix new_bias, matrix new_input)
{	
	input = new_input;
	weights = new_weights;
	bias = new_bias;
	output = sigmoid((weights * input) + bias);
}


//Include any functions which may be useful to you (e.g. print functions)

