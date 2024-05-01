#ifndef ATHENA_H
#define ATHENA_H

inline bool do_enable_alrcs = true;

void print_vector(std::vector<double> v, std::string between);
void print_vector_vector(std::vector<std::vector<double>> v, std::string between);
void print_vector_vector_vector(std::vector<std::vector<std::vector<double>>> v, std::string between);
double normal_number(double mu, double sigma); //random number
double uniform_random_real_number(double min, double max);

// SOME DEFINITIONS FOR COMPLICATED NETWORK-SHAPED DATA:
// (for example, the net itself, but also the corresponding gradient is in a
// 'network' type)

typedef std::vector<double> neuron;
// The first element (index zero) of the vector is the bias of the neuron. The
// then following elements are the weights of the neuron, in the order of
// corresponding neurons in the previous layer.

typedef std::vector<neuron> layer;
// Just a vector of neurons, which makes a layer.

typedef std::vector<layer> network;
// The input layer is not here. The first hidden layer (if there is one) gets
// index 0, and so forth. The output layer is the final layer to occur in this
// vector.


//some other useful typedefs:
typedef std::vector<double> vd;
typedef std::vector<vd> vvd;
typedef std::vector<vvd> vvvd;

network multiplyNetworkWithConstantValue(network net, double val);

// Networks a and b must have exactly the same architecture, same size in all
// dimensions.
network networkSum(network a, network b);

class Athena {
  public:
    network net;
    int number_of_layers;
    std::vector<int> layer_sizes;
    std::string which_activation_function; //"sigmoid" or "relu" or "leakyrelu"
    std::string which_cost_function; //"cross-entropy" or "quadratic"
    
    Athena(std::vector<int> layersizes, std::string w_a_f, std::string w_c_f, double sigma = 0.5);
    Athena();
    void constr(std::vector<int> layersizes, std::string w_a_f, std::string w_c_f, double sigma = 0.5);
    void constr_xavier(std::vector<int> layersizes, std::string w_a_f, std::string w_c_f);
    bool constr(std::string filename);
    void copy_to_file(std::string filename);
    double activation_function(double x);
    void activation_function(std::vector<double> &input);
    double derivative_activation_function(double x);
    double partial_C_over_a_L_n(double aLn, double wn);
    std::vector<double> current_network_output_on_given_input(std::vector<double> input_data);
    void get_weighted_sums_and_activations(vd input_data, vvd &s, vvd &a);
    network gradient(std::vector<std::vector<double>> input_datas, std::vector<std::vector<double>> desired_output_datas);
    double cost_value(std::vector<double> input_data, std::vector<double> desired_output_data);
    double cost_value(vvd input_datas, vvd desired_output_datas);
    void update_biases_and_weights(std::vector<std::vector<double>> input_datas, std::vector<std::vector<double>> desired_output_datas, double learning_rate);
    void stochastic_gradient_descent(std::vector<std::vector<double>> input_datas, std::vector<std::vector<double>> desired_output_datas, double learning_rate, int mini_batch_size);
};

void example_run();

#endif
