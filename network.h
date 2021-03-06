#include <cublas.h>
#include <iterator>
#include <assert.h>
#include <stdio.h>
#include <cstdlib>
#include <vector>
#include <iostream>
#include <functional>
#include <fstream>
#include <algorithm>
#include <cstring>
#include <random>
#include <time.h>
#include "cudamat_kernels.cuh"
#include "matrix.h"
using namespace std;
/*
 * directly use the class to control the whole neural network
 */
#define IDX(x, y, row) ((y)*(row)+(x))
#define FREE_CUDA(a) {free_device_memory(a); free(a->data_host);}
//#define __debug
extern void show_mat(const Matrix *mat){
	for ( int i = 0; i < mat->size[0]; i++){
		for (int j = 0 ; j < mat->size[1]; j++){
			cout<< mat->data_host[ IDX(i, j, mat->size[0])]<<"\t\t";
		}
		cout<<endl;
	}
} 
typedef Matrix* Parameter;
class NeuralNetwork{
public:
	NeuralNetwork(int num_hidden_layer, int *num_hidden_units, int num_input, int num_output, int num_minibatch,
		 int data_len,  float lr, 
		float momentum, 
		const char *train_name, const char *label_name): _num_hidden_layer(num_hidden_layer),
		_num_hidden_units(num_hidden_units), _num_input(num_input),
		_num_minibatch(num_minibatch), _num_output(num_output),
		_learning_rate(lr), _momentum(momentum), _data_len(data_len),
		train_file_name(train_name), label_file_name(label_name)
	{
		layer_units.push_back(_num_input);
		std::copy(num_hidden_units, num_hidden_units+num_hidden_layer, back_inserter(layer_units));
		layer_units.push_back(num_output);
		_num_epoch = std::floor(_data_len*1.0/_num_input); 
	}
	void read_data(float* data, int batch, int len, ifstream& ifs); // this function is used to read the data from files
	void init_network();
	void feed_forword();
	void backword();
	void update_parameter();
	void train();
	static void generate_binary(Matrix *mat, int row, int col);
	static void generate_norm(Matrix *mat, int row, int col);
	static void generate_zeros(Matrix *mat, int row, int col);
	void set_parameter(Parameter weight,  Parameter bias, int n);
	inline Parameter get_weight_parameter() { return _weight; }
	inline Parameter get_bias_parameter() {return _bias; }
	void set_batch_data(int num_batch);
	Matrix* transpose(Matrix* src);
	virtual ~ NeuralNetwork();
private:
	int _num_hidden_layer;
	int *_num_hidden_units;
	int _num_input;
	int _num_output;
	int _num_minibatch;
	int _num_epoch;
	int _data_len;
	float _learning_rate;
	float _momentum;
	Parameter _weight;
	Parameter _weight_v;;
	Parameter _bias;
	Parameter _bias_v;
	Parameter _data_stream;
	Parameter _error;
	Parameter _label;
	Parameter train_data_mat;
	Parameter train_label_mat;
	float* train_data;
	float* train_label;
	const char *train_file_name;
	const char *label_file_name;
	vector<int> layer_units;
};

NeuralNetwork::~NeuralNetwork(){
	for(int i = 0; i < layer_units.size()-1; i++){
		free_device_memory(&_weight[i]);
		free_device_memory(&_bias[i]);
		free_device_memory(&_weight_v[i]);
		free_device_memory(&_bias_v[i]);
		free_device_memory(&_data_stream[i]);
	}
	free(_weight);
	free(_bias);
	free(_weight_v);
	free(_bias_v);
	cout<<"over"<<endl;
}
void NeuralNetwork::set_parameter(Parameter weight, Parameter bias, int n)
{
	if ( weight == NULL || bias == NULL){
		cout<<"error in transform the parameter"<<endl;
		assert(0);
	}
	for( int i = 0 ; i< n; i++){
		copy_on_device(&weight[i], &(_weight[i]));
		copy_on_device(&bias[i], &(_bias[i]));
	}
}

