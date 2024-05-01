#include <iostream>
#include <vector>
#include <functional>
#include <tuple>

#define SDL_MAIN_HANDLED

#if __has_include("SDL2/SDL.h")
#include "SDL2/SDL.h"
#endif

#if __has_include("SDL.h")
#include "SDL.h"
#endif

#include "go_rules.h"
#include "globals.h"


void remove_all_visited_blocks() {
    for (int y=0; y<N; y++) {
        for (int x=0; x<N; x++) {
            if (visited[y][x] == true) {
                board[y][x] = 0;
            }
        }
    }
    return;
}

void remove_all_visited_blocks_potential() {
    for (int y=0; y<N; y++) {
        for (int x=0; x<N; x++) {
            if (visited_potential[y][x] == true) {
                potential_board[y][x] = 0;
            }
        }
    }
    return;
}

bool vectorcontains(std::vector<std::vector<std::vector<int> > > &v1, std::vector<std::vector<int> > &v2) {
    for(int i = 0; i < (int)v1.size(); i++) {
        if(v1[i] == v2) {
            return true;
        }
    }
    return false;
}

bool has_potential_liberties(int y, int x, bool initial_function_call) {
    if(initial_function_call) {
        at_least_one_liberty_is_found_potential = false;
        for(int q = 0; q < N; q++) {
            for(int w = 0; w < N; w++) {
                visited_potential[q][w] = false;
            }
        }
    }
    visited_potential[y][x] = true;
    if (x > 0) {
        if(potential_board[y][x-1] == 0) {
            at_least_one_liberty_is_found_potential = true;
        }
        if(potential_board[y][x-1] == potential_board[y][x] && visited_potential[y][x-1] == false) {
            has_potential_liberties(y,x-1, false);
        }
    }
    if (x < N-1) {
        if(potential_board[y][x+1] == 0) {
            at_least_one_liberty_is_found_potential = true;
        }
        if(potential_board[y][x+1] == potential_board[y][x] && visited_potential[y][x+1] == false) {
            has_potential_liberties(y,x+1, false);
        }
    }
    if (y > 0) {
        if(potential_board[y-1][x] == 0) {
            at_least_one_liberty_is_found_potential = true;
        }
        if(potential_board[y-1][x] == potential_board[y][x] && visited_potential[y-1][x] == false) {
            has_potential_liberties(y-1,x, false);
        }
    }
    if (y < N-1) {
        if(potential_board[y+1][x] == 0) {
            at_least_one_liberty_is_found_potential = true;
        }
        if(potential_board[y+1][x] == potential_board[y][x] && visited_potential[y+1][x] == false) {
            has_potential_liberties(y+1,x, false);
        }
    }
    if(initial_function_call) {
        return at_least_one_liberty_is_found_potential;
    } else {
        return false;
    }
}

bool has_liberties(int y, int x, bool initial_function_call) {
    if(initial_function_call) {
        // ^ if this function is now NOT recursively called but was called from another function
        //we must now 'reset' the following things:
        at_least_one_liberty_is_found = false;
        for(int q = 0; q < N; q++) {
            for(int w = 0; w < N; w++) {
                visited[q][w] = false;
            }
        }
    }
    //Notice that, after this function has been called, the visited-vector is NOT yet reset.
    //This means that we can see the string of connected Go stones even after this function is ended.
    //These have visited[y][x] = true.
    //This can be useful: when we place a block, we can check whether its neighbors have liberties or not.
    //And if this function says the neighbor has no liberties, then we can at once delete
    //the stones for which visited[y][x] is true. These stones have then been captured.
    visited[y][x] = true;   //this means that the current position/block has been visited.
    if (x > 0) {
        if(board[y][x-1] == 0) {
            at_least_one_liberty_is_found = true;
        }
        if(board[y][x-1] == board[y][x] && visited[y][x-1] == false) {
            has_liberties(y,x-1, false);
        }
    }
    if (x < N-1) {
        if(board[y][x+1] == 0) {
            at_least_one_liberty_is_found = true;
        }
        if(board[y][x+1] == board[y][x] && visited[y][x+1] == false) {
            has_liberties(y,x+1, false);
        }
    }
    if (y > 0) {
        if(board[y-1][x] == 0) {
            at_least_one_liberty_is_found = true;
        }
        if(board[y-1][x] == board[y][x] && visited[y-1][x] == false) {
            has_liberties(y-1,x, false);
        }
    }
    if (y < N-1) {
        if(board[y+1][x] == 0) {
            at_least_one_liberty_is_found = true;
        }
        if(board[y+1][x] == board[y][x] && visited[y+1][x] == false) {
            has_liberties(y+1,x, false);
        }
    }
    if(initial_function_call) {
        return at_least_one_liberty_is_found;
    } else {
        return false;
    }
}



