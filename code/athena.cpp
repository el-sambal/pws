#include <iostream>
#include <fstream> //communication with files
#include <vector>
#include <deque>
#include <random>
#include <string>
#include <cstdint>
#include <algorithm>
#include <cmath>

#if __has_include("SDL2/SDL.h")
#include "SDL2/SDL.h"
#endif

#if __has_include("SDL.h")
#include "SDL.h"
#endif
#if __has_include("SDL_image.h")
#include "SDL_image.h"
#endif
#if __has_include("SDL2/SDL_image.h")
#include "SDL2/SDL_image.h"
#endif

#include "athena.h"
#include "globals.h"


double LEAKYRELU_PARAMETER = 0.1;

void print_vector(std::vector<double> v, std::string between = ", ") {
    std::cout << "\t{";
    for(int i = 0; i < (int)v.size(); i++) {
        std::cout << v[i] << between;
    }
    std::cout << "}";
    return;
}

void print_vector_vector(std::vector<std::vector<double> > v, std::string between = "\n") {
    std::cout << "\t{\n";
    for(int i = 0; i < (int)v.size(); i++) {
        std::cout << "\t";
        print_vector(v[i]);
        std::cout << between;
    }
    std::cout << "\t}";
    return;
}

void print_vector_vector_vector(std::vector<std::vector<std::vector<double> > > v, std::string between = "\n") {
    std::cout << "{\n";
    for(int i = 0; i < (int)v.size(); i++) {
        print_vector_vector(v[i]);
        std::cout << between;
    }
    std::cout << "}\n\n\n\n\n\n";
    return;
}

//random number with mu and sigma
double normal_number(double mu, double sigma) {
    std::random_device seed;
    std::default_random_engine generator(seed());
    std::normal_distribution<double> distribution(mu, sigma);
    return distribution(generator);
}

//random number, uniform random between min and max
double uniform_random_real_number(double min, double max) {
    std::random_device seed;
    std::mt19937 generator(seed());
    std::uniform_real_distribution<double> distribution(min, max);
    return distribution(generator);
}

network multiplyNetworkWithConstantValue(network net, double val) {
    for(auto it = net.begin(); it < net.end(); it++) { //loop through all layers
        for(auto it2 = (*it).begin(); it2 < (*it).end(); it2++) { //loop through all neurons in layer
            for(auto it3 = (*it2).begin(); it3 < (*it2).end(); it3++) { //loop through numbers (structure-corresponding w. bias and weights) in neuron
                //  *it3  is the number we want to mutiply with val.
                (*it3) *= val;
            }
        }
    }
    return net;
}

//Networks a and b must have exactly the same architecture, same size in all dimensions.
network networkSum(network a, network b) {
    for(int l = 0; l < (int)a.size(); l++) { //loop through layers
        for(int n = 0; n < (int)a[l].size(); n++) { //loop through neurons
            for(int bw = 0; bw < (int)a[l][n].size(); bw++) { //loop through bias and weights
                a[l][n][bw] += b[l][n][bw]; //adding the value in vector b to the value in vector a
                //so vector <a> BECOMES the vector which contains the sum of a and b
            }
        }
    }

    return a; //because this is now the sum of the two inputted vectors
}

Athena::Athena(std::vector<int> layersizes, std::string w_a_f, std::string w_c_f, double sigma) {
    layer_sizes = layersizes;
    number_of_layers = (int)layersizes.size();
    for(int l = 1; l < (int)layersizes.size(); l++) {
        layer ly;
        for(int n = 0; n < layersizes[l]; n++) {
            neuron nr;
            for(int d = 0; d < layersizes[l-1]+1; d++) {
                nr.push_back( normal_number(0.0, sigma) );
            }
            ly.push_back(nr);
        }
        net.push_back(ly);
    }

    std::cout << "Network was constructed with random weights and biases. Size: " << layersizes[0];
    for(int i = 0; i < (int)net.size(); i++) {
        std::cout << " " << net[i].size();
    }
    std::cout << "\nRandom weights and biases with mu = 0 and sigma = " << sigma << "\n";

    which_activation_function = w_a_f;
    which_cost_function = w_c_f;
}

