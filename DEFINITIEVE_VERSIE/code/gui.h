#ifndef GUI_H
#define GUI_H

SDL_Surface* load(std::string filename);
SDL_Texture* makeTexture(std::string filename);
void render_board();
std::tuple<int, int> GUI_make_move(int player);

#endif