void update_board(int choice_y, int choice_x, int player) {
    turn++;
    if(choice_y == -1 && choice_x == -1) {
        last_move_was_a_pass = true;
    } else {
        last_move_was_a_pass = false;
    }
    if(choice_y == -1 && choice_x == -1) {
        consecutive_passes++;
        if(consecutive_passes == 2) {
            game_ended = true;
        }
        former_positions.push_back(board);
        return;
    } else {
        consecutive_passes = 0;
    }

    int opposite_player = (player == 2) ? 1 : 2;
    board[choice_y][choice_x] = player;
    if(choice_y < N-1) {
        if(board[choice_y+1][choice_x] == opposite_player) {
            if(!has_liberties(choice_y+1, choice_x, true)) {
                remove_all_visited_blocks();
            }
        }
    }
    if(choice_y > 0) {
        if(board[choice_y-1][choice_x] == opposite_player) {
            if(!has_liberties(choice_y-1, choice_x, true)) {
                remove_all_visited_blocks();
            }
        }
    }


    if(choice_x < N-1) {
        if(board[choice_y][choice_x+1] == opposite_player) {
            if(!has_liberties(choice_y, choice_x+1, true)) {
                remove_all_visited_blocks();
            }
        }
    }
    if(choice_x > 0) {
        if(board[choice_y][choice_x-1] == opposite_player) {
            if(!has_liberties(choice_y, choice_x-1, true)) {
                remove_all_visited_blocks();
            }
        }
    }

    
    former_positions.push_back(board);
}
void update_potential_board(int choice_y, int choice_x, int player) {
    if(choice_y == -1 && choice_x == -1) return;

    int opposite_player = (player == 2) ? 1 : 2;
    potential_board[choice_y][choice_x] = player;
    if(choice_y < N-1) {
        if(potential_board[choice_y+1][choice_x] == opposite_player) {
            if(!has_potential_liberties(choice_y+1, choice_x, true)) {
                remove_all_visited_blocks_potential();
            }
        }
    }
    if(choice_y > 0) {
        if(potential_board[choice_y-1][choice_x] == opposite_player) {
            if(!has_potential_liberties(choice_y-1, choice_x, true)) {
                remove_all_visited_blocks_potential();
            }
        }
    }


    if(choice_x < N-1) {
        if(potential_board[choice_y][choice_x+1] == opposite_player) {
            if(!has_potential_liberties(choice_y, choice_x+1, true)) {
                remove_all_visited_blocks_potential();
            }
        }
    }
    if(choice_x > 0) {
        if(potential_board[choice_y][choice_x-1] == opposite_player) {
            if(!has_potential_liberties(choice_y, choice_x-1, true)) {
                remove_all_visited_blocks_potential();
            }
        }
    }
}

bool move_is_legal(int choice_y, int choice_x, int player) {
    if(choice_y == -1 && choice_x == -1) {
        return true;
    }

    potential_board = board;

    if ((choice_x > N-1) || (choice_y > N-1)) {
        return false;
    }
    if ((choice_x < 0) || (choice_y < 0)) {
        return false;
    }
    if ((potential_board[choice_y][choice_x] == 2) || (potential_board[choice_y][choice_x] == 1)) {
        return false;
    }

    potential_board[choice_y][choice_x] = player;
    if (!has_potential_liberties(choice_y, choice_x, true)) {
        bool going_to_return = true;

        if (choice_y < N-1) {
            if (!has_potential_liberties(choice_y+1, choice_x, true) && potential_board[choice_y+1][choice_x] != player) {
                going_to_return = false;
            }
        }
        if (choice_y > 0) {
            if (!has_potential_liberties(choice_y-1, choice_x, true) && potential_board[choice_y-1][choice_x] != player) {
                going_to_return = false;
            }
        }
        if (choice_x < N-1) {
            if (!has_potential_liberties(choice_y, choice_x+1, true) && potential_board[choice_y][choice_x+1] != player) {
                going_to_return = false;
            }
        }
        if (choice_x > 0) {
            if (!has_potential_liberties(choice_y, choice_x-1, true) && potential_board[choice_y][choice_x-1] != player) {
                going_to_return = false;
            }
        }
        if (going_to_return) {
            potential_board = board;
            return false;
        }
    }

    update_potential_board(choice_y, choice_x, player);

    //KO RULE:
    if (vectorcontains(former_positions, potential_board)) {
        potential_board = board;
        return false;
    }
    
    potential_board = board;
    return true;
}

std::vector<std::tuple<int,int> > all_legal_moves(int player) {
    std::vector<std::tuple<int,int> > v;
    for(int y = -1; y < N; y++) {
        for(int x = -1; x < N; x++) {
            if(move_is_legal(y,x,player)) {
                v.push_back(std::make_tuple(y,x));
            }
        }
    }
    return v;
}