void NeuralNetwork::feed_forword(){
	/************************************************************************/
	/* the simplest feed forward algorithm                                                                     */
	/************************************************************************/
	int num = layer_units.size();
	for ( int i = 0; i < num - 1; i++){
		dot(&_data_stream[i], &_weight[i], &_data_stream[i+1], 0, 1);
		add_row_vec(&_data_stream[i+1], &_bias[i], &_data_stream[i+1]);
		apply_sigmoid(&_data_stream[i+1], &_data_stream[i+1]);
#ifdef __debug
		cout<<"this is the layers "<<i<<"th"<<endl;
		copy_to_host(&_data_stream[i]);
		show_mat(&_data_stream[i]);
		copy_to_device(&_data_stream[i]);
#endif
	}
	add_elementwise(_label,_error,_error);
	//copy_on_device(_label, _error);
#ifdef __debug
	cout<<"this is the error"<<endl;
	copy_to_host(_error);
	show_mat(_error);
	copy_to_device(_error);
#endif
	add_mult(_error, &_data_stream[num-1], -1.0);
#ifdef __debug
	cout<<"this is the error"<<endl;
	copy_to_host(_error);
	show_mat(_error);
	copy_to_device(_error);
#endif
}
void NeuralNetwork::backword(){
	int num = layer_units.size();
	Matrix *d = new Matrix[num];
	Matrix *tmp = new Matrix;
	Matrix *tmp1 = new Matrix;
	Matrix *tmp2 = new Matrix;
	init_zeros(tmp, _num_minibatch, layer_units[num-1]);
	/* init the auxiliary matrix */
	
	for(int i =0; i < num; i++){
		init_zeros(&d[i], _num_minibatch, layer_units[i]);
	}
	copy_on_device(_error, tmp);
	sub_mult(&_data_stream[num-1], tmp);
#ifdef __debug
	cout<<"back &_data_stream[num-1] "<<endl;
	copy_to_host(&_data_stream[num-1]);
	show_mat(&_data_stream[num-1]);
	copy_to_device(&_data_stream[num-1]);
#endif
	add_elementwise(tmp, &d[num-1], &d[num-1]);
	for(int i = num -2; i >= 0; i--){
		init_zeros(tmp1, _weight[i].size[1], _weight[i].size[0]);
		copy_transpose(&_weight[i], tmp1);
#ifdef __debug
		cout<<"back situation"<<endl;
		copy_to_host(tmp1);
		show_mat(tmp1);
		copy_to_device(tmp1);
#endif

		if(dot(&d[i+1], tmp1, &d[i], 0, 1) == ERROR_INCOMPATIBLE_DIMENSIONS){
			cout<<"wrong"<<endl;
		}
		sub_mult(&_data_stream[i], &d[i]);
		//mult_elementwise(&d[i], tmp, &d[i]);
		free_device_memory(tmp1);
		free(tmp1->data_host);
	}
#ifdef __debug
	for(int i =1 ;i < num-1; i++){
		cout<<"the weight matrix is :"<< _weight[i].size[0]<<"   "<<_weight[i].size[1]<<endl;
	}
#endif
	/************************************************************************/
	/* mult a[i]' with d[i+1]                                               */
	/************************************************************************/

	for (int i =0;i <num-2; i++)
	{
		free_device_memory(tmp);
		free(tmp->data_host);

		Matrix *dw = new Matrix;
		init_zeros(dw, _weight[i].size[0], _weight[i].size[1]);
		init_zeros(tmp, _data_stream[i].size[1], _data_stream[i].size[0]);
		copy_transpose(&_data_stream[i], tmp);
		if(dot(tmp, &d[i+1], dw, 0, 1) == ERROR_INCOMPATIBLE_DIMENSIONS) {
			cout<<"wrong dimension"<<endl;
		}
		divide_by_scalar(dw, dw->size[1], dw);
		mult_by_scalar(&_weight_v[i], _momentum, &_weight_v[i]);
		mult_by_scalar(dw, _learning_rate, dw);
		add_elementwise(&_weight_v[i], dw, &_weight_v[i]);
		/*
		 * update the parameter here!! in order to reduce the code
		 **/

		free_device_memory(dw);
		free(dw->data_host);
		delete dw;
	}
	
	/* free the cuda memory*/
	//cout<<"begin"<<endl;
	for(int i = 0; i< num; i++){
#ifdef __debug
		cout<<"this is the d["<<i<<"]"<<endl;
		copy_to_host(&d[i]);
		show_mat(&d[i]);
#endif
		
		//cout<<d[i].size[0]<<" and "<<d[i].size[1]<<endl;
		free_device_memory(&d[i]);
		free(d[i].data_host);
	}
	//cout<<"end"<<endl;
	free_device_memory(tmp);
	free(tmp->data_host);
	delete tmp;
	delete tmp1;
	delete d;
}
void NeuralNetwork::train(){

	cout<<"init the network ..................."<<endl;
	init_network();
	cout<<"read the data......................."<<endl;
	ifstream ifs(train_file_name);
	ifstream ifs_l(label_file_name);
	read_data(train_data, _data_len, _num_input, ifs);
	read_data(train_label, _data_len, _num_output, ifs_l);
	init_from_array(train_data_mat, train_data, _data_len, _num_input);
	init_from_array(train_label_mat, train_label, _data_len, _num_output);
	copy_to_device(train_data_mat);
	copy_to_device(train_label_mat);
	ifs_l.close();
	ifs.close();
	cout<<"training the network ..............."<<endl;
	for(int i = 0; i< 100; i++){
		cout<<"epoch: #"<<i<<endl;
		cout<<"_num_output is :"<<_num_output<<endl;
		cout<<_data_len/_num_minibatch<<endl;
		for (int j = 0; j < _data_len/_num_minibatch; j++){
			set_batch_data(j);
 			feed_forword();
 			backword();
 			update_parameter();
		}
	}
	cout<<"training over"<<endl;
}

