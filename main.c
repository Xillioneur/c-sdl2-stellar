#define _USE_MATH_DEFINES
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>

#define WINDOW_W 1200
#define WINDOW_H 675

#define MAX_ASTEROIDS 60
#define MAX_BULLETS 50
#define MAX_ENEMY_BULLETS 20
#define MAX_PARTICLES 350
#define MAX_OREBLOBS 80
#define NUM_STARS 600
#define NUM_DEBRIS 300
#define NUM_PLANETS 7

#define SHIP_ROT_SPEED 0.12f
#define SHIP_THRUST 0.48f
#define BULLET_SPEED 14.0f
#define BULLET_LIFE 52
#define INVINCIBLE_FRAMES 140
#define ORE_THRESHOLD 1000

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO);
    window = SDL_CreateWindow("Stellar", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_W, WINDOW_H, 0);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    bool running = true;
    while (running) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) running = false;
        }

        SDL_Delay(16);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    return 0;
}