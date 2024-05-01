#include <iostream>
#include <tuple>
#include <vector>
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
#include "go_rules.h"


SDL_Surface* load(std::string filename) { //creates surface
    SDL_Surface* surf = IMG_Load(filename.c_str());
    if(surf == NULL) {
        std::cout << "ERROR. Could not load image" << IMG_GetError() << "\n";
        exit(0);
    }
    return surf;
}

SDL_Texture* makeTexture(std::string filename) {
    SDL_Texture *texture;
    texture = SDL_CreateTextureFromSurface(renderer, load(filename));
    if(texture == NULL) {
        std::cout << "Woops." << SDL_GetError() << "\n";
    }
    return texture;
}



void render_board() {

    SDL_RenderClear(renderer);

    SDL_Rect board_png;
    board_png.x = 0;
    board_png.y = 0;
    board_png.w = P*N;
    board_png.h = P*N;
    SDL_RenderCopy( renderer, PNGsurface, NULL, &board_png);
    for (int y = 0; y < N; y++) {
        for (int x = 0; x < N; x++) {
            if(komi > 0) {
                if (board[y][x] == 1) {

                    SDL_Rect stone;
                    stone.x = P*x;
                    stone.y = P*y;
                    stone.w = P;
                    stone.h = P;
                    SDL_RenderCopy( renderer, black_stone, NULL, &stone);
                }
                if (board[y][x] == 2) {
                    SDL_Rect stone;
                    stone.x = P*x;
                    stone.y = P*y;
                    stone.w = P;
                    stone.h = P;
                    SDL_RenderCopy( renderer, white_stone, NULL, &stone);
                }
            } else { //komi is negative: render with black/white inverted so that it seems to the user that black starts the game, even though internally it is white who starts the game. In order to compensate, we have to invert black/white here, only while rendering.
                if (board[y][x] == 1) {
                    SDL_Rect stone;
                    stone.x = P*x;
                    stone.y = P*y;
                    stone.w = P;
                    stone.h = P;
                    SDL_RenderCopy( renderer, white_stone, NULL, &stone);
                }
                if (board[y][x] == 2) {

                    SDL_Rect stone;
                    stone.x = P*x;
                    stone.y = P*y;
                    stone.w = P;
                    stone.h = P;
                    SDL_RenderCopy( renderer, black_stone, NULL, &stone);

                }

            }

        }
    }
    SDL_RenderPresent(renderer);
}




std::tuple<int, int> GUI_make_move(int player) {
    int choice_y, choice_x;
    for (int i = 0; i < 1; i++) {
        // std::cin >> choice_y >> choice_x;
        SDL_Event event;
        bool has_clicked_or_passed = false;
        bool pass = false;

        while (!has_clicked_or_passed && (!quit && (SDL_PollEvent(&event) || !has_clicked_or_passed))) {

            if (event.type == SDL_MOUSEBUTTONDOWN) {
                has_clicked_or_passed = true;
            } else if (event.type == SDL_QUIT) {
                quit = true;
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.scancode == SDL_SCANCODE_P) {
                    pass = true;
                    //std::cout << "GUI: Speler " << player << " heeft een zet overgeslagen.\n";
                    has_clicked_or_passed = true;
                }
            }
            render_board();
        }

        if (!quit) {
            // SDL_Delay(1000);

            if (!pass) {
                choice_y = (event.button.y - (event.button.y % P)) / P;
                choice_x = (event.button.x - (event.button.x % P)) / P;
            } else {
                choice_y = -1;
                choice_x = -1;
            }

            if (!move_is_legal(choice_y, choice_x, player)) {
                i--;
            }
        }
    }



    return std::make_tuple((int)choice_y, (int)choice_x);
}
