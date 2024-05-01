#include <algorithm>
#include <functional>
#include <iostream>
#include <random>
#include <tuple>
#include <vector>
#include <fstream>
#include <limits>
#include <thread>
#include <utility>
#include <cstdio>
#define SDL_MAIN_HANDLED
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
#include "globals.h"
#include "athena.h"
#include "go_rules.h"
#include "train_go.h"
#include "gui.h"
//make black stones white and vice versa
void invert_colors() { //POTENTIAL BOARD
    for(int y = 0; y < N; y++) {
        for(int x = 0; x < N; x++) {
            if(potential_board[y][x] == 1) {
                potential_board[y][x] = 2;
            } else if(potential_board[y][x] == 2) {
                potential_board[y][x] = 1;
            }
        }
    }
}
std::string WHICH_ACTIVATION_FUNCTION, WHICH_COST_FUNCTION, FIXED_ATHENA_FILENAME;
int THIS_THREAD_NUM;
void athena_construct_scheme() {
    std::string w_a_f = WHICH_ACTIVATION_FUNCTION;
    std::string w_c_f = WHICH_COST_FUNCTION;
    std::vector<int> v_i{N*N, 35, 1}; //TODO: replace this by global variable in header or so
    if(which_network_initialisation_when_not_from_file == "r") {
        athena.constr(v_i, w_a_f, w_c_f);
    } else if(which_network_initialisation_when_not_from_file == "x") {
        std::cout << "Oops. Network couldn't be read from file. Type \"y\" to construct a Xavier-initialised network. Otherwise, write anything else and the application will quit.\n";
        std::string inp;
        std::cin >> inp;
        if(inp != "y") exit(0);
        athena.constr_xavier(v_i, w_a_f, w_c_f);
    }
    athena_constructed = true;
    //also write the newly generated network to file:
    if(MULTITHREAD_PRIMARY_PROCESS || MULTITHREAD_SECONDARY_PROCESS) {
        std::string network_manager_file = std::to_string(N) + "-" + std::to_string(komi) + "network_count_manager.txt";
        std::ifstream file(network_manager_file, std::ios::in);
        if(!file.is_open()) {
            std::cout << "ERROR in line " << __LINE__ << __FILE__ << ".\n";
            exit(0);
        }
        int multithread_network_num;
        file >> multithread_network_num;
        file.close();
        athena.copy_to_file(std::to_string(N) + "-" + std::to_string(komi) + "multithread_network" + std::to_string(multithread_network_num) + ".txt");
    }
}
//fixed athena is like the opponent's athena. You play athena vs fixed athena where the first one gets trained and updated and the second one not.
void fixed_athena_construct_scheme() {
    while(!fixed_athena_constructed) {
        if(fixed_athena.constr(FIXED_ATHENA_FILENAME)) {
            //std::cout << "Merci beaucoup! ;)\n";
            fixed_athena_constructed = true;
        } else {
            std::cout << "Oops, that network could not be opened. Try again. " << FIXED_ATHENA_FILENAME << " <- fixed athena filename\n" << std::flush;
        }
    }
}
//fixed athena is like the opponent's athena. You play athena vs fixed athena where the first one gets trained and updated and the second one not.
//BLACK'S TURN
double athena_output_on_given_position(bool use_fixed_athena) { //uses POTENTIAL board
    if(!use_fixed_athena) {
        if(!athena_constructed) {athena_construct_scheme();}
    } else {
        if(!fixed_athena_constructed) {fixed_athena_construct_scheme();}
    }
    std::vector<double> input_for_athena;
    for (int y = 0; y < N; y++) {
        for (int x = 0; x < N; x++) {
            double b = ((potential_board[y][x] == 0) ? (0.5) : (potential_board[y][x] == 1 ? 1.0 : 0.0));
            //Input for Athena: black stone: 1; empty intersection: 0.5; white stone: 0     <-------------------------------- IMPORTANT
            //The above is important
            input_for_athena.push_back(b);
        }
    }
    if(!use_fixed_athena) {
        return athena.current_network_output_on_given_input(input_for_athena)[0];
    } else {
        return fixed_athena.current_network_output_on_given_input(input_for_athena)[0];
    }
}

//consec_passes is the number of consecutive passes that have occured, including the just-played move by black
double tree_evaluation_after_black_move(bool initial_func_call, int depth, int consec_passes/*, Function instant_evaluation_function, Types ... Args*/) {
    if(initial_func_call) potential_board = board;
    if(consec_passes == 2) {
        if((double)count_area_black() < (double)count_area_white() + komi) {
            return -std::numeric_limits<double>::infinity();
        } else {
            return std::numeric_limits<double>::infinity();
        }
    }
    if(consec_passes == 1 && ((double)count_area_black() < (double)count_area_white() + komi)) {
        //white will also pass and win. black's evaluation is minus infinity
        return -std::numeric_limits<double>::infinity();
    }
    auto leg_movesw = all_legal_moves(2); //all legal moves of WHITE
    double min_eval = std::numeric_limits<double>::infinity(); //white tries to make blacks evaluation as small as possible because white wants that black loses

    /*if(leg_movesw.size() > 13) {
        return athena_output_on_given_position(false);
    }*/

    for(int i = 0; i < (int)leg_movesw.size(); i++) {
        auto potential_board_first = potential_board;
        update_potential_board(std::get<0>(leg_movesw[i]), std::get<1>(leg_movesw[i]), 2); //WHITE!
        auto consec_passes_first = consec_passes;
        board = potential_board;
        if(std::get<0>(leg_movesw[i]) == -1) {
            consec_passes++;
        } else {
            consec_passes = 0;
        }

        double max_eval_here = -std::numeric_limits<double>::infinity();
        if(consec_passes == 2) {
            if((double)count_area_black() < (double)count_area_white() + komi) {
                max_eval_here = -std::numeric_limits<double>::infinity();
            } else {
                max_eval_here = std::numeric_limits<double>::infinity();
            }
        } else {
            auto potential_board_first2 = potential_board;
            auto consec_passes_first2 = consec_passes;
            auto leg_movesb = all_legal_moves(1); //all legal moves of BLACK
            for(int j = 0; j < (int)leg_movesb.size(); j++) {
                update_potential_board(std::get<0>(leg_movesb[j]), std::get<1>(leg_movesb[j]), 1); //BLACK!
                board = potential_board;
                if(std::get<0>(leg_movesb[j]) == -1) {
                    consec_passes++;
                } else {
                    consec_passes = 0;
                }
                double evalb;
                if(consec_passes == 2) {
                    if((double)count_area_black() < (double)count_area_white() + komi) {
                        evalb = -std::numeric_limits<double>::infinity();
                    } else {
                        evalb = std::numeric_limits<double>::infinity();
                    }
                } else if(depth == 0) {
                    //evalb = evaluation_now_look_half_move_deep(consec_passes);
                    evalb = athena_output_on_given_position(false);
                } else {
                    evalb = tree_evaluation_after_black_move(false, depth-1, consec_passes/*, instant_evaluation_function, Args...*/);
                }
                if(evalb > max_eval_here) {
                    max_eval_here = evalb;
                }
                potential_board = potential_board_first2;
                board = potential_board_first2;
                consec_passes = consec_passes_first2;
            }
        }
        if(max_eval_here < min_eval) min_eval = max_eval_here;
        board = potential_board_first;
        potential_board = potential_board_first;
        consec_passes = consec_passes_first;
    }
    return min_eval;
}

