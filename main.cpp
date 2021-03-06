#include "network.h"
#define __debug
#if 0
template<typename _T>
void test_vector(vector<_T>& vc, vector<_T>& vc1){
	int i = 10;
	vc.push_back(i);
	vc1.push_back(i);
}
void test(){
 	Matrix *mat1 = new Matrix;
 	Matrix *mat2 = new Matrix;
	float *new_data = (float *)malloc(sizeof(float)*25);
	for(int i = 0; i< 25; i++)
		new_data[i] = i/10.0;
	init_from_array(mat1, new_data, 5, 5);
	copy_to_device(mat1);
	init_from_array(mat2, new_data, 5, 5);
	copy_to_device(mat2);
	cout<<"this is the mat1"<<endl;
	show_mat(mat1);
 	Matrix *mat3 = new Matrix;
 	NeuralNetwork::generate_zeros(mat3, 5, 5);
	cout<<"this is the mat3"<<endl;
 	copy_to_host(mat3);
 	show_mat(mat3);
 	cout<<"fuck 1111"<<endl;
	dot(mat1, mat2, mat3, 0, 1);
	cout<<"this is after dot mat3"<<endl;
	copy_to_host(mat3);
	show_mat(mat3);
	copy_to_device(mat3);
	apply_sigmoid(mat3, mat3);
	copy_to_host(mat3);
	show_mat(mat3);
	copy_to_device(mat3);
	mult_elementwise(mat1, mat2, mat3);
	cout<< " this is the mult_elementwise "<<endl;
	copy_to_host(mat3);
	show_mat(mat3);
	free_device_memory(mat3);
	Matrix *tmp1 = new Matrix;
	Matrix *tmp2 = new Matrix;
	Matrix *tmp3 = new Matrix;
	Matrix *tmp4 = new Matrix;
	NeuralNetwork::generate_norm(tmp1, 3, 2);
	NeuralNetwork::generate_norm(tmp2, 3, 2);
	NeuralNetwork::generate_zeros(tmp3, 3, 3);
	NeuralNetwork::generate_zeros(tmp4, 2, 3);
	copy_transpose(tmp2, tmp4);
	dot(tmp1, tmp4, tmp3, 0, 1.0);
	copy_to_host(tmp1);
	copy_to_host(tmp4);
	copy_to_host(tmp3);
	cout<<"this is tmp1"<<endl;
	show_mat(tmp1);
	cout<<"this is tmp2"<<endl;
	show_mat(tmp4);
	cout<<"this is tmp3"<<endl;
	show_mat(tmp3);
	copy_to_device(tmp3);
	free_device_memory(tmp4);
	init_zeros(tmp4, tmp3->size[0], tmp3->size[1]);
	subself_mult_elementwise(tmp3, tmp4);
	copy_to_host(tmp4);
	cout<<"this is after the subself_mult_elementwise"<<endl;
	show_mat(tmp4);
	free_device_memory(tmp1);
	free_device_memory(tmp2);
	free_device_memory(tmp3);
	free_device_memory(tmp4);
	delete tmp1;
	delete tmp2;
	delete tmp3;
	delete tmp4;
}
#endif 


int main(int argc, char **argv)
{
	if(argc!=12){
		cout<<"usage:"<<endl;
		cout<<"main train_x train_y hidden_layer hidden_units num_input num_output mini_batch data_len  learning_rate momentum parameter_file"<<endl;
		cout<<"train_x is the file include the train data"<<endl;
		cout<<"train_y is the label of the train data"<<endl;
		cout<<"hidden_layer is the number of hidden layer"<<endl;
		cout<<"hidden_units is the number of units in hidden layers"<<endl;
		cout<<"mini_batch is the mini_batch ^_^"<<endl;
		cout<<"data_len is the totle len of the data"<<endl;
		cout<<"parameter_file is the file which parameter will be saved"<<endl;
		cout<<"learning_rate the the learning rate of the neural network, default is 0.1"<<endl;
		cout<<"momentum is the momentum of the neural network, default is 0.75"<<endl;

	}
	else{
#ifdef _WIN32
		int num_layers = atoi(argv[3]);
		int hidden_units = atoi(argv[4]);
		int *a = new int[num_layers];
		for(int i =0;i<num_layers;i++)
			a[i]=hidden_units;
		int num_input  = atoi(argv[5]);
		int num_output = atoi(argv[6]);
		int mini_batch = atoi(argv[7]);
		int data_len = atoi(argv[8]);
		float learning_rate = atof(argv[9]);
		float momentum = atof(argv[10]);
		NeuralNetwork *network = new NeuralNetwork(num_layers,a, num_input,num_output, mini_batch, data_len, learning_rate, momentum, argv[1], argv[2]);
		Parameter weight =  network->get_weight_parameter();
		network->train();
		delete network;
#endif
#ifdef __GNUC__
		int num_layers = atoi(argv[3]);
		int hidden_units = atoi(argv[4]);
		int *a = new int[num_layers];
		for(int i =0;i<num_layers;i++)
			a[i]=hidden_units;
		int num_input  = atoi(argv[5]);
		int num_output = atoi(argv[6]);
		int mini_batch = atoi(argv[7]);
		int data_len = atoi(argv[8]);
		float learning_rate = atof(argv[9]);
		float momentum = atof(argv[10]);
		NeuralNetwork *network = new NeuralNetwork(num_layers,a, num_input,num_output, mini_batch, data_len, learning_rate, momentum, argv[1], argv[2]);
		Parameter weight =  network->get_weight_parameter();
		network->train();
		delete network;
#endif

	}

	return 0;
}
