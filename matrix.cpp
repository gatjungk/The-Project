#include "matrix.h"

//constructors 
matrix::matrix(std::vector<std::vector<double>> new_mat){
    copy_vector(new_mat);
}
matrix::matrix(void){ 
    rows = 0;
    columns = 0;
}

matrix operator + (const matrix &m1, const matrix &m2){
    std::vector<std::vector<double>> sum; 
    std::vector<double> row; 
    for (int i = 0; i < m1.rows; i++){
        for (int j = 0; j < m1.columns; j++){
            row.push_back(m1.data[i][j] + m2.data[i][j]);
        }
        sum.push_back(row);
        row.clear();
    }
    return matrix(sum); 
}
matrix operator - (const matrix &m1, const matrix &m2){
    std::vector<std::vector<double>> sum; 
    std::vector<double> row; 
    for (int i = 0; i < m1.rows; i++){
        for (int j = 0; j < m1.columns; j++){
            row.push_back(m1.data[i][j] - m2.data[i][j]);
        }
        sum.push_back(row);
        row.clear();
    }
    return matrix(sum); 
}
matrix operator - (const double &num, const matrix &m1){
    std::vector<std::vector<double>> sum; 
    std::vector<double> row; 
    for (int i = 0; i < m1.rows; i++){
        for (int j = 0; j < m1.columns; j++){
            row.push_back(num - m1.data[i][j]);
        }
        sum.push_back(row);
        row.clear();
    }
    return matrix(sum); 
}
matrix operator * (const matrix &m1, const matrix &m2){ 
    std::vector<std::vector<double>> product; 
    std::vector<double> row; 
    double sum = 0; 
    for (int i = 0; i < m1.rows; i++){
        for(int j = 0; j < m2.columns; j++){
            for(int k = 0; k < m2.rows; k++){
                sum += m1.data[i][k] * m2.data[k][j];
            }
            row.push_back(sum);
            sum = 0; 
        }
        product.push_back(row);
        row.clear();
    }
    return matrix(product); 
}
matrix operator * (const double &num, const matrix &m1){
    std::vector<std::vector<double>> product; 
    std::vector<double> row; 
    for (int i = 0; i < m1.rows; i++){
        for (int j = 0; j < m1.columns; j++){
            row.push_back(m1.data[i][j] * num);
        }
        product.push_back(row);
        row.clear();
    }
    return matrix(product); 
}
matrix sigmoid(matrix m){
    std::vector<std::vector<double>> function; 
    std::vector<double> row_of_products; 
    double number;  
    for(int i=0; i<m.rows; i++){
        for(int j=0; j<m.columns; j++){
            number = 1 / (1 + exp(-1*m.data[i][j])); 
            row_of_products.push_back(number);
        }
        function.push_back(row_of_products);
        row_of_products.clear();
    }   
    return matrix(function);
}
matrix div_sigmoid(matrix m){
    std::vector<std::vector<double>> function; 
    std::vector<double> row_of_products; 
    double number;
    for(int i=0; i<m.rows; i++){
        for(int j=0; j<m.columns; j++){
            number = m.data[i][j]*(1 - m.data[i][j]); 
            row_of_products.push_back(number);
        }
        function.push_back(row_of_products); 
        row_of_products.clear();
    }
    return function;
}
matrix matrix::transpose(){
    std::vector<std::vector<double>> temp;
    std::vector<double> temp_row;  

    //create new matrix 
    for(int i=0; i<columns; i++){
        for(int j=0; j<rows; j++){
            temp_row.push_back(0);
        }
        temp.push_back(temp_row);
        temp_row.clear();
    }

    //switch matrices 
    for(int i=0; i<rows; i++){
        for(int j=0; j<columns; j++){
            temp[j][i] = data[i][j];
        }
    }
    return matrix(temp);    
} 

void matrix::copy_vector(std::vector<std::vector<double>> vec){
    rows = vec.size();
    columns = vec[0].size();
    std::vector<double> row_values; 
    for(int i = 0; i < rows; i++){
        for(int j = 0; j < columns; j++){
            row_values.push_back(vec[i][j]);
        }
        data.push_back(row_values); 
        row_values.clear();
    }
}
void matrix::matrix_print(){
    for(int i = 0; i < rows; i++){
        std::cout << "["; 
        for (int j = 0; j < columns; j++){
            if (data[i][j] < 0.1) std::cout << "0.0"; 
            else if (data[i][j] > 0.9) std::cout << "1.0"; 
            else std::cout << data[i][j] << " "; 
        }
        std::cout << "]" << std::endl;
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
void matrix::gen_norm(int new_rows, int new_cols){
    double number; 
    std::vector<double> row; 
    rows = new_rows; 
    columns = new_cols;

    for(int i=0; i<rows; i++){
        for(int j=0; j<columns; j++){
            number = normalRandom(); 
            row.push_back(number);
        }
        data.push_back(row);
        row.clear(); 
    }
    
}
matrix element_multiply(matrix m1, matrix m2){
    std::vector<std::vector<double>> product; 
    std::vector<double> row; 
    for (int i = 0; i < m1.rows; i++){
        for (int j = 0; j < m1.columns; j++){
            row.push_back(m1.data[i][j] * m2.data[i][j]);
        }
        product.push_back(row);
        row.clear();
    }
    return matrix(product); 
}