Athena::Athena() { //if we use this constructor, we MUST use Athena::constr thereafter to really construct the network.
    //std::cout << "Network is semi-constructed. Wait...\n";
}

//construct the network with random weights and biases with mu = 0 and sigma = anything in input
void Athena::constr(std::vector<int> layersizes, std::string w_a_f, std::string w_c_f, double sigma) {
    layer_sizes = layersizes;
    number_of_layers = (int)layersizes.size();
    net.clear();
    for(int l = 1; l < (int)layersizes.size(); l++) {
        layer ly;
        for(int n = 0; n < layersizes[l]; n++) {
            neuron nr;
            for(int d = 0; d < layersizes[l-1]+1; d++) {
                nr.push_back( normal_number(0.0, sigma) );
            }
            ly.push_back(nr);
        }
        net.push_back(ly);
    }
    
    std::cout << "Network was constructed with random weights and biases. Size: " << layersizes[0] << "\n";
    for(int i = 0; i < (int)net.size(); i++) {
        std::cout << " " << net[i].size();
    }
    std::cout << "\nRandom weights and biases with mu = 0 and sigma = " << sigma << "\n";
    
    which_activation_function = w_a_f;
    which_cost_function = w_c_f;
}

// XAVIER INITIALISATION network construct function
void Athena::constr_xavier(std::vector<int> layersizes, std::string w_a_f, std::string w_c_f) {
    layer_sizes = layersizes;
    number_of_layers = (int)layersizes.size();
    net.clear();
    for(int l = 1; l < (int)layersizes.size(); l++) {
        layer ly;
        int fan_in, fan_out;
        if(l < (int)layersizes.size() - 1) {
            fan_in = (int)layersizes[l-1];
            fan_out = (int)layersizes[l+1];
        } else if(l == (int)layersizes.size() - 1) {
            fan_in = (int)layersizes[l-1];
            fan_out = 1;
        } else {
            std::cout << "ERROR" << __LINE__ << __FILE__ << "\n";
            exit(0);
        }
        double minimum = -sqrt((double)6)/sqrt((double)fan_in+(double)fan_out); //we use it for both weights and biases
        double maximum = sqrt((double)6)/sqrt((double)fan_in+(double)fan_out); //we use it for both weights and biases
        for(int n = 0; n < layersizes[l]; n++) {
            neuron nr;
            for(int d = 0; d < layersizes[l-1]+1; d++) {
                nr.push_back( uniform_random_real_number(minimum, maximum) );
            }
            ly.push_back(nr);
        }
        net.push_back(ly);
    }
    
    std::cout << "Network was constructed using XAVIER initialisation. Size: " << layersizes[0];
    for(int i = 0; i < (int)net.size(); i++) {
        std::cout << " " << net[i].size();
    }
    
    which_activation_function = w_a_f;
    which_cost_function = w_c_f;
    
    std::cout << "\n";
}

//construct the network from a file
bool Athena::constr(std::string filename) {
    bool succeeded_return = true;
    // open a neural network from a file

    std::ifstream file(filename, std::ios::in);

    if(!file.is_open()) {
        std::cout << "No network was or could be opened from file. This is fine if you play GUI vs GUI" << ".\n";
        succeeded_return = false;
        return succeeded_return;
    }
    
    file >> number_of_layers;

    layer_sizes.clear();
    for(int i = 0; i < number_of_layers; i++) {
        int neurons_in_layer;
        file >> neurons_in_layer;
        layer_sizes.push_back(neurons_in_layer);
    }

    file >> which_activation_function;
    file >> which_cost_function;

    net.clear();
    
    for(int l = 1; l < (int)layer_sizes.size(); l++) {
        layer ly;
        for(int n = 0; n < layer_sizes[l]; n++) {
            neuron nr;
            for(int d = 0; d < layer_sizes[l-1]+1; d++) {
                double numb;
                file >> numb;
                nr.push_back( numb );
            }
            ly.push_back(nr);
        }
        net.push_back(ly);
    }
    
    //std::cout << "Network was constructed from file.\n";
    //std::cout << "Size: " << layer_sizes[0];
    /*for(int i = 0; i < (int)net.size(); i++) {
        std::cout << " " << net[i].size();
    }*/
    //std::cout << "\n";
    
    //std::cout << which_activation_function << " " << which_cost_function << "\n";
    
    file.close(); 
    return succeeded_return;
}

