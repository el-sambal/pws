#ifndef GO_RULES_H
#define GO_RULES_H

inline double komi;
inline int N;
inline int P = 40;
inline std::vector<std::vector<int>> board;
inline std::vector<std::vector<std::vector<int>> > former_positions;
inline std::vector<std::vector<int>> potential_board;
inline int turn = 0;
inline std::vector<std::vector<bool>> visited;
inline std::vector<std::vector<bool>> visited_potential;
inline bool at_least_one_liberty_is_found;
inline bool at_least_one_liberty_is_found_potential;
inline int consecutive_passes = 0;
inline bool game_ended = false;
inline SDL_Window *window;
inline SDL_Renderer *renderer;
inline SDL_Surface *screen_surf;
inline bool quit = false;

void remove_all_visited_blocks();
void remove_all_visited_blocks_potential();
bool vectorcontains(std::vector<std::vector<std::vector<int>> > &v1, std::vector<std::vector<int>> &v2);
bool has_potential_liberties(int y, int x, bool initial_function_call);
bool has_liberties(int y, int x, bool initial_function_call);
void update_board(int choice_y, int choice_x, int player);
void update_potential_board(int choice_y, int choice_x, int player);
bool move_is_legal(int choice_y, int choice_x, int player);
std::vector<std::tuple<int,int>> all_legal_moves(int player);
void print_board();
bool is_surrounded_by_black(int yy, int xx);
bool is_surrounded_by_white(int yy, int xx);
int count_area_black();
int count_area_white();

int init_go();
#endif
