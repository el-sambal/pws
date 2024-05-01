#ifndef GLOBALS_H
#define GLOBALS_H

inline bool ONLY_GUI_EASY = true; //compile the 'easy-to-use version' of the program, without config.txt, without training. This version only includes being able to play with GUI.

//this header file is meant for global variables and most importantly, those which the 'user' (we) should be able to manually change.
inline double ALFA = 0.005;
inline double DISCOUNT = 0.999;
inline std::string which_network_initialisation_when_not_from_file = "x"; //"x" for xavier, "r" for random numbers mu=0 sigma=constant

//variables that will get a value in one of the cpp files:
inline bool Q_LEARNING;
inline std::string player_1_input_mode; 
// "gui" OR "random_move" OR "athena" OR "smartened_random"
// can be shortened also: "g" or "r" or "a" or "s"
inline std::string player_2_input_mode;
inline int NUMBER_OF_GAMES, NUMBER_OF_TEST_DATA_GAMES;
inline int NUMBER_OF_PAST_GAMES_TO_TRAIN_WITH;
inline double NETWORK_LEARNING_RATE;
inline int MINI_BATCH_SIZE;
inline int NUMBER_OF_EPOCHS;
inline bool MAKE_NETWORK_OPTIMISTIC;

inline SDL_Texture *black_stone, *white_stone, *PNGsurface;

//not to be edited:
inline bool athena_constructed = false;
inline bool fixed_athena_constructed = false;
inline bool last_move_was_a_pass = false;
inline bool do_train_network; //see main.cpp: main()
inline bool MULTITHREAD_PRIMARY_PROCESS;
inline int NUMBER_OF_THREADS;
inline bool MULTITHREAD_SECONDARY_PROCESS;
inline bool DO_NOT_WRITE_DATA_TO_FILES;

inline int V = 17;

#endif