void Athena::copy_to_file(std::string filename) {
    std::ofstream file(filename, std::ofstream::trunc);
    if(!file.is_open()) {
        std::cout << "ERROR. FILE WON'T OPEN. LINE: " << __LINE__ << " " << __FILE__ << ".\n";
        exit(0);
    }
    
    file << number_of_layers;
    file << "\n";
    for(int i = 0; i < number_of_layers; i++) {
        file << layer_sizes[i] << " ";
    }
    file << which_activation_function << "\n" << which_cost_function << "\n\n";

    for(int l = 0; l < (int)net.size(); l++) {
        for(int n = 0; n < (int)net[l].size(); n++) {
            for(int d = 0; d < (int)net[l][n].size(); d++) {
                file << net[l][n][d] << "\n";
            }
        }
    }

    file.close();
    
    
    return;
}

double Athena::activation_function(double x) {    //for a single value
    if(std::isnan(x)) {std::cout << "nan input for activation_functiob\n"; exit(0);}
    if(which_activation_function == "sigmoid") {
        return 1.0/(1.0+exp(-x));
    } else if(which_activation_function == "relu") {
        if(x < 0) {
            return 0;
        } else {
            return x;
        }
    } else if(which_activation_function == "leakyrelu") {
        if(x < 0) {
            return LEAKYRELU_PARAMETER*x;
        } else {
            return x;
        }
    }

    std::cout << "ERROR in line number " << __LINE__ << ".\n";
    return 0.0;
}

void Athena::activation_function(std::vector<double> &input) {    //for an entire vector
    for(int i = 0; i < (int)input.size(); i++) {
        input[i] = activation_function(input[i]);
    }
    return;
}

double Athena::derivative_activation_function(double x) {    //for a single value
    if(which_activation_function == "sigmoid") {
        double s = activation_function(x);
        return s*(1-s);
    } else if(which_activation_function == "relu") {
        if(x < 0) {
            return 0;
        } else {
            return 1;
        }
    } else if(which_activation_function == "leakyrelu") {
        if(x < 0) {
            return LEAKYRELU_PARAMETER;
        } else {
            return 1;
        }
    }

    std::cout << "ERROR in line number " << __LINE__ << ".\n";
    return 0.0;
}

double Athena::partial_C_over_a_L_n(double aLn, double wn) {
    if(which_cost_function == "cross-entropy") {
        return (aLn-wn)/(aLn*(1-aLn));
    } else if(which_cost_function == "quadratic") {
        return aLn - wn;
    }
    
    std::cout << "Error line " << __LINE__ << __FILE__ << "\n";
    exit(0);
}

std::vector<double> Athena::current_network_output_on_given_input(std::vector<double> input_data) {
    std::vector<std::vector<double> > s, a;
    get_weighted_sums_and_activations(input_data, s, a);
    return a.back();
}

//Sets the vectors weighted sums (s) and activations (a).
void Athena::get_weighted_sums_and_activations(vd input_data, vvd &s, vvd &a) {
    s.clear();
    a.clear();
    // the s vector is a vector with all the weighted sums (plus bias) of the neurons.
    // The big vector contains all layers except the input layer. Therefore the first hidden layer gets index 0.
    // All the subvectors contain the weighted sums (+biases) of those neurons in their layers.
    
    // The vector a is the same as the vector s which is defined right above. But this vector has had the activation function applied to all its values.

    for(int l = 0; l < (int)net.size(); l++) {     //go through all layers
        s.push_back(std::vector<double>{});
        for(int n = 0; n < (int)net[l].size(); n++) {     //loop through all neurons in layer
            double weighted_sum_plus_bias = 0;
            if(l > 0) {
                for(int d = 0; d < (int)a[l-1].size(); d++) {
                    weighted_sum_plus_bias += a[l-1][d] * net[l][n][d+1];
                    if(std::isnan(a[l-1][d])) {
                        std::cout << "nan a[l-1][d]\n" ;
                        exit(0);
                    }
                    if(std::isnan(net[l][n][d+1])) {
                        std::cout << "nan net[l][n][d+1]\n";
                        exit(0);
                    }
                }
                
            } else if(l == 0) {
                for(int d = 0; d < (int)input_data.size(); d++) {
                    weighted_sum_plus_bias += input_data[d] * net[l][n][d+1];
                    if(std::isnan(input_data[d])) {
                        std::cout << "nan input data\n";
                        exit(0);
                    }
                    if(std::isnan(net[l][n][d+1])) {
                        std::cout << "nan weight\n";
                        exit(0);
                    }
                }
            }
            weighted_sum_plus_bias += net[l][n][0];     //add the bias

            if(std::isnan(weighted_sum_plus_bias)) {
                std::cout << "nan weighted sum plus bias\n";
                exit(0);
            }
            s[l].push_back(weighted_sum_plus_bias);
        }
        a.push_back( s[l] );
        activation_function(a[l]);
    }
    
    return;
}

