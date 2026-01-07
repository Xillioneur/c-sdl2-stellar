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

#define SHIP_ROT_SPEED 0.09f
#define SHIP_THRUST 0.10f
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

int asteroid_cnt = 0;
int pbullet_cnt = 0;
int ebullet_cnt = 0;
int particle_cnt = 0;
int oreblob_cnt = 0;
int ufo_cnt = 0;
int wave = 0;
int ore = 0;
bool laser_unlocked = false;
int frame = 0;
float scrollX = 0.0f;

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;

void wrap(float* x, float* y) {
    *x = fmodf(*x + WINDOW_W * 10, WINDOW_W);
    *y = fmodf(*y + WINDOW_H * 10, WINDOW_H);
}

void thick_line(int x1, int y1, int x2, int y2, int thickness) {
    if (thickness <= 1) {
        SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
        return;
    }
    int dx = x2 - x1, dy = y2 - y1;
    float len = hypotf(dx, dy);
    if (len < 1.0f) return;
    float nx = -dy / len, ny = dx / len;
    for (int t = -thickness/2; t <= thickness/2; t++) {
        int ox = (int)(nx * t), oy = (int)(ny * t);
        SDL_RenderDrawLine(renderer, x1 + ox, y1 + oy, x2 + ox, y2 );
    }
}

void spawn_asteroid(int size) {
    if (asteroid_cnt >= MAX_ASTEROIDS) return;
    Asteroid* a = &asteroids[asteroid_cnt++];
    a->active = 1;
    a->size = size;

    float base_r = size == 3 ? 50.0f : size == 2 ? 28.0f : 14.0f;
    a->vertices = 9 + rand() % 6;
    for (int i = 0; i < a->vertices; i++) {
        float ang = i * 2.0f * M_PI / a->vertices;
        float vary = 0.7f + (rand() % 60) / 100.0f;
        float r = base_r * vary;
        a->offsets[i][0] = cosf(ang) * r;
        a->offsets[i][1] = sinf(ang) * r;
    }

    int tries = 0;
    do {
        a->x = rand() % WINDOW_W;
        a->y = rand() % WINDOW_H;
    } while (hypotf(a->x - ship.x, a->y - ship.y) < 200 && ++tries < 60);

    float speed = (0.8f - size * 0.9f) + wave * 0.4f;
    float dir = rand() * 2 * M_PI / RAND_MAX;
    a->vx = cosf(dir) * speed;
    a->vy = sinf(dir) * speed;
    a->spin = (rand() % 100 / 90.0f - 1.0f) * 0.06f;
    a->angle = 0;
}

void init_game() {
    srand(time(NULL));
    ship = (Ship){WINDOW_W / 2.0f, WINDOW_H / 2.0f, 0, 0, -M_PI / 2, 4, 0, 0};
    asteroid_cnt = pbullet_cnt = ebullet_cnt = particle_cnt = oreblob_cnt = ufo_cnt = 0;
    ore = 0;
    laser_unlocked = false;
    wave = 1;
    frame = 0;
    scrollX = 0.0f;

    int initial = 4 + wave;
    for (int i = 0; i < initial; i++) spawn_asteroid(3);
}

