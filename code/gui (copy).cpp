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

#include "globals.h"
#include "go_rules.h"

void render_board() {
    SDL_RenderClear(renderer);
    for (int y = 0; y < N; y++) {
        for (int x = 0; x < N; x++) {
            if(komi > 0) {
                if (board[y][x] == 0)
                    SDL_SetRenderDrawColor(renderer, 255, 100, 0, 255);
                if (board[y][x] == 1)
                    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                if (board[y][x] == 2)
                    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
            } else { //komi is negative: render with black/white inverted so that it seems to the user that black starts the game, even though internally it is white who starts the game. In order to compensate, we have to invert black/white here, only while rendering.
                if (board[y][x] == 0)
                    SDL_SetRenderDrawColor(renderer, 255, 100, 0, 255);
                if (board[y][x] == 1)
                    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
                if (board[y][x] == 2)
                    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            }
            SDL_Rect rectangle;
            rectangle.y = P * y;
            rectangle.x = P * x;
            rectangle.w = P;
            rectangle.h = P;
            SDL_RenderFillRect(renderer, &rectangle);
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
                // std::cout << "Je hebt met de muis geklikt op: " << event.button.x << " " << event.button.y << "\n";
                // std::cout << "Je hebt met de muis geklikt op: " << event.motion.x << " " << event.motion.y << "\n";
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
                std::cout << "\nThe move (" << choice_y << " " << choice_x << ") is illegal.\n";
                SDL_Delay(50);
                i--;
            }
        }
    }
    
    
    
    return std::make_tuple((int)choice_y, (int)choice_x);
}