void print_board() {
    for (int y = 0; y<N; y++) {
        for (int x = 0; x<N; x++) {
            std::cout << " " <<board[y][x];
        }
        std::cout<< "\n";
    }
    std::cout << "\n";
    // for (int z = 0; z < former_positions.size(); z++) {
    //     for (int y = 0; y < N; y++) {
    //         for (int x = 0; x < N; x++ ) {
    //           std::cout << former_positions[z][y][x];
    //         }
    //         std::cout<< "\n";
    //     }
    //     std::cout<< "\n";
    // }
}

bool is_surrounded_by_black(int yy, int xx) {
    bool seen_opposite_color = false;
    auto visited_s = board;

    std::function<void(int,int)> visit_neighbors = [&](int y, int x) {
        visited_s[y][x] = 3; //an empty, visited square gets the value 3
        if(y > 0) {
            if(visited_s[y-1][x] == 0) {
                visit_neighbors(y-1, x);
            } else if(visited_s[y-1][x] == 2) {
                seen_opposite_color = true;
            }
        }
        if(y < N-1) {
            if(visited_s[y+1][x] == 0) {
                visit_neighbors(y+1, x);
            } else if(visited_s[y+1][x] == 2) {
                seen_opposite_color = true;
            }
        }
        if(x < N-1) {
            if(visited_s[y][x+1] == 0) {
                visit_neighbors(y, x+1);
            } else if(visited_s[y][x+1] == 2) {
                seen_opposite_color = true;
            }
        }
        if(x > 0) {
            if(visited_s[y][x-1] == 0) {
                visit_neighbors(y, x-1);
            } else if(visited_s[y][x-1] == 2) {
                seen_opposite_color = true;
            }
        }
    };

    visit_neighbors(yy,xx);
    return !seen_opposite_color;
}

bool is_surrounded_by_white(int yy, int xx) {
    bool seen_opposite_color = false;
    auto visited_s = board;

    std::function<void(int,int)> visit_neighbors =
        [&](int y, int x) {
            visited_s[y][x] = 3; //an empty, visited square gets the value 3
            if(y > 0) {
                if(visited_s[y-1][x] == 0) {
                    visit_neighbors(y-1, x);
                } else if(visited_s[y-1][x] == 1) {
                    seen_opposite_color = true;
                }
            }
            if(y < N-1) {
                if(visited_s[y+1][x] == 0) {
                    visit_neighbors(y+1, x);
                } else if(visited_s[y+1][x] == 1) {
                    seen_opposite_color = true;
                }
            }
            if(x < N-1) {
                if(visited_s[y][x+1] == 0) {
                    visit_neighbors(y, x+1);
                } else if(visited_s[y][x+1] == 1) {
                    seen_opposite_color = true;
                }
            }
            if(x > 0) {
                if(visited_s[y][x-1] == 0) {
                    visit_neighbors(y, x-1);
                } else if(visited_s[y][x-1] == 1) {
                    seen_opposite_color = true;
                }
            }
        };

    visit_neighbors(yy,xx);
    return !seen_opposite_color;
}

int count_area_black() {
    int area_black = 0;
    for(int y = 0; y < N; y++) {
        for(int x = 0; x < N; x++) {
            if(board[y][x] == 1) {
                area_black++;
            } else if(board[y][x] == 0) {
                if(is_surrounded_by_black(y,x)) {
                    area_black++;
                }
            }
        }
    }

    return area_black;
}

int count_area_white() {
    int area_white = 0;
    for(int y = 0; y < N; y++) {
        for(int x = 0; x < N; x++) {
            if(board[y][x] == 2) {
                area_white++;
            } else if(board[y][x] == 0) {
                if(is_surrounded_by_white(y,x)) {
                    area_white++;
                }
            }
        }
    }

    return area_white;
}





int init_go() {
    quit = false;

    //P = 40;
    turn = 0;
    at_least_one_liberty_is_found = false;
    at_least_one_liberty_is_found_potential = false;
    consecutive_passes = 0;
    game_ended = false;


    /*std::cout << "N = ";
    std::cin >> N;
    std::cout << "komi = ";
    std::cin >> komi;
    std::cout << "\n\n\n\n\n\n";*/

    //N and komi are no longer asked for when starting the program. One must change these variables in go_rules.h

    std::vector<std::vector<int> > boardm(N, std::vector<int>(N,0));
    std::vector<std::vector<int> > potential_boardm(N, std::vector<int>(N,0));
    std::vector<std::vector<bool> > visitedm(N, std::vector<bool>(N,false));
    std::vector<std::vector<bool> > visited_potentialm(N, std::vector<bool>(N,false));

    board = boardm;
    potential_board = potential_boardm;
    visited = visitedm;
    visited_potential = visited_potentialm;

    return 0;
}
