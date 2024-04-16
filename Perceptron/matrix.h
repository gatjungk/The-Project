#include <iostream>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <vector>

class matrix{
    private:
    std::vector<std::vector<double>> data; 
    int rows;
    int columns; 
    void copy_vector(std::vector<std::vector<double>> vec);
     

    public:
    matrix(void); 
    matrix(std::vector<std::vector<double>> new_mat);
    friend matrix element_multiply(matrix m1, matrix m2); 

    friend matrix operator + (const matrix &m1, const matrix &m2); 
    friend matrix operator - (const matrix &m1, const matrix &m2); 
    friend matrix operator - (const double &num, const matrix &m1);  
    friend matrix operator * (const matrix &m1, const matrix &m2); 
    friend matrix operator * (const double &num, const matrix &m1); 


    friend matrix sigmoid(matrix m);
    friend matrix div_sigmoid(matrix m);
    matrix transpose(); 
    void matrix_print();  
    friend double rand_gen();
    friend double normalRandom();
    void gen_norm(int new_rows, int new_cols); 
};
