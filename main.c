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

typedef struct {
    float x, y, vx, vy, angle;
    int lives, score, invincible;
} Ship;

typedef struct {
    float x, y, vx, vy, angle, spin;
    int size;
    int vertices;
    float offsets[14][2];
    int active;
} Asteroid;

typedef struct {
    float x, y, vx, vy;
    int life, hits;
    Uint32 color;
    int active;
} Bullet;

typedef struct {
    float x, y, vx, vy;
    float life;
    Uint32 color;
    int active;
} Particle;

typedef struct {
    float x, y;
    float life;
    int active;
} OreBlob;

typedef struct {
    float x, y, vx;
    int shoot_timer;
    int active;
} UFO;

typedef struct { float base_x, base_y; int brightness, phase, size; } Star;
typedef struct { float base_x, base_y; float vx; int size; } Debris;
typedef struct { float base_x, base_y; float radius; Uint32 color; float spin; bool has_ring; } Planet;

typedef struct {
    float base_x, base_y;
    float radius;
    float pulse_phase;
} Sun;


Ship ship;
Asteroid asteroids[MAX_ASTEROIDS];
Bullet player_bullets[MAX_BULLETS];
Bullet enemy_bullets[MAX_ENEMY_BULLETS];
Particle particles[MAX_PARTICLES];
OreBlob oreblobs[MAX_OREBLOBS];
UFO ufos[2];
Star stars[NUM_STARS];
Debris debris[NUM_DEBRIS];
Planet planets[NUM_PLANETS];
Sun sun;

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;

void init_game() {
    srand(time(NULL));
    ship = (Ship){WINDOW_W / 2.0f, WINDOW_H / 2.0f, 0, 0, -M_PI / 2, 4, 0, 0};
}

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO);
    window = SDL_CreateWindow("Stellar", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_W, WINDOW_H, 0);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    init_game();

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