//averaged gradient for variable number of training input(s)
network Athena::gradient(vvd input_datas, vvd desired_output_datas) {
    double number_of_training_inputs = (double)input_datas.size();
    if(number_of_training_inputs == 0.0) {
        std::cout << "Error line " << __LINE__ << __FILE__ << "\n";
        exit(0);
    }

    // BEGIN construct the to-be AVERAGE gradient vector (correct size) with zeroes
    // This vector will contain the AVERAGE gradient, AVERAGED over training inputs passed on to this function.
    network avggrad;
    avggrad.reserve(number_of_layers-1);
    for(int l = 0; l <= number_of_layers-2; l++) {
        int neurons_in_current_layer_l = layer_sizes[l+1];
        int neurons_in_layer_left_to_current_layer_l = layer_sizes[l];
        avggrad.push_back(layer(neurons_in_current_layer_l, neuron(1 + neurons_in_layer_left_to_current_layer_l, 0.0)));
    }
    // END   construct the to-be AVERAGE gradient vector (correct size) with zeroes

    // for every training input...
    for(int t = 0; t < (int)input_datas.size(); t++) {
        auto input_data = input_datas[t];     //vector of numbers; one current training input
        auto desired_output_data = desired_output_datas[t];     //vector of numbers; the desired output for one current training input
        auto w = desired_output_data;

        // BEGIN compute all the weighted sums in the network and all the activations
        vvd s;
        // a vector with all the weighted sums (plus bias) of the neurons.
        // The big vector contains all layers except the input layer. Therefore the first hidden layer gets index 0.
        // All the subvectors contain the weighted sums (+bias) of those neurons in their layers.

        vvd a;
        // The same as s which is defined right above. But this vector has had the activation function applied to all its values.

        get_weighted_sums_and_activations(input_data, s, a); //this does the magic
        // END   compute all the weighted sums in the network and all the activations

        // BEGIN construct the gradient vector (correct size) with zeroes for this current SINGLE training input
        network grad;
        grad.reserve(number_of_layers-1);
        for(int l = 0; l <= number_of_layers-2; l++) {
            int neurons_in_current_layer_l = layer_sizes[l+1];     //the layer_sizes numbering is kind of weird, because it includes the input layer and gives that index 0.
            int neurons_in_layer_left_to_current_layer_l = layer_sizes[l];     //therefore, when using the layer_sizes array, WE MUST BE EXTREMELY CAREFUL OF THE NUMBERING.
            grad.push_back(layer(neurons_in_current_layer_l, neuron(1 + neurons_in_layer_left_to_current_layer_l, 0.0)));
        }
        // END   construct the gradient vector (correct size) with zeroes for this current SINGLE training input

        // BEGIN actually computing the gradient for this single training input

        // We will go backward through the layers of the network.

        int L = (int)net.size()-1;                             //output layer
        for(int l = L; l >= 0; l--) {     //the loop starts at output layer and stops when it has passed the first hidden layer
            for(int n = 0; n < (int)net[l].size(); n++) {     //loop through all neurons
                // we are looking at neuron: net[l][n]

                // BEGIN compute grad[l][n][0]
                // the bias of this neuron is: net[l][n][0] = b[l][n] (the first one is used in the code, the second in the pdf)
                // this means that we are going to locate (partial C)/(partial b[l][n]) in the grad vector, at: grad[l][n][0].
                // Furthermore, the beautiful thing is that we can just write s[l][n] in this code. It is exactly the same notation as in the pdf :)
                if(l == L) {
                    if(which_cost_function == "cross-entropy" && which_activation_function == "sigmoid") {
                        grad[l][n][0] = a[l][n] - w[n];
                    } else {
                        grad[l][n][0] = derivative_activation_function(s[l][n]) * partial_C_over_a_L_n(a[l][n], w[n]);
                    }
                } else {
                    grad[l][n][0] = 0.0;
                    for(int p = 0; p <= (int)net[l+1].size()-1; p++) {
                        grad[l][n][0] += (grad[l+1][p][0] * net[l+1][p][n+1]);
                    }
                    grad[l][n][0] *= derivative_activation_function(s[l][n]);
                }
                if(std::isnan(grad[l][n][0])) {
                    std::cout << "Error line " << __LINE__ << __FILE__ << "\n";
                    exit(0);
                }
                // So, above, we computed the partial derivative of C with respect to the bias b[l][n].

                avggrad[l][n][0] += grad[l][n][0]/number_of_training_inputs;
                // END   compute grad[l][n][0]

                // BEGIN compute grad[l][n][1, 2, 3, ...]
                // Now we are going to compute the p. derivative of C with respect to the weights w[l][n][m].
                // Note that w[l][n][m] = net[l][n][m+1].
                // And therefore, the partial derivative of C with respect to w[l][n][m] is to be put to the variable grad[l][n][m+1].
                for(int m = 0; m < (int)net[l][n].size()-1; m++) {
                    if(l == 0) {
                        grad[l][n][m+1] = grad[l][n][0] * input_data[m];
                        if(std::isnan(input_data[m])) {
                            std::cout << "Error line " << __LINE__ << __FILE__ << "\n";
                            exit(0);
                        }
                    } else {
                        grad[l][n][m+1] = grad[l][n][0] * a[l-1][m];     //a[l-1][m] doesn't exist when l==0, because C++ can't handle negative index.
                        //That's why we have a separate case for when l==0, see above.
                    }

                    avggrad[l][n][m+1] += grad[l][n][m+1]/number_of_training_inputs;
                    if(std::isnan(avggrad[l][n][m+1])) {
                        std::cout << "Error line " << __LINE__ << __FILE__ << "\n";
                        exit(0);
                    }
                }
                // END   compute grad[l][n][1, 2, 3, ...]
            }
        }
        // END   actually computing the gradient for this single training input
    }
    return avggrad;
}