//this uses fixed athena
//consec_passes is the number of consecutive passes that have occured, including the just-played move by black
//when this function is called, black has played the last move. Now, for every white move, it is evaluated with fixed athena how white thinks about the position. The maximum gainable evaluation for white can thus be calculated. Good for white is bad for black, so the MINUS whites_max_eval is returned. The return value of this function therefore as usual is an evaluation of quality of the move that black just played, seen from black perspective.
double evaluation_now_look_half_move_deep(int consec_passes) {
    potential_board = board;
    if(consec_passes == 2) {
        if((double)count_area_black() < (double)count_area_white() + komi) {
            return -std::numeric_limits<double>::infinity();
        } else {
            return std::numeric_limits<double>::infinity();
        }
    }
    if(consec_passes == 1 && ((double)count_area_black() < (double)count_area_white() + komi)) {
        //white will also pass and win. black's evaluation is minus infinity
        return -std::numeric_limits<double>::infinity();
    }
    auto leg_movesw = all_legal_moves(2); //all legal moves of WHITE
    double whites_max_eval = -std::numeric_limits<double>::infinity(); //white tries to make blacks evaluation as small as possible because white wants that black loses
    for(int i = 0; i < (int)leg_movesw.size(); i++) {
        auto potential_board_first = potential_board;
        update_potential_board(std::get<0>(leg_movesw[i]), std::get<1>(leg_movesw[i]), 2); //WHITE!
        auto consec_passes_first = consec_passes;
        board = potential_board;
        if(std::get<0>(leg_movesw[i]) == -1) {
            consec_passes++;
        } else {
            consec_passes = 0;
        }

        double whites_eval;
        if(consec_passes == 2) { //white has passed
            if((double)count_area_black() < (double)count_area_white() + komi) {
                whites_eval = std::numeric_limits<double>::infinity();
            } else {
                whites_eval = -std::numeric_limits<double>::infinity();
            }
        } else {
            //one move of white is now done. Now look at evaluation of fixed athena, white athena.
            invert_colors();
            whites_eval = athena_output_on_given_position(true);
            invert_colors();
        }
        if(whites_max_eval < whites_eval) whites_max_eval = whites_eval;
        board = potential_board_first;
        potential_board = potential_board_first;
        consec_passes = consec_passes_first;
    }

    return -whites_max_eval;
}

