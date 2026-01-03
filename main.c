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