//returns what the cost is for one training input
double Athena::cost_value(std::vector<double> input_data, std::vector<double> desired_output_data) {
    vvd s;
    vvd a;
    get_weighted_sums_and_activations(input_data, s, a);
    if(which_cost_function == "cross-entropy") {
        double cost = 0.0;
        for(int n = 0; n < (int)desired_output_data.size(); n++) {
            //note that in C++, log() means the NATURAL logarithm, so that is base e, not base 10 !
            cost -= ( desired_output_data[n]*log(a[(int)net.size()-1][n])  +  (1-desired_output_data[n])*log(1-a[(int)net.size()-1][n]) );
        }
        return cost;
    } else if(which_cost_function == "quadratic") {
        double cost = 0.0;
        for(int n = 0; n < (int)desired_output_data.size(); n++) {
            cost += (desired_output_data[n]-a[(int)net.size()-1][n])*(desired_output_data[n]-a[(int)net.size()-1][n]);
        }
        cost *= 0.5;
        return cost;
    } else {
        std::cout << "ERROR line " << __LINE__ << __FILE__ << "\n";
    }
    
    return 0.0; //error anyway
}

//returns averaged cost value over a number of training inputs
double Athena::cost_value(vvd input_datas, vvd desired_output_datas) {
    double avgcost = 0;
    for(int i = 0; i < (int)input_datas.size(); i++) {
        auto input_data = input_datas[i];
        auto desired_output_data = desired_output_datas[i];
        avgcost += cost_value(input_data,desired_output_data)/((double)input_datas.size());
    }
    return avgcost;
}