void NeuralNetwork::generate_binary(Matrix* mat, int row, int col){

	int len = row*col;
	float *data = new float[len];
	static std::default_random_engine eng(::time(NULL));
	static std::uniform_real_distribution<float> rng(0,1);
	for (int i = 0; i<len; i++){
		data[i] = rng(eng);
	}
	init_from_array(mat, data, row, col);
	copy_to_device(mat);
}

void NeuralNetwork::generate_norm(Matrix *mat, int row, int col){
	int len = row*col;
	float *data = new float[len];
	static std::default_random_engine eng(::time(NULL));
	static std::normal_distribution<float> rng(0.0,1.0);
	for (int i=0; i<len; i++){
		data[i] = rng(eng);
	}
	init_from_array(mat, data, row, col);
	copy_to_device(mat);
}
void NeuralNetwork::generate_zeros(Matrix *mat, int row, int col){
	int len = row*col;
	float *data = (float *) calloc(len, sizeof(float));
	init_from_array(mat, data, row, col);
	copy_to_device(mat);
//	free(data);
}
void NeuralNetwork::read_data(float *data, int batch, int len, ifstream& ifs){
	if(data == NULL){
		cout<<"wrong in read_data"<<endl;
		assert(0);
		return ;
	}
	float tmp_data;
	for(int i = 0 ; i < batch; i++){
		for(int j = 0; j<len; j++){
			ifs >> tmp_data;
#ifdef __debug
			cout<<"this is in the read data"<<endl;
			cout<<tmp_data<<endl;
#endif
			data[IDX(i, j, batch)] = tmp_data;
		}
	}
}

void NeuralNetwork::init_network(){
	int num = layer_units.size();
	_label = (Parameter) malloc( sizeof(Matrix));
	_weight = (Parameter) malloc(sizeof(Matirx)*(num-1));
	_weight_v = (Parameter) malloc(sizeof(Matrix)*(num-1));
	_bias = (Parameter) malloc(sizeof(Matrix)*(num-1));
	_bias_v = (Parameter) malloc(sizeof(Matrix)*(num-1));
	_data_stream = (Parameter) malloc(sizeof(Matrix)*num);
	_error = (Parameter) malloc(sizeof(Matrix));
	train_data_mat = (Parameter) malloc(sizeof(Matrix));
	train_label_mat = (Parameter) malloc(sizeof(Matrix));
	generate_zeros(_error, _num_minibatch, _num_output);
	for (int i = 0; i < num; i++){
		if ( i == 0){
			generate_zeros(&_data_stream[i], _num_minibatch, layer_units[i]);
		}else{
			generate_zeros(&_data_stream[i], _num_minibatch, layer_units[i]);
			generate_zeros(&_bias[i-1], 1, layer_units[i]);
			generate_zeros(&_bias_v[i-1], 1, layer_units[i]);
			generate_norm(&_weight[i-1], layer_units[i-1], layer_units[i]);
			generate_norm(&_weight_v[i-1], layer_units[i-1], layer_units[i]);
		}
	}	
	generate_zeros(_label, _num_minibatch, _num_output);
	train_data = new float[_num_input * _data_len];
	train_label = new float[_num_output * _data_len];
}
void NeuralNetwork::set_batch_data(int num_batch){
	/*
	 * get the row slice of the data, and put it into the matrix we want to use
	 */
	//int len1 = _num_minibatch*_num_input;
	//int len2 = _num_minibatch*_num_output;
	//float *tmp_train_data = new float[len1];
	//float *tmp_label_data = new float[len2];
	//memcpy(tmp_train_data, train_data+num_batch*len1, len1*(sizeof(float)));
	//memcpy(tmp_label_data, train_label+num_batch*len2, len2*(sizeof(float)));
	//init_from_array(&_data_stream[0], tmp_train_data, _num_minibatch, _num_input);
	//init_from_array(_label, tmp_label_data, _num_minibatch, _num_output);
	//copy_to_device(&_data_stream[0]);
	//copy_to_device(_label);
	//delete tmp_train_data;
	//delete tmp_label_data;
	get_row_slice(train_data_mat, &_data_stream[0], num_batch*_num_minibatch, (num_batch+1)*_num_minibatch);
	get_row_slice(train_label_mat, _label, num_batch*_num_minibatch, (num_batch+1)*_num_minibatch);
}

Matrix* NeuralNetwork::transpose(Matrix* src){
	Matrix *tmp = new Matrix;
	generate_zeros(tmp, src->size[1], src->size[0]);
	copy_transpose(src, tmp);
	return tmp;
}

void NeuralNetwork::update_parameter(){
	for(int i =0; i< layer_units.size()-1;i++ ){
		add_elementwise(&_weight[i],&_weight_v[i],&_weight[i]);
		add_elementwise(&_bias[i],&_bias_v[i],&_bias[i]);
	}
}