std::tuple<int, int, double> player_choice() {
    double max_eval = -1000.0;
    int player = turn % 2 + 1;
    //whose turn is it currently?
    // 1 = black; 2 = white
    std::string player_input_mode;
    if (player == 1) {
        //std::cout << "\n\n\n\n\nZWART is aan de beurt\n";
        player_input_mode = player_1_input_mode;
    }
    if (player == 2) {
        //std::cout << "\n\n\n\n\nWIT is aan de beurt\n";
        player_input_mode = player_2_input_mode;
    }
    int choice_y;
    int choice_x;

    auto random_mv = [&player, &choice_y, &choice_x]() {
        auto leg_moves = all_legal_moves(player);
        std::random_device r_d;
        std::mt19937 g(r_d());
        std::shuffle(leg_moves.begin(), leg_moves.end(), g);
        choice_y = std::get<0>(leg_moves[0]);
        choice_x = std::get<1>(leg_moves[0]);
    };

    auto smartened_random_mv = [&player, &choice_y, &choice_x]() {
        if(player == 1) {
            auto leg_moves = all_legal_moves(player);
            std::random_device r_d;
            std::mt19937 g(r_d());
            std::shuffle(leg_moves.begin(), leg_moves.end(), g);
            choice_y = std::get<0>(leg_moves[0]);
            choice_x = std::get<1>(leg_moves[0]);
            double cab = (double)count_area_black();
            double caw = (double)count_area_white();
            if(cab > caw + komi && last_move_was_a_pass) {
                choice_y = -1;
                choice_x = -1;
            }
            if((cab < caw + komi && (int)leg_moves.size() > 1) && choice_y == -1) {
                choice_y = std::get<0>(leg_moves[ 1 ]);
                choice_x = std::get<1>(leg_moves[ 1 ]);
            }
        } else if(player == 2) {
            auto leg_moves = all_legal_moves(player);
            std::random_device r_d;
            std::mt19937 g(r_d());
            std::shuffle(leg_moves.begin(), leg_moves.end(), g);
            choice_y = std::get<0>(leg_moves[0]);
            choice_x = std::get<1>(leg_moves[0]);
            double cab = (double)count_area_black();
            double caw = (double)count_area_white();
            if(cab < caw + komi && last_move_was_a_pass) {
                choice_y = -1;
                choice_x = -1;
            }
            if((cab > caw + komi && (int)leg_moves.size() > 1) && choice_y == -1) {
                choice_y = std::get<0>(leg_moves[ 1 ]);
                choice_x = std::get<1>(leg_moves[ 1 ]);
            }
        } else {
            std::cout << "ERROR" << __LINE__ << __FILE__ << "\n";
            exit(0);
        }
    };



    auto choose_black_best_move = [&player, &choice_y, &choice_x, &max_eval]<class Function, class ... Types>(Function evaluation_function, Types ... Args) {
        //is 'smartened' //for BLACK!
        auto leg_moves = all_legal_moves(1); //all legal moves of BLACK
        double cab = (double)count_area_black();
        double caw = (double)count_area_white();
        if((int)leg_moves.size() == 1 || (cab > caw + komi && last_move_was_a_pass)) {
            choice_y = -1;
            choice_x = -1;
            potential_board = board;
            int playernow = 1; //BLACK!!
            update_potential_board(-1,-1, playernow);
            max_eval = evaluation_function(Args...);
        } else
        {
            max_eval = -std::numeric_limits<double>::infinity();
            int corresponding_index = -1;
            int i = 0;
            if(cab < caw + komi) {
                i = 1; //skip the option of passing a move: that would make the network lose.
            }
            for(; i < (int)leg_moves.size(); i++) {
                auto board_first = board;
                potential_board = board;
                int playernow = 1; //BLACK!!!
                update_potential_board(std::get<0>(leg_moves[i]), std::get<1>(leg_moves[i]), playernow);
                board = potential_board;
                double eval = evaluation_function(Args...);
                if(eval >= max_eval) {
                    max_eval = eval;
                    corresponding_index = i;
                }
                board = board_first;
                potential_board = board_first;
            }
            choice_y = std::get<0>(leg_moves[corresponding_index]);
            choice_x = std::get<1>(leg_moves[corresponding_index]);
        }
    };

    auto choose_white_best_move = [&player, &choice_y, &choice_x, &max_eval]<class Function, class ... Types>(Function evaluation_function, Types ... Args) {
        //this is tricky. In the internal code, the network that is training and learning, ALWAYS plays black. On the other hand, this is the FIXED network which is supposed to be the opponent of the learning network. Therefore, the fixed athena ALWAYS PLAYS WHITE. BUT CAUTION:
        //Also this fixed athena thought that it was black when it itself was being trained. Therefore, we have to be very meticulous here.
        //We will invert colors TWICE, for the network must think that it is playing black
        //CAUTION there is also the ko rule which must be taken care of! Therefore meticulousness is necessary.

        auto leg_moves = all_legal_moves(2); //all legal moves of WHITE
        double cab = (double)count_area_black();
        double caw = (double)count_area_white();
        if((int)leg_moves.size() == 1 || (cab < caw + komi && last_move_was_a_pass)) {
            choice_y = -1;
            choice_x = -1;
            potential_board = board;
            int playernow = 2; //WHITE!!
            update_potential_board(-1,-1, playernow);
            max_eval = -1000; //random value, shouldn't matter because this is white playing, so no training will happen here
        } else {
            max_eval = -std::numeric_limits<double>::infinity();
            int corresponding_index = -1;
            int i = 0;
            if(cab > caw + komi) {
                i = 1; //skip the option of passing a move: that would make the network lose.
            }
            for(; i < (int)leg_moves.size(); i++) {
                auto board_first = board;
                potential_board = board;
                int playernow = 2; //WHITE!!!
                update_potential_board(std::get<0>(leg_moves[i]), std::get<1>(leg_moves[i]), playernow);
                board = potential_board;

                //we do not have to do anything with the turn variable now.
                invert_colors(); //potential board
                double eval = evaluation_function(Args...);
                invert_colors(); //potential board

                //std::cout << eval << "<- eval\n";
                if(eval > max_eval) {
                    max_eval = eval;
                    corresponding_index = i;
                }
                //std::cout << max_eval << "<- max_eval\n";
                board = board_first;
                potential_board = board_first;
            }
            choice_y = std::get<0>(leg_moves[corresponding_index]);
            choice_x = std::get<1>(leg_moves[corresponding_index]);
        }
    };

    // BEGIN determine choice_y and choice_x
    if (player_input_mode == "g") { //gui
        auto m = GUI_make_move(player);
        choice_y = std::get<0>(m);
        choice_x = std::get<1>(m);
    } else if(player_input_mode == "r") { //completely random move
        random_mv();
    } else if(player_input_mode == "s") { //smartened random mover
        smartened_random_mv();
    } else if(player_input_mode == "a") { //athena that plays black (internally)
        choose_black_best_move(athena_output_on_given_position, false);
    } else if(player_input_mode == "d") { //dumb athena used for training
        if(uniform_random_real_number(0.0, 100.0) < 90.0) {
            smartened_random_mv();
        } else {
            choose_black_best_move(athena_output_on_given_position, false);
        }
    } else if(player_input_mode == "t") { //tree-making, internally black-playing athena
        /*if(all_legal_moves(1).size() < 15) {
            choose_black_best_move(tree_evaluation_after_black_move, true, 1, (last_move_was_a_pass ? 1 : 0));
        } else if(all_legal_moves(1).size() < 40) {
            //choose_black_best_move(tree_evaluation_after_black_move, true, 1, (last_move_was_a_pass ? 1 : 0));
            choose_black_best_move(evaluation_now_look_half_move_deep, (last_move_was_a_pass ? 1 : 0));
        } else {
            choose_black_best_move(athena_output_on_given_position, false);
        }*/
        if(all_legal_moves(1).size() < V) {
            choose_black_best_move(tree_evaluation_after_black_move, true, 1, (last_move_was_a_pass ? 1 : 0));
        } else {
            choose_black_best_move(athena_output_on_given_position, false);
        }
    } else if(player_input_mode == "fa") { //athena that plays white (internally) and should therefore think that it plays black (internally)
        if(uniform_random_real_number(0.0, 100.0) < 25.0) {
            smartened_random_mv(); //we need to have a randomized aspect in fixed athena, otherwise it will always play the same
        } else {
            choose_white_best_move(athena_output_on_given_position, true);
        }
    } else {
        std::cout << "ERROR" << __LINE__ << __FILE__ << "\n";
        exit(0);
    }
    // END determine choice_y and choice_x
    if (!quit) {
        update_board(choice_y, choice_x, player);
        if( (player_input_mode == "a" || player_input_mode == "t") && choice_y == -1) {
            std::cout << "The computer has passed a move. It\'s your turn.\n";
        }
    }
    render_board();
    if(player_input_mode == "g") {
        SDL_Delay(80);
    }
    return std::make_tuple((int)choice_y, (int)choice_x, (double)max_eval);
}
void reset_all_global_variables() {
    //athena_constructed = false; //notice that athena_output_on_given_position, when called, is now going to reconstrct athena via the constr() function. The constr() function also resets the network and then constructs it again.
    last_move_was_a_pass = false;
    //variabeles that do not need to be reset: komi, N, P
    board.clear();
    former_positions.clear();
    //former_positions.push_back(std::vector<std::vector<int> >(N, std::vector<int>(N, 0)));
    potential_board.clear();
    turn = 0;
    visited.clear();
    visited_potential.clear();
    //variables that do not need to be reset: at_least_one_liberty_is_found, at_least_one_liberty_is_found_potential
    consecutive_passes = 0;
    game_ended = false;
    quit = false;
    init_go();
    if(komi < 0) {
        if(player_1_input_mode == "g" || player_2_input_mode == "g") {
            auto m = GUI_make_move(2);
            board[std::get<0>(m)][std::get<1>(m)] = 2; //WHITE ! (but renders as black)
        } else {
            //already make the first move randomly -- by WHITE! (Then the network can keep playing black, as always)
            auto leg_moves = all_legal_moves(2);
            //std::cout << (int)leg_moves.size() << "\n";
            std::random_device r_d;
            std::mt19937 g(r_d());
            std::shuffle(leg_moves.begin(), leg_moves.end(), g);
            int choice_y = std::get<0>(leg_moves[0]);
            int choice_x = std::get<1>(leg_moves[0]);
            //std::cout << choice_y << " " << choice_x << "\n";
            if(choice_y != -1) {
                board[choice_y][choice_x] = 2; //WHITE! (but renders as black)
            }
        }
        render_board();
        //SDL_Delay(1500);
    }
}
// plays a game, but DOESN'T call Athena functions or so
bool fully_play_one_game(bool do_not_write_anything = false, bool this_is_test_data = false, int test_data_game_number = -1) {
    if(DO_NOT_WRITE_DATA_TO_FILES) do_not_write_anything = true;
    reset_all_global_variables();
    std::vector<std::tuple<int, int> > moves_done; //all moves which have been played. By either black or white.
    std::vector<double> q;
    while (!game_ended && !quit) {
        //std::cout << "c\n";
        auto move_data = player_choice();
        moves_done.push_back(std::make_tuple(std::get<0>(move_data), std::get<1>(move_data))); //you now get all moves which have been played. By either black or white.
        //q.push_back( std::min( std::max(std::get<2>(move_data),0.0), 1.0 ) );
        q.push_back(std::get<2>(move_data));
    }
    bool black_wins = (double)count_area_black() > (double)count_area_white() + komi;
    //fix Q-values in final states
    if(moves_done.size() % 2 == 1) {
        //last move was from black
        q.back() = (black_wins ? 1.0 : 0.0);
    } else {
        //last move was from white
        q[(int)q.size()-1-1] = (black_wins ? 1.0 : 0.0); //change q for black's last move
    }
    if(do_not_write_anything) {
        if(ONLY_GUI_EASY) std::cout << ((black_wins != (komi < 0)) ? "\n\nBLACK HAS WON THE GAME.\n" : "\n\nWHITE HAS WON THE GAME.\n");
        //if komi is negative, present to the user the result which the user sees: invert colors.
        return black_wins;
    }
    std::string curr_game_file;
    //now, we want to save the game and it's result in a file.
    if(!this_is_test_data) {
        std::string game_manager_file = std::to_string(N) + "-" + std::to_string(komi) + "game_count_manager.txt";
        if(MULTITHREAD_SECONDARY_PROCESS) {
            game_manager_file = "thread" + std::to_string(THIS_THREAD_NUM) + "/" + game_manager_file;
        }
        std::ifstream file(game_manager_file,std::ios::in);
        if(!file.is_open()) {
            std::cout << "ERROR in line " << __LINE__ << __FILE__ << ".\n";
            std::cout << game_manager_file << "\n";
            exit(0);
        }
        int number_of_games_played_totally;
        file >> number_of_games_played_totally;
        file.close();
        std::ofstream file2(game_manager_file,std::ios::out|std::ios::trunc);
        if(!file2.is_open()) {
            std::cout << "ERROR in line " << __LINE__ << __FILE__ << ".\n";
            exit(0);
        }
        file2 << ++number_of_games_played_totally;
        file2.close();

        //now we know which file to write to.
        curr_game_file = std::to_string(N) + "-" + std::to_string(komi) + "game" + std::to_string(number_of_games_played_totally) + ".txt";
    } else {
        curr_game_file = std::to_string(N) + "-" + std::to_string(komi) + "test_data_game" + std::to_string(test_data_game_number) + ".txt";
    }

    if(MULTITHREAD_SECONDARY_PROCESS) {
        curr_game_file = "thread" + std::to_string(THIS_THREAD_NUM) + "/" + curr_game_file;
    }
    std::ofstream gamefile(curr_game_file, std::ios::trunc);
    if(!gamefile.is_open()) {
        std::cout << "ERROR in line " << __LINE__ << __FILE__ << ".\n";
        exit(0);
    }
    for(int i = 0; i < (int)former_positions.size(); i += 2) {
        for(int y = 0; y < N; y++) {
            for(int x = 0; x < N; x++) {
                gamefile << former_positions[i][y][x] << " ";
            }
        }
        if(!Q_LEARNING) {
            gamefile << (black_wins ? 1 : 0); //who eventually won the game
        } else {
            double new_q_value;
            if((int)q.size() >= i + 3) {
                new_q_value = q[i] + ALFA*(DISCOUNT*q[i+2] - q[i]);
            } else {
                new_q_value = black_wins;
            }
            gamefile << new_q_value;
        }
        gamefile << "\n";
    }
    gamefile.close();
    return black_wins;
}
void run_go(int thrnum) {
    std::system((std::string("./go thread ") + std::to_string(thrnum)).c_str()); //run this program with parameters
}
void play_games() {
    if(!MULTITHREAD_SECONDARY_PROCESS) {
        std::cout << "ERROR" << __LINE__ << __FILE__ << "\n";
        exit(0);
    }
    int black_win_counter = 0;
    int black_win_counter_test_data = 0;
    int number_of_games_played_totally;
    //std::cout << "THREAD " << THIS_THREAD_NUM << " : Game nr. 1 is now being played...\n" << std::flush;
    for(int i = 0; i < NUMBER_OF_GAMES; i++) {
        std::string game_manager_file = "thread" + std::to_string(THIS_THREAD_NUM) + "/" + std::to_string(N) + "-" + std::to_string(komi) + "game_count_manager.txt";
        std::ifstream file(game_manager_file,std::ios::in);
        if(!file.is_open()) {
            std::cout << "ERROR in line " << __LINE__ << __FILE__ << ".\n";
            std::cout << game_manager_file << "\n";
            std::cout << "We will try to solve this error by creating the file: " << game_manager_file << "\n";
            std::cout << "Do you want this? Type \"y\". Otherwise the program will quit.";
            std::string s;
            std::cin >> s;
            if(s != "y") {
                std::cout << "Abort. Bye; - Athena.\n";
                exit(0);
            }
            std::ofstream wfile(game_manager_file,std::ios::out);
            if(!wfile.is_open()) {
                std::cout << "ERROR. " << __LINE__ << __FILE__ << " Have you created the thread directory?\n";
                exit(0);
            }
            wfile << 1;
            wfile.close();
            std::cout << "OK, Done. Please run this program again. Bye! -ATHENA\n";
            exit(0);
        }
        if(fully_play_one_game()) {
            black_win_counter++;
        }
        //if(THIS_THREAD_NUM == 0) std::cout << "THREAD " << THIS_THREAD_NUM << " : Black wins " << black_win_counter << " out of " << (i+1) << " games. Still playing...\n" << std::flush;
        file >> number_of_games_played_totally;
        file.close();
        /*std::string fname = "thread" + std::to_string(THIS_THREAD_NUM) + "/" + std::to_string(N) + "-" + std::to_string(komi) + "network" + std::to_string(number_of_games_played_totally) + ".txt";
        athena.copy_to_file(fname);*/
    }
    //training data ABOVE ---- test data BELOW
    for(int i = 0; i < NUMBER_OF_TEST_DATA_GAMES; i++) { //we need no test data game count manager, we just keep overwriting the test_data game files. No need to keep them forever...
        if(fully_play_one_game(false, true, i)) {
            black_win_counter_test_data++;
        }
    }
    std::cout << "THREAD " << THIS_THREAD_NUM << " : Won " << black_win_counter << " out of " << (NUMBER_OF_GAMES) << " games and " << black_win_counter_test_data << " out of " << (NUMBER_OF_TEST_DATA_GAMES) << " games.\n" << std::flush;

    std::string number_won_file = "thread" + std::to_string(THIS_THREAD_NUM) + "/" + std::to_string(N) + "-" + std::to_string(komi) + "wonnum.txt";
    std::string out_of_file = "thread" + std::to_string(THIS_THREAD_NUM) + "/" + std::to_string(N) + "-" + std::to_string(komi) + "outof.txt";
    std::ofstream nwfile(number_won_file,std::ios::trunc);
    std::ofstream oofile(out_of_file,std::ios::trunc);
    int number_won = black_win_counter + black_win_counter_test_data;
    int out_of = NUMBER_OF_GAMES + NUMBER_OF_TEST_DATA_GAMES;
    nwfile << number_won;
    oofile << out_of;


    //remove old files...
    for(int i = number_of_games_played_totally - 50*NUMBER_OF_PAST_GAMES_TO_TRAIN_WITH; i < number_of_games_played_totally - 10*NUMBER_OF_PAST_GAMES_TO_TRAIN_WITH; i++) {
        if(i % 10000 == 0) {
            //do not remove
            continue;
        }
        std::string fname = "thread" + std::to_string(THIS_THREAD_NUM) + "/" + std::to_string(N) + "-" + std::to_string(komi) + "network" + std::to_string(i) + ".txt";
        std::remove(fname.c_str());
        fname = "thread" + std::to_string(THIS_THREAD_NUM) + "/" + std::to_string(N) + "-" + std::to_string(komi) + "game" + std::to_string(i) + ".txt";
        std::remove(fname.c_str());
    }
}
void training_loop() {
    if(MULTITHREAD_SECONDARY_PROCESS) {
        std::cout << "ERROR" << __LINE__ << __FILE__ << "\n";
        exit(0);
    }
    if(!MULTITHREAD_PRIMARY_PROCESS) {
        int number_of_games = NUMBER_OF_GAMES;
        int black_win_counter = 0;
        int number_of_games_played_totally;
        std::cout << "Game nr. 1 is now being played..." << std::flush;
        for(int i = 0; i < number_of_games; i++) {
            if(fully_play_one_game()) {
                black_win_counter++;
            }
            std::cout << "\rBlack wins " << black_win_counter << " out of " << (i+1) << " games. Still playing..." << std::flush;
            std::string game_manager_file = std::to_string(N) + "-" + std::to_string(komi) + "game_count_manager.txt";
            std::ifstream file(game_manager_file,std::ios::in);
            if(!file.is_open()) {
                std::cout << "ERROR in line " << __LINE__ << __FILE__ << ".\n";
                std::cout << game_manager_file << "\n";
                exit(0);
            }
            file >> number_of_games_played_totally;
            file.close();
            std::string fname = std::to_string(N) + "-" + std::to_string(komi) + "network" + std::to_string(number_of_games_played_totally) + ".txt";
            athena.copy_to_file(fname);
        }
        std::cout << "\nBlack won " << black_win_counter << " out of " << number_of_games << " games. Training will now begin." << std::flush;
        train_network(NUMBER_OF_PAST_GAMES_TO_TRAIN_WITH, NUMBER_OF_TEST_DATA_GAMES, NETWORK_LEARNING_RATE, MINI_BATCH_SIZE, NUMBER_OF_EPOCHS);
        std::cout << "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\bTraining finished. ";
        std::cout << "The current learning rate is " << NETWORK_LEARNING_RATE << "\n";
        //remove old files...
        for(int i = number_of_games_played_totally - 50*NUMBER_OF_PAST_GAMES_TO_TRAIN_WITH; i < number_of_games_played_totally - 10*NUMBER_OF_PAST_GAMES_TO_TRAIN_WITH; i++) {
            if(i % 10000 == 0) {
                //do not remove
                continue;
            }
            std::string fname = std::to_string(N) + "-" + std::to_string(komi) + "network" + std::to_string(i) + ".txt";
            std::remove(fname.c_str());
            fname = std::to_string(N) + "-" + std::to_string(komi) + "game" + std::to_string(i) + ".txt";
            std::remove(fname.c_str());
        }
        return;
    } else { //if(MULTITHREAD_PRIMARY_PROCESS)
        //BEGIN THREAD
        std::vector<std::thread> thread_vec;
        for(int i = 0; i < NUMBER_OF_THREADS; i++) {
            thread_vec.push_back(std::thread(run_go, i));
        }
        for(int i = 0; i < NUMBER_OF_THREADS; i++) {
            thread_vec[i].join();
        }
        //END THREAD
        int won_total = 0;
        int played_total = 0;
        for(int i = 0; i < NUMBER_OF_THREADS; i++) {
            std::string number_won_file = "thread" + std::to_string(i) + "/" + std::to_string(N) + "-" + std::to_string(komi) + "wonnum.txt";
            std::string out_of_file = "thread" + std::to_string(i) + "/" + std::to_string(N) + "-" + std::to_string(komi) + "outof.txt";
            std::ifstream nwfile(number_won_file,std::ios::in);
            std::ifstream oofile(out_of_file,std::ios::in);
            int number_won;
            int out_of;
            nwfile >> number_won;
            oofile >> out_of;
            won_total += number_won;
            played_total += out_of;
        }
        std::cout << "Results: algorithm won " << won_total << " out of " << played_total << " games. That is " << (100.0*(double)won_total)/((double)played_total) << " percent.\n";
        std::cout << "\nGames have been played.\n" << std::flush;
        if(!DO_NOT_WRITE_DATA_TO_FILES) {
            std::cout << "Now training... " << std::flush;
            train_network(NUMBER_OF_PAST_GAMES_TO_TRAIN_WITH, NUMBER_OF_TEST_DATA_GAMES, NETWORK_LEARNING_RATE, MINI_BATCH_SIZE, NUMBER_OF_EPOCHS);
            std::cout << "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\bTraining finished.\n" << std::flush;
            std::string network_manager_file = std::to_string(N) + "-" + std::to_string(komi) + "network_count_manager.txt";
            std::ifstream file(network_manager_file, std::ios::in);
            while(!file.is_open()) {
                std::cout << "ERROR in line " << __LINE__ << __FILE__ << ".\n";
                std::cout << network_manager_file << "\n";
                std::cout << "We will try to solve this error by creating the file: " << network_manager_file << "\n";
                std::cout << "Do you want this? Type \"y\". Otherwise the program will quit.";
                std::string s;
                std::cin >> s;
                if(s != "y") {
                    std::cout << "Abort. Bye; - Athena.\n";
                    exit(0);
                }
                std::ofstream wfile(network_manager_file,std::ios::out);
                if(!wfile.is_open()) {
                    std::cout << "ERROR. " << __LINE__ << __FILE__ << "\n";
                    exit(0);
                }
                wfile << 0;
                wfile.close();
                std::cout << "OK, Done. Trying again...\n";
                file.open(network_manager_file, std::ios::in);
            }
            int multithread_network_num;
            file >> multithread_network_num;
            file.close();
            std::ofstream file2(network_manager_file,std::ios::out|std::ios::trunc);
            if(!file2.is_open()) {
                std::cout << "ERROR in line " << __LINE__ << __FILE__ << ".\n";
                exit(0);
            }
            file2 << ++multithread_network_num;
            file2.close();
            std::string fname = std::to_string(N) + "-" + std::to_string(komi) + "multithread_network" + std::to_string(multithread_network_num) + ".txt";
            athena.copy_to_file(fname);
            //std::cout << "Training finished. ";
            std::cout << "The current learning rate is " << NETWORK_LEARNING_RATE << "\n";
        }
        return;
    }
}

