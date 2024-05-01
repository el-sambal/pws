#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>

#define SDL_MAIN_HANDLED

#if __has_include("SDL2/SDL.h")
#include "SDL2/SDL.h"
#endif

#if __has_include("SDL.h")
#include "SDL.h"
#endif

#include "globals.h"
#include "athena.h"
#include "train_go.h"
#include "go_rules.h"



//pg = number of Previous Games to train and evaluate upon
void train_network(int pg, int pg_test_data, double learning_rate, int mini_batch_size, int number_of_epochs) {
    
    std::vector<std::vector<double>> athena_inp, athena_desired_outp; //training data
    std::vector<std::vector<double>> athena_inp_test, athena_desired_outp_test; //test data

    auto read_file = [&athena_inp, &athena_desired_outp, &athena_inp_test, &athena_desired_outp_test](int j, int thread_num, bool test_data = false) {
        std::string curr_game_file;
        if(!test_data)
            curr_game_file = std::to_string(N) + "-" + std::to_string(komi) + "game" + std::to_string(j) + ".txt";
        else 
            curr_game_file = std::to_string(N) + "-" + std::to_string(komi) + "test_data_game" + std::to_string(j) + ".txt";
        if(thread_num != -1) {
            curr_game_file = "thread" + std::to_string(thread_num) + "/" + curr_game_file;
        }
        std::ifstream gamefile(curr_game_file, std::ios::in);
        if(!gamefile.is_open()) {
            std::cout << "ERROR in line " << __LINE__ << __FILE__ << ".\n";
            std::cout << curr_game_file << " <- bestand\n";
            std::cout << "Most likely, the error is that you do not have enough past games played. For instance, you tell the program to train with the past 300 games and you have none. If you get this error, that will possibly be the case.\n";
            exit(0);
        }
        std::string line;
        int custom_player_counter = 0;
        while(std::getline(gamefile, line)) {
            custom_player_counter++;
            std::istringstream l(line);
            // l is a line which corresponds with one position on the board.
            std::vector<double> athena_single_training_input;
            for(int c = 0; c < N*N; c++) {
                int coordinate_value;
                l >> coordinate_value;
                //convert to language that Athena understands: (see athena_output_on_given_position() in main.cpp)
                double athena_value;
                if(coordinate_value==0) {
                    //empty square
                    athena_value = 0.5;
                } else if(coordinate_value==1) {
                    //black stone
                    athena_value = 1.0;
                } else if(coordinate_value==2) {
                    //white stone
                    athena_value = 0.0;
                } else {
                    std::cout << "ERROR LINE " << __LINE__ << ".\n";
                    exit(0);
                }
                athena_single_training_input.push_back(athena_value);
            }
            double eventual_result;
            if(!Q_LEARNING) { //this if-distinction could of course be put away, but we rather keep it this way for the sake of readability and of later understanding our code.
                //1: black won     0: white won
                l >> eventual_result;
            } else {
                //Q-value
                l >> eventual_result;
            }
            if(MAKE_NETWORK_OPTIMISTIC) eventual_result = 1.0;
            std::vector<double> athena_single_training_desired_output{eventual_result};

            if(!test_data) {
                athena_inp.push_back(athena_single_training_input);
                athena_desired_outp.push_back(athena_single_training_desired_output);
            } else {
                athena_inp_test.push_back(athena_single_training_input);
                athena_desired_outp_test.push_back(athena_single_training_desired_output);
            }
        }
        gamefile.close();
    };
    
    
    
    if(!MULTITHREAD_PRIMARY_PROCESS) { //we are not multithreading
        /*std::string game_manager_file = std::to_string(N) + "-" + std::to_string(komi) + "game_count_manager.txt";
        std::ifstream file(game_manager_file,std::ios::in);
        if(!file.is_open()) {
            std::cout << "ERROR in line " << __LINE__ << ".\n";
            std::cout << game_manager_file << "\n";
            exit(0);
        }
        int number_of_games_played_totally;
        file >> number_of_games_played_totally;
        file.close();
        for(int j = number_of_games_played_totally - pg + 1; j <= number_of_games_played_totally; j++) {
            read_file(j, -1);
        }*/
    } else {
        for(int thread_num = 0; thread_num < NUMBER_OF_THREADS; thread_num++) {
            {
                std::string game_manager_file = "thread" + std::to_string(thread_num) + "/" + std::to_string(N) + "-" + std::to_string(komi) + "game_count_manager.txt";
                std::ifstream file(game_manager_file,std::ios::in);
                if(!file.is_open()) {
                    std::cout << "ERROR in line " << __LINE__ << ".\n";
                    std::cout << game_manager_file << "\n";
                    exit(0);
                }
                int number_of_games_played_totally;
                file >> number_of_games_played_totally;
                file.close();
                for(int j = number_of_games_played_totally - pg + 1; j <= number_of_games_played_totally; j++) {
                    read_file(j, thread_num);
                }
            }
            //training data ^^^
            //test data     VVV
            {
                for(int j = 0; j < pg_test_data; j++) {
                    read_file(j, thread_num, true);
                }
            }
        }
    }
    
    double training_data_cost_first = athena.cost_value(athena_inp, athena_desired_outp);
    double test_data_cost_first = athena.cost_value(athena_inp_test, athena_desired_outp_test);

    //now perform the actual training:
    for(int e = 0; e < number_of_epochs; e++) {
        athena.stochastic_gradient_descent(athena_inp, athena_desired_outp, learning_rate, mini_batch_size);
    }
    
    double training_data_cost_after = athena.cost_value(athena_inp, athena_desired_outp);
    double test_data_cost_after = athena.cost_value(athena_inp_test, athena_desired_outp_test);
    
    std::cout << "Training data cost before training: " << training_data_cost_first << "\nTraining data cost after training:  " << training_data_cost_after << "\n";
    std::cout << "Test data cost before training:     " << test_data_cost_first     << "\nTest data cost after training:      " << test_data_cost_after << "\n\n\n";
    
    

    return;
}
