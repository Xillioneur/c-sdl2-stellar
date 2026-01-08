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
#define SHIP_THRUST 0.14f
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

void spawn_particle(float x, float y, float vx, float vy, Uint32 color, float life) {
    if (particle_cnt >= MAX_PARTICLES) return;
    particles[particle_cnt++] = (Particle){x, y, vx, vy, life, color, 1};

}

void thrust_flame() {
    float rear = ship.angle + M_PI;
    float px = ship.x + cosf(rear) * 24;
    float py = ship.y + sinf(rear) * 24;
    for (int i = 0; i < 14; i++) {
        float ang = rear + (rand() % 90 - 45) * 0.018f;
        float spd = 7.0f + (rand() % 70) / 10.0f;
        Uint32 c = (rand() % 2 == 0) ? 0xFFFFAAFF : 0xFFFFFFFF;
        spawn_particle(px, py,
                    cosf(ang) * spd + ship.vx * 0.3f,
                    sinf(ang) * spd + ship.vy * 0.3f,
                    c, 25 + rand() % 20);
    }
}

void trail_emit() {
    float speed = hypotf(ship.vx, ship.vy);
    if (speed < 4.0f || frame % 2 != 0) return;
    float rear = atan2f(ship.vy, ship.vx) + M_PI;
    float px = ship.x + cosf(rear) * 22;
    float py = ship.y + sinf(rear) * 22;
    for (int i = 0; i < 5; i++) {
        float ang = rear + (rand() % 80 - 40) * 0.015f;
        float spd = 2.0f + speed * 0.35f;
        spawn_particle(px, py, cosf(ang) * spd, sinf(ang) * spd, 0xCCEEFFFF, 30 + rand() % 25);
    }
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

void break_asteroid(int idx) {
    Asteroid* a = &asteroids[idx];
    int new_size = a->size - 1;
    ship.score += (4 - a->size) * 80;

    // TODO: Add explosion
    if (new_size >= 1) {
        int pieces = 2 + rand() % 2;
        for (int i = 0; i < pieces; i++) {
            spawn_asteroid(new_size);
            Asteroid* na = &asteroids[asteroid_cnt - 1];
            na->x = a->x;
            na->y = a->y;
            float dir = i * 2 * M_PI / pieces + (rand() % 100 - 50) * M_PI / 180.0f;
            float spd = hypotf(a->vx, a->vy) * 1.6f + 1.0f;
            na->vx = cosf(dir) * spd + a->vx *0.4f;
            na->vy = sinf(dir) * spd + a->vy * 0.4f;
        }
    } 
}

void generate_planet(Planet* p) {
    int tries = 0;
    bool good = false;
    while (!good && tries < 100) {
        float orbit_radius = 150 + powf(M_E, tries * 0.03f) * (200 + rand() % 600);
        float angle = (rand() % 360) * M_PI / 180.0f;
        p->base_x = sun.base_x + cosf(angle) * orbit_radius;
        p->base_y = sun.base_y + sinf(angle) * orbit_radius * 0.6f;
        p->radius = 30 + rand() % 30;

        int hue = rand() % 360;
        float sat = 0.6f + (rand() % 40)/100.0f;
        float val = 0.8f + (rand() % 20)/100.0f;
        float c = val * sat;
        float hp = hue / 60.0f;
        float x = c * (1 - fabsf(fmodf(hp, 2) - 1));
        Uint8 r,g,b;
        if (hp < 1) { r = c*255; g = x*255; b = 0; }
        else if (hp < 2) { r = x*255; g = c*255; b = 0; }
        else if (hp < 3) { r = 0; g = c*255; b = x*255; }
        else if (hp < 4) { r = 0; g = x*255; b = c*255; }
        else if (hp < 5) { r = x*255; g = 0; b = c*255; }
        else { r = c*255; g = 0; b = x*255; }
        p->color = (r << 16) | (g << 8) | b | 0xFF;
        p->spin = 0.0f;
        p->has_ring = rand() % 3 == 0;

        good = true;
        for (int i = 0; i < NUM_PLANETS; i++) {
            if (&planets[i] == p) continue;
            float dx = planets[i].base_x - p->base_x;
            float dy = planets[i].base_y - p->base_y;
            float dist = hypotf(dx, dy);
            float min_dist = planets[i].radius + p->radius + 250.0f;
            if (dist < min_dist) {
                good = false;
                break;
            }
        }
        tries++;
    }
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

    for (int i = 0; i < NUM_STARS; i++) {
        stars[i].base_x = (rand() % 80000) - 40000;
        stars[i].base_y = rand() % WINDOW_H;
        stars[i].brightness = 100 + rand() % 155;
        stars[i].phase = rand() % 256;
        stars[i].size = 1 + (rand() % 3);
    }

    for (int i = 0; i < NUM_PLANETS; i++) {
        generate_planet(&planets[i]);
    }

    sun.base_x = WINDOW_W * 0.75f;
    sun.base_y = WINDOW_H * 0.3f;
    sun.radius = 120.0f;
    sun.pulse_phase = 0.0f;
}

float distance(float x1, float y1, float x2, float y2) {
    float dx = x1 - x2;
    float dy = y1 - y2;
    if (fabsf(dx) > WINDOW_W / 2) dx -= (dx > 0 ? WINDOW_W : -WINDOW_W);
    if (fabsf(dy) > WINDOW_H / 2) dy -= (dy > 0 ? WINDOW_H : -WINDOW_H);
    return hypotf(dx, dy);
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
    sun.pulse_phase += 0.015f;

    // Planets
    for (int i = 0; i < NUM_PLANETS; i++) {
        planets[i].base_x -= 0.12f + wave * 0.008f;
        if (planets[i].base_x < -500) {
            generate_planet(&planets[i]);
        }
    }
    
    if (left) ship.angle -= SHIP_ROT_SPEED;
    if (right) ship.angle += SHIP_ROT_SPEED;
    if (thrust) {
        ship.vx += cosf(ship.angle) * SHIP_THRUST;
        ship.vy += sinf(ship.angle) * SHIP_THRUST;
        thrust_flame();
    }
    trail_emit();

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

    for (int i = particle_cnt - 1; i >= 0; i--) {
        Particle* p = &particles[i];
        p->x += p->vx;
        p->y += p->vy;
        p->vy += 0.08f;
        p->life -= 1.0f;
        if (p->life <= 0) {
            particles[i] = particles[--particle_cnt];
        } else {
            wrap(&p->x, &p->y);
        }
    }

    for (int bi = 0; bi < MAX_BULLETS; bi++) {
        Bullet* pb = &player_bullets[bi];
        if (!pb->active) continue;
        bool hit_something = false;
        for (int ai = 0; ai < asteroid_cnt; ai++) {
            Asteroid* a = &asteroids[ai];
            if (!a->active) continue;
            float rad = a->size == 3 ? 50 : a->size == 2 ? 28 : 14;
            if (distance(pb->x, pb->y, a->x, a->y) < rad + 8) {
                if (--pb->hits <= 0) { pb->active = 0; hit_something = true; }
                break_asteroid(ai);
                asteroids[ai] = asteroids[--asteroid_cnt];
                ai--;
                if (hit_something) break;
            }
        }
        if (hit_something) continue;
        // TODO: Check collision with UFOs
    }

    if (ship.invincible == 0) {
        bool hit = false;
        // TODO: Check if enemy bullets hit the player
        if (!hit) {
            for (int i = 0; i < asteroid_cnt; i++) {
                Asteroid* a = &asteroids[i];
                if (a->active) {
                    float rad = a->size == 3 ? 50 : a->size == 2 ? 28 : 14;
                    if (distance(a->x, a->y, ship.x, ship.y) < rad + 24) {
                        break_asteroid(i);
                        asteroids[i] = asteroids[--asteroid_cnt];
                        hit = true;
                        break;
                    }
                }
            }
        }
        if (hit) {
            ship.lives--;
            ship.invincible = INVINCIBLE_FRAMES;
            ship.vx = ship.vy = 0;
            ship.x = WINDOW_W / 2.0f;
            ship.y = WINDOW_H / 2.0f;
            // TODO: Add explosion
            if (ship.lives <= 0) {
                printf("Game Over! Score: %d | Wave: %d | Ore Collected: %d\n", ship.score, wave, ore);
                init_game();
            }
        }

        if (asteroid_cnt == 0) {
            // TODO: Laser Cannon
            wave++;
            int new_asts = 4 + wave * 2;
            for (int i = 0; i < new_asts; i++) spawn_asteroid(3);
        }
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
    
    // twinkling stars
    for (int i = 0; i < NUM_STARS; i++) {
        float px = stars[i].base_x - scrollX * 0.15f;
        px = fmodf(px + 100000, 200000) - 100000;
        if (px < -50 || px > WINDOW_W + 50) continue;
        float tw = 0.6f + 0.4f * sinf(frame * 0.07f + stars[i].phase);
        int br = (int)(stars[i].brightness * tw);
        SDL_SetRenderDrawColor(renderer, br, br, 255, 255);
        int sx = (int)px;
        int sy = (int)stars[i].base_y;
        for (int s = -stars[i].size; s <= stars[i].size; s++) {
            SDL_RenderDrawPoint(renderer, sx + s, sy);
            SDL_RenderDrawPoint(renderer, sx, sy + s);
        }
    }

    // Sun with pulse and corona
    float sun_pulse = 1.0f + 0.15f * sinf(sun.pulse_phase);
    float sun_r = sun.radius * sun_pulse;
    SDL_SetRenderDrawColor(renderer, 255, 255, 200, 100);
    for (int r = (int)sun_r + 60; r > (int)sun_r + 20; r -= 15) {
        for (int dy = -r; dy <= r; dy += 10) {
            int hw = (int)sqrtf(r*r - dy*dy);
            SDL_RenderDrawLine(renderer, (int)sun.base_x - hw, (int)(sun.base_y + dy),
                                (int)sun.base_x + hw, (int)(sun.base_y + dy));
        }
    }
    SDL_SetRenderDrawColor(renderer, 255, 240, 150, 255);
    for (int dy = -sun_r; dy <= sun_r; dy += 6) {
        int hw = (int)sqrtf(sun_r*sun_r - dy*dy);
        SDL_RenderDrawLine(renderer, (int)sun.base_x - hw, (int)(sun.base_y + dy),
                            (int)sun.base_x + hw, (int)(sun.base_y + dy));
    }

    // distant planets 
    for (int i = 0; i < NUM_PLANETS; i++) {
        Planet* p = &planets[i];
        float px = p->base_x;
        if (px < -300 || px > WINDOW_W + 300) continue;

        p->spin += 0.002f;
        int r = (int)p->radius;
        for (int dy = -r; dy <= r; dy += 5) {
            int hw = (int)sqrtf(r*r - dy*dy);
            float swirl = sinf(dy * 0.03f + p->spin * 4);
            int alpha = 160 + (int)(95 * swirl);
            Uint32 c = p->color;
            SDL_SetRenderDrawColor(renderer, (c>>16)&255, (c>>8)&255, c&255, alpha);
            SDL_RenderDrawLine(renderer, (int)(px - hw), (int)(p->base_y + dy),
                                (int)(px + hw), (int)(p->base_y + dy));
        }
        // Rim glow
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 100);
        for (int dy = -r; dy <= r; dy += 15) {
            int hw = (int)sqrtf(r*r - dy*dy) + 8;
            SDL_RenderDrawLine(renderer, (int)(px - hw), (int)(p->base_y + dy),
                                (int)(px + hw), (int)(p->base_y + dy));
        }
        // Rings for some planets
        if (p->has_ring) {
            SDL_SetRenderDrawColor(renderer, 220, 220, 255, 120);
            int ring_w = r / 3;
            for (int off = -ring_w; off <= ring_w; off += 6) {
                SDL_RenderDrawLine(renderer, (int)(px - r*1.6f), (int)(p->base_y + off),
                                    (int)(px + r*1.6f), (int)(p->base_y + off));
            }
        }
    }

    // particles
    for (int i = 0; i < particle_cnt; i++) {
        Particle* p = &particles[i];
        int alpha = (int)(255 * (p->life / 60.0f));
        if (alpha < 30) continue;
        SDL_SetRenderDrawColor(renderer, (p->color>>16)&255, (p->color>>8)&255, p->color&255, alpha);
        int px = (int)p->x, py = (int)p->y;
        SDL_RenderDrawPoint(renderer, px, py);
        SDL_RenderDrawPoint(renderer, px+1, py);
        SDL_RenderDrawPoint(renderer, px-1, py);
        SDL_RenderDrawPoint(renderer, px, py+1);
        SDL_RenderDrawPoint(renderer, px, py-1);
    }

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