int main(int argc, char *argv[]) {
    MULTITHREAD_SECONDARY_PROCESS = false;
    if(argc == 3) {
        THIS_THREAD_NUM = std::atoi(argv[2]);
        MULTITHREAD_SECONDARY_PROCESS = true;
        MULTITHREAD_PRIMARY_PROCESS = false;
    }
    int loopnum;
    int q_learn;
    int mno; //make network optimistic
    if(!ONLY_GUI_EASY) {
        std::ifstream configfile("../config.txt", std::ios::in);
        if(!configfile.is_open()) {
            std::cout << "ERROR couldn't open config file. " << __LINE__ << __FILE__ << "\n";
            exit(0);
        }
        //std::cout << "Read from config.txt file: learning rate; Q-learning yes or no (1 or 0); player 1 input mode; player 2 input mode; activation function; cost function; number of test data games; number of games; number of past games to train with; mini batch size; number of epochs; N; (INTERNAL) komi; how many loops; make network optimistic (bool); filename of fixed athena network.\n";
        configfile >> NETWORK_LEARNING_RATE;
        configfile >> q_learn;
        //if(q_learn == 1) {Q_LEARNING = true;std::cout<<"Q-learning enabled\n";} else {Q_LEARNING = false;std::cout<<"Q-learning DISabled\n";}
        do_train_network = true;
        configfile >> player_1_input_mode >> player_2_input_mode >> WHICH_ACTIVATION_FUNCTION >> WHICH_COST_FUNCTION >> NUMBER_OF_TEST_DATA_GAMES >> NUMBER_OF_GAMES >> NUMBER_OF_PAST_GAMES_TO_TRAIN_WITH >> MINI_BATCH_SIZE >> NUMBER_OF_EPOCHS >> N >> komi >> loopnum;
        if(player_1_input_mode == "athena") player_1_input_mode = "a";
        if(player_1_input_mode == "random_move") player_1_input_mode = "r";
        if(player_1_input_mode == "gui") player_1_input_mode = "g";
        if(player_1_input_mode == "smartened_random") player_1_input_mode = "s";
        if(player_2_input_mode == "athena") player_2_input_mode = "a";
        if(player_2_input_mode == "random_move") player_2_input_mode = "r";
        if(player_2_input_mode == "gui") player_2_input_mode = "g";
        if(player_2_input_mode == "smartened_random") player_2_input_mode = "s";
        athena_constructed = false;
        /*if(player_1_input_mode == "g" || player_2_input_mode == "g") {
            do_train_network = false;
            if(player_1_input_mode == "a" || player_2_input_mode == "a") {
                std::cout << "Please type the file path of the network which you want to use. For instance: type  ../5-24.000000network1278000.txt\n";
                std::string fp;
                std::cin >> fp;
                std::ifstream nf(fp, std::ios::in);
                if(!nf.is_open()) {
                    std::cout << "ERROR couldn't open that network file. Are you sure that the path that you entered, exists? Error in " << __LINE__ << __FILE__ << "\n";
                    exit(0);
                }
                nf.close();
                if(athena.constr(fp)) {
                    std::cout << "The network was constructed from the file name that you inserted.\n";
                } else {
                    std::cout << "Oops, that network could not be opened. Program will quit.\n";
                    exit(0);
                }
                athena_constructed = true;
            }
        }*/
        //std::cout << player_1_input_mode << " " << player_2_input_mode << "\n";
        configfile >> mno;
        if(mno == 1) {
        MAKE_NETWORK_OPTIMISTIC = true;
        std::cout << "Warning. The network is not actually training now. It is making itself optimistic.\n"; //NOTE: IMPORTANT: REMOVE THIS ?
        } else {MAKE_NETWORK_OPTIMISTIC = false;}
        configfile >> FIXED_ATHENA_FILENAME;
        configfile.close();
    } else {
        std::cout << "Hi. You are running the \"Play against algorithm - GUI version\".\nThis program was coded by Aron Hardeman and Raf Brenninkmeijer.\nProgram is compiled on: " << __TIME__ << " " << __DATE__ << "\nThis program is part of our school research project (Profielwerkstuk).\nThe computer algorithm is driven by an artificial neural network which trained itself (also coded by us).\n\n";

        std::cout << "Choose board size. Please type either 9, 13 or 19 and press enter : ";
        std::cin >> N;
        if(N != 9 and N!=13 and N!=19) {
            std::cout << "Invalid board size! Program quits.\n";
            exit(0);
        }
        std::string color;
        std::cout << "Do you want to play against the Computer? [y/n] : ";
        std::string pac;
        std::cin >> pac;
        if(pac != "y") {
            std::cout << "OK: You will play against yourself: free play.\n";
            player_1_input_mode = "g";
            player_2_input_mode = "g";
        } else
        if(N==9) {
            std::cout << "With board size 9, you can only play white. White will be selected automatically.\nThis is nice! With board size 9x9, you can choose the extra smart algorithm which looks deeper.\nDo you want this [y/n] : ";
            std::string vk;
            std::cin >> vk;
            if(vk == "y") {
                std::cout << "Nice! Now you will have to enter the number V which determines at what times the extra smart algorithm is enabled.\nIf less than V moves are possible in a given position, then the extra smart algorithm will be enabled; otherwise it will be disabled.\nThe default value of V used during generation of results in our research project, is 17.\nYou can choose 17 as well, but the higher V is, the more games you should lose...\nAnd: the higher V is, the higher will be the calculation times.\nPLEASE NOTE THAT THE PROGRAM MAY CRASH IF V IS TOO HIGH. Recommended: V = 17\nChoose V and press enter : ";
                std::cin >> V;
                player_1_input_mode = "t";
                player_2_input_mode = "g";
            } else {
                std::cout << "OK. You are just playing against the regular variant.\n";
                player_1_input_mode = "a";
                player_2_input_mode = "g";
            }
            color = "w";
        } else {
            std::cout << "Do you want to be black or white? Type b or w and press [Enter]: ";
            std::cin >> color;
            player_1_input_mode = "a";
            player_2_input_mode = "g";
        }
        do_train_network = false;
        if(pac != "y") {
            komi = 6.5;
        } else
        if(color == "b") {
            komi = -6.5;
            std::string networkfilename;
            if(N == 19) {
                networkfilename = "../19--6.500000multithread_network333994.txt";
                std::cout << "This is 19x19 NK (negative komi) network #333994. Good luck!\n";
            } else if(N == 13) {
                networkfilename = "../13--6.500000multithread_network1472.txt";
                std::cout << "This is 13x13 NK (negative komi) network #1472. Good luck!\n";
            } else if(N == 9) {
                std::cout << "We're sorry, if you chose board size 9x9, you can only play white. Try again.\n";
                exit(0);
            } else {
                std::cout << "ERROR" << __LINE__ << __FILE__ << "\n";
                exit(0);
            }
            if(athena.constr(networkfilename)) {
                std::cout << "Network construct OK...\n\n";
            } else {
                std::cout << "Oops, the network could not be opened. Program will quit.\n";
                exit(0);
            }
            athena_constructed = true;
        } else if(color == "w") {
            komi = 6.5;
            std::string networkfilename;
            if(N == 19) {
                networkfilename = "../19-6.500000multithread_network321703.txt";
                std::cout << "This is 19x19 PK (positive komi) network #321703. Good luck!\n";
            } else if(N == 13) {
                networkfilename = "../13-6.500000multithread_network540.txt";
                std::cout << "This is 13x13 PK (positive komi) network #540. Good luck!\n";
            } else if(N == 9) {
                networkfilename = "../9-6.500000multithread_network34468.txt";
                std::cout << "This is 9x9 PK (positive komi) network #34468. Good luck!\n";
            } else {
                std::cout << "ERROR" << __LINE__ << __FILE__ << "\n";
                exit(0);
            }
            if(athena.constr(networkfilename)) {
                std::cout << "Network construct OK...\n\n";
            } else {
                std::cout << "Oops, the network could not be opened. Program will quit.\n";
                exit(0);
            }
            athena_constructed = true;
        } else {
            std::cout << "You did not type b or w. Program will quit.\n";
            exit(0);
        }

        std::cout << "You can Pass (not play a move) by pressing P while playing. Understood? Type P and press enter: ";
        std::string doesnothing;
        std::cin >> doesnothing;
    }
    SDL_Init(SDL_INIT_VIDEO);
    IMG_Init(IMG_INIT_PNG);

    window = SDL_CreateWindow("GO - Raf Brenninkmeijer & Aron Hardeman", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, P * N, P * N, SDL_WINDOW_OPENGL);
    renderer = SDL_CreateRenderer(window, -1, 0);
    black_stone = makeTexture("../img/BLACK_STONE.png");
    white_stone = makeTexture("../img/WHITE_STONE.png");
    PNGsurface = makeTexture("../img/BOARD" + std::to_string(N) + ".png");
    //screen_surf = SDL_GetWindowSurface(window);
    if(ONLY_GUI_EASY) {
        MULTITHREAD_PRIMARY_PROCESS = false;
        NUMBER_OF_THREADS = -1;
        DO_NOT_WRITE_DATA_TO_FILES = true;
    } else
    if(!MULTITHREAD_SECONDARY_PROCESS) {
        std::cout << "Enable multithreading? (Also available with just one thread) [y/n] : " << std::flush;
        std::string mt;
        std::cin >> mt;
        if(mt == "y" || mt == "Y") {
            MULTITHREAD_PRIMARY_PROCESS = true;
            std::cout << "Number of threads : " << std::flush;
            std::cin >> NUMBER_OF_THREADS;
            std::cout << "\n";
        } else {
            MULTITHREAD_PRIMARY_PROCESS = false;
            NUMBER_OF_THREADS = -1;
            std::cout << "\n";
        }
        std::cout << "Write data to files and train? [y/n] : " << std::flush;
        std::string wdtf;
        std::cin >> wdtf;
        if(wdtf == "y" || wdtf == "Y") {
            DO_NOT_WRITE_DATA_TO_FILES = false;
        } else {
            DO_NOT_WRITE_DATA_TO_FILES = true;
        }
    } else {
        MULTITHREAD_PRIMARY_PROCESS = false;
    }
    // BEGIN construct athena
    if(!MULTITHREAD_PRIMARY_PROCESS && !MULTITHREAD_SECONDARY_PROCESS) { //in this case, it means we are not multithreading
        std::string game_manager_file = std::to_string(N) + "-" + std::to_string(komi) + "game_count_manager.txt";
        std::ifstream file(game_manager_file,std::ios::in);
        if(!file.is_open()) {
            std::cout << "ERROR in line " << __LINE__ << __FILE__ << ".\n";
            std::cout << game_manager_file << "\n";
            std::cout << "We will try to solve this error by creating the file: " << game_manager_file << "\n";
            std::cout << "Do you want this? Type \"y\". Otherwise the program will quit.";
            std::string s;
            std::cin >> s;
            if(s != "y") {
                std::cout << "Abort. Bye; - Athena.\n";
                exit(0);
            }
            std::ofstream wfile(game_manager_file,std::ios::out);
            wfile << 1;
            wfile.close();
            std::cout << "OK, Done. Please run this program again. Bye! -ATHENA\n";
            exit(0);
        }
        int number_of_games_played_totally;
        file >> number_of_games_played_totally;
        file.close();
        std::string fname = std::to_string(N) + "-" + std::to_string(komi) + "network" + std::to_string(number_of_games_played_totally) + ".txt";
        if(!athena_constructed) {
            if(athena.constr(fname)) {
                athena_constructed = true; //so that it doesn't reconstruct it
                //std::cout << "Network was restored from the previous network that was saved while training.\n";
            }
        }
    } else if(MULTITHREAD_PRIMARY_PROCESS || MULTITHREAD_SECONDARY_PROCESS){
        std::string network_manager_file = std::to_string(N) + "-" + std::to_string(komi) + "network_count_manager.txt";
        std::ifstream file(network_manager_file, std::ios::in);
        while(!file.is_open()) {
            std::cout << "ERROR in line " << __LINE__ << __FILE__ << ".\n";
            std::cout << network_manager_file << "\n";
            std::cout << "We will try to solve this error by creating the file: " << network_manager_file << "\n";
            std::cout << "Do you want this? Type \"y\". Otherwise the program will quit.";
            std::string s;
            std::cin >> s;
            if(s != "y") {
                std::cout << "Abort. Bye; - Athena.\n";
                exit(0);
            }
            std::ofstream wfile(network_manager_file,std::ios::out);
            if(!wfile.is_open()) {
                std::cout << "ERROR. " << __LINE__ << __FILE__ << "\n";
                exit(0);
            }
            wfile << 0;
            wfile.close();
            std::cout << "OK, Done. Trying again...\n";
            file.open(network_manager_file, std::ios::in);
        }
        int multithread_network_num;
        file >> multithread_network_num;
        file.close();
        std::string fname = std::to_string(N) + "-" + std::to_string(komi) + "multithread_network" + std::to_string(multithread_network_num) + ".txt";
        if(!athena_constructed) {
            if(athena.constr(fname)) {
                athena_constructed = true; //so that it doesn't reconstruct it
                //std::cout << "Network was restored from the previous network that was saved while multithread-training.\n";
            }
        }
        if(!athena_constructed) athena_construct_scheme();
    }
    // END construct athena
    if(MULTITHREAD_SECONDARY_PROCESS) {
        play_games();
    } else if(do_train_network) {
        //std::cout << "\nThere we go. We are going to play and train. N = " << N << " and komi = " << komi << ".";
        if(komi == 0) {
            std::cout << "Komi is zero and that is weird! Program quits.\n";
            exit(0);
        }
        for(int C = 0; C < loopnum; C++) {training_loop();}
    } else {
        fully_play_one_game(true);
    }
    SDL_DestroyWindow(window);
    SDL_Quit();
    IMG_Quit();
    return 0;
}