void update() {
    const Uint8* keys = SDL_GetKeyboardState(NULL);
    int left = keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT];
    int right = keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT];
    int thrust = keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP];
    static int prev_space = 0;
    int space = keys[SDL_SCANCODE_SPACE];
    int fire = space && !prev_space;
    prev_space = space;


    frame++;
    scrollX += 1.0f + wave * 0.08f;
    
    if (left) ship.angle -= SHIP_ROT_SPEED;
    if (right) ship.angle += SHIP_ROT_SPEED;
    if (thrust) {
        ship.vx += cosf(ship.angle) * SHIP_THRUST;
        ship.vy += sinf(ship.angle) * SHIP_THRUST;
    }

    if (fire) {
        int slot = pbullet_cnt;
        if (slot >= MAX_BULLETS) slot = rand() % MAX_BULLETS;
        Bullet* b = &player_bullets[slot];
        if (pbullet_cnt < MAX_BULLETS) pbullet_cnt++;
        b->active = 1;
        b->x = ship.x + cosf(ship.angle) * 28;
        b->y = ship.y + sinf(ship.angle) * 28;
        float bs = BULLET_SPEED * (laser_unlocked ? 1.5f : 1.0f);
        b->vx = cosf(ship.angle) * bs + ship.vx;
        b->vy = sinf(ship.angle) * bs + ship.vy;
        b->life = BULLET_LIFE;
        b->hits = laser_unlocked ? 3 : 1;
        b->color = laser_unlocked ? 0xFF3366FF : 0xFFFFFFFF;
        // TODO: Add explosion()
    }

    ship.x += ship.vx;
    ship.y += ship.vy;
    wrap(&ship.x, &ship.y);
    if (ship.invincible > 0) ship.invincible--;

    for (int i = 0; i < asteroid_cnt; i++) {
        if (!asteroids[i].active) continue;
        asteroids[i].x += asteroids[i].vx;
        asteroids[i].y += asteroids[i].vy;
        wrap(&asteroids[i].x, &asteroids[i].y);
        asteroids[i].angle += asteroids[i].spin;
    }

    for (int i = 0; i < MAX_BULLETS; i++) {
        Bullet* b = &player_bullets[i];
        if (!b->active) continue;
        b->x += b->vx;
        b->y += b->vy;
        wrap(&b->x, &b->y);
        if (--b->life <= 0) b->active = 0;
    }
}

void draw_ship() {
    Uint8 alpha = ship.invincible ? 120 + (Uint8)(135 * (sinf(frame * 0.35f) * 0.5f + 0.5f)) : 255;
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, alpha);

    float nx = ship.x + cosf(ship.angle) * 32;
    float ny = ship.y + sinf(ship.angle) * 32;
    float lx = ship.x + cosf(ship.angle + 2.7f) * 24;
    float ly = ship.y + sinf(ship.angle + 2.7f) * 24;
    float rx = ship.x + cosf(ship.angle - 2.7f) * 24;
    float ry = ship.y + sinf(ship.angle - 2.7f) * 24;
    float rearx = ship.x + cosf(ship.angle + M_PI) * 15;
    float reary = ship.y + sinf(ship.angle + M_PI) * 15;

    thick_line((int)nx, (int)ny, (int)lx, (int)ly, 5);
    thick_line((int)lx, (int)ly, (int)rearx, (int)reary, 5);
    thick_line((int)rearx, (int)reary, (int)rx, (int)ry, 5);
    thick_line((int)rx, (int)ry, (int)nx, (int)ny, 5);
}

void draw_asteroid(Asteroid* a) { 
    SDL_SetRenderDrawColor(renderer, 240, 240, 255, 255);
    for (int i = 0; i < a->vertices; i++) {
        int j = (i + 1) % a->vertices;
        float ox1 = a->offsets[i][0];
        float oy1 = a->offsets[i][1];
        float ox2 = a->offsets[j][0];
        float oy2 = a->offsets[j][1];
        int x1 = (int)(a->x + ox1 * cosf(a->angle) - oy1 * sinf(a->angle));
        int y1 = (int)(a->y + ox1 * sinf(a->angle) + oy1 * cosf(a->angle));
        int x2 = (int)(a->x + ox2 * cosf(a->angle) - oy2 * sinf(a->angle));
        int y2 = (int)(a->y + ox2 * sinf(a->angle) + oy2 * cosf(a->angle));
        thick_line(x1, y1, x2, y2, 4);
    }
}

void render() {
    SDL_SetRenderDrawColor(renderer, 5, 5, 15, 255);
    SDL_RenderClear(renderer);

    // asteroids
    for (int i = 0; i < asteroid_cnt; i++) if (asteroids[i].active) draw_asteroid(&asteroids[i]);

    // bullets
    for (int i = 0; i < MAX_BULLETS; i++) {
        Bullet* b = &player_bullets[i];
        if (!b->active) continue;
        int alpha = 200 + (int)(55 * sinf(frame * 0.2f));
        SDL_SetRenderDrawColor(renderer, (b->color>>16)&255, (b->color>>8)&255, b->color&255, alpha);
        int bx = (int)b->x, by = (int)b->y;
        for (int d = -3; d <= 3; d++) {
            SDL_RenderDrawPoint(renderer, bx + d, by);
            SDL_RenderDrawPoint(renderer, bx, by + d);
        }
    }


    draw_ship();

    SDL_RenderPresent(renderer);
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

        update();
        render();

        SDL_Delay(16);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    return 0;
}