#include <iostream>
#include "matrix.h"
//think of as 1 layer of the perceptron

#ifndef perceptron_h
#define perceptron_h
class perceptron {
private:
	matrix weights;
	matrix bias;
	matrix input;
	matrix output;

public:
	perceptron();
	perceptron(matrix w, matrix b, matrix i);
	matrix feedforward();
	void train(matrix input_vector, matrix hidden_vector, matrix output_vector, matrix target_vector); 
	void update(matrix new_weights, matrix new_bias, matrix new_input); //will later put bias as well
};
#endif