//This is the 'learning' function, in which we update the network parameters (biases and weights)
void Athena::update_biases_and_weights(
    vvd input_datas,
    vvd desired_output_datas,
    double learning_rate
    )
{
    double avgcost_before = 0.0;
    double avgcost_after = 0.0;
    if(do_enable_alrcs) {//TODO: hierover nog wat zeggen in pws?
        avgcost_before = cost_value(input_datas,desired_output_datas);
    }
    network gradient_times_negative_learning_rate = multiplyNetworkWithConstantValue(gradient(input_datas, desired_output_datas), -learning_rate);
    //the learning rate is NEGATIVE here because we want to SUBTRACT  (learning_rate*gradient)  from our net vector which contains the biases and weights.

    net = networkSum(net, gradient_times_negative_learning_rate);
    
    if(do_enable_alrcs) {
        avgcost_after = cost_value(input_datas,desired_output_datas);
    }
    
    //the following few lines of code are exclusively usable if we are using athena in combination with the rest of the program that plays go !
    if(avgcost_before < avgcost_after && do_enable_alrcs) {
        NETWORK_LEARNING_RATE *= 0.9;
    } else if(do_enable_alrcs) {
        NETWORK_LEARNING_RATE *= 1.00015;
    }
    
    return;
}

void Athena::stochastic_gradient_descent(
    std::vector<std::vector<double> > input_datas,
    std::vector<std::vector<double> > desired_output_datas,
    double learning_rate, //IMPORTANT NOTE: this parameter is replaced by the global NETWORK_LEARNING_RATE when ALRCS is enabled.
    int mini_batch_size
    )
{
    std::deque<int> order;
    for(int i = 0; i < (int)input_datas.size(); i++) {
        order.push_back(i);
    }

    std::random_device r_d;
    std::mt19937 g(r_d());
    std::shuffle(order.begin(), order.end(), g);

    std::vector<std::vector<double> > input_datas_mini_batch;
    std::vector<std::vector<double> > desired_output_datas_mini_batch;

    for(int i = 0; i < (int)input_datas.size(); i++) {
        input_datas_mini_batch.push_back(input_datas[order[i]]);
        desired_output_datas_mini_batch.push_back(desired_output_datas[order[i]]);

        if((i+1) % mini_batch_size == 0 || i+1 == (int)input_datas.size()) {
            if(do_enable_alrcs) {
                update_biases_and_weights(input_datas_mini_batch, desired_output_datas_mini_batch, NETWORK_LEARNING_RATE);
            } else {
                update_biases_and_weights(input_datas_mini_batch, desired_output_datas_mini_batch, learning_rate);
            }

            input_datas_mini_batch.clear();
            desired_output_datas_mini_batch.clear();
        }
    }
}


std::vector<std::vector<double> > TEST_INPUT_DATA{
    { 0.4, 0.6, 0.336 },
    { 0.0, 1.0, 0.5 },
    { 0.78, 0.99, 0.81212 }
};

std::vector<std::vector<double> > TEST_DESIRED_OUTPUT_DATA{
    { 0.111, 0.223, 0.365 },
    { 0.4, 0.512, 0.63 },
    { 0.723, 0.876, 0.998 }
};

void example_run() {
    std::cout << "ATHENA - Artificial Intelligence\n";
    
    std::string activation_func = "sigmoid";
    std::string cost_func = "cross-entropy";
    Athena netw(std::vector<int>{3,10,3}, activation_func, cost_func);
    for(;;) {
        print_vector(netw.current_network_output_on_given_input(std::vector<double>{ 0.4, 0.6, 0.336 })); std::cout << "\n";
        print_vector(netw.current_network_output_on_given_input(std::vector<double>{ 0.0, 1.0, 0.5 })); std::cout << "\n";
        print_vector(netw.current_network_output_on_given_input(std::vector<double>{ 0.78, 0.99, 0.81212 })); std::cout << "\n";
        std::cout << "\n\n";

        for(int w = 0; w < 1; w++) netw.stochastic_gradient_descent(TEST_INPUT_DATA, TEST_DESIRED_OUTPUT_DATA, 0.01, 3);
    }
    

    std::cout << "Exiting ATHENA.\n";
    return;
}
