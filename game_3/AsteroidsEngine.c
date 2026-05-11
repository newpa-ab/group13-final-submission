#include "AsteroidsEngine.h"

#include <math.h>
#include "AsteroidsAssets.h"
#include "LCD.h"
#include "Utils.h"

#define AST_SCREEN_W 240.0f
#define AST_SCREEN_H 240.0f
#define AST_SCREEN_CENTER_X (AST_SCREEN_W * 0.5f)
#define AST_SCREEN_CENTER_Y (AST_SCREEN_H * 0.5f)
#define AST_SHIP_BASE_SPRITE_INDEX 6U

#define AST_SHIP_THRUST 0.25f
#define AST_SHIP_MIN_THRUST_RATIO 0.18f
#define AST_SHIP_DRAG 0.985f
#define AST_SHIP_MAX_SPEED 4.0f

#define AST_BULLET_SPEED 6.5f
#define AST_BULLET_TTL 45
#define AST_FIRE_COOLDOWN_FRAMES 8
#define AST_DOUBLE_SHOT_FRAMES 600U
#define AST_MAGNET_FRAMES 600U
#define AST_MAGNET_RADIUS 72.0f
#define AST_POWERUP_TTL 720U

#define AST_INVULN_FRAMES 75
#define AST_PARTICLE_SPEED 2.2f

typedef struct {
    float x;
    float y;
} Vec2;

typedef enum {
    AST_BULLET_RAIL = 0,
    AST_BULLET_SPREAD,
    AST_BULLET_LANCE
} AsteroidBulletType_t;

static float wrapped_delta(float object_pos, float camera_pos, float world_size);
static Vec2 world_to_screen(AsteroidsEngine_t* engine, float world_x, float world_y);
static Vec2 world_to_screen_no_wrap(AsteroidsEngine_t* engine, float world_x, float world_y);
static uint8_t point_on_screen(Vec2 p, float margin);

static uint8_t difficulty_start_lives(AsteroidsDifficulty_t difficulty)
{
    switch (difficulty) {
    case AST_DIFFICULTY_EASY:
        return 4;
    case AST_DIFFICULTY_HARD:
        return 2;
    case AST_DIFFICULTY_NORMAL:
    default:
        return 3;
    }
}

static uint8_t difficulty_extra_rocks(AsteroidsDifficulty_t difficulty)
{
    switch (difficulty) {
    case AST_DIFFICULTY_EASY:
        return 0;
    case AST_DIFFICULTY_HARD:
        return 2;
    case AST_DIFFICULTY_NORMAL:
    default:
        return 1;
    }
}

static float difficulty_speed_scale(AsteroidsDifficulty_t difficulty)
{
    switch (difficulty) {
    case AST_DIFFICULTY_EASY:
        return 0.82f;
    case AST_DIFFICULTY_HARD:
        return 1.28f;
    case AST_DIFFICULTY_NORMAL:
    default:
        return 1.0f;
    }
}

static float wrapf(float v, float max_v)
{
    if (v < 0.0f) {
        v += max_v;
    }
    if (v >= max_v) {
        v -= max_v;
    }
    return v;
}

static float deg_to_rad(float deg)
{
    return deg * (3.14159265f / 180.0f);
}

static void compass_to_screen_vector(float angle_deg, float* x, float* y)
{
    float rad = deg_to_rad(angle_deg);
    *x = sinf(rad);
    *y = -cosf(rad);
}

static float compass_to_screen_angle(float angle_deg)
{
    return angle_deg - 90.0f;
}

static void clear_entities(AsteroidsEngine_t* engine)
{
    uint8_t i;

    for (i = 0; i < ASTEROIDS_MAX_BULLETS; i++) {
        engine->bullets[i].active = 0;
    }
    for (i = 0; i < ASTEROIDS_MAX_ROCKS; i++) {
        engine->rocks[i].active = 0;
    }
    for (i = 0; i < ASTEROIDS_MAX_PARTICLES; i++) {
        engine->particles[i].active = 0;
    }
    for (i = 0; i < ASTEROIDS_MAX_BURSTS; i++) {
        engine->bursts[i].active = 0;
    }
    for (i = 0; i < ASTEROIDS_MAX_POWERUPS; i++) {
        engine->powerups[i].active = 0;
    }
}

static void init_starfield(AsteroidsEngine_t* engine)
{
    uint8_t i;

    for (i = 0; i < ASTEROIDS_STAR_COUNT; i++) {
        engine->stars[i].x = (uint8_t)Random_U16((uint16_t)AST_SCREEN_W);
        engine->stars[i].y = (uint8_t)Random_U16((uint16_t)AST_SCREEN_H);
        engine->stars[i].colour = (i % 3 == 0) ? 13 : 1;
    }
}

static uint8_t count_active_rocks(AsteroidsEngine_t* engine)
{
    uint8_t i;
    uint8_t count = 0;

    for (i = 0; i < ASTEROIDS_MAX_ROCKS; i++) {
        if (engine->rocks[i].active) {
            count++;
        }
    }
    return count;
}

static uint8_t spawn_rock(AsteroidsEngine_t* engine, float x, float y, float vx, float vy, uint8_t radius)
{
    uint8_t i;

    for (i = 0; i < ASTEROIDS_MAX_ROCKS; i++) {
        if (!engine->rocks[i].active) {
            engine->rocks[i].x = x;
            engine->rocks[i].y = y;
            engine->rocks[i].vx = vx;
            engine->rocks[i].vy = vy;
            engine->rocks[i].radius = radius;
            engine->rocks[i].active = 1;
            return 1;
        }
    }

    return 0;
}

static void spawn_wave(AsteroidsEngine_t* engine)
{
    uint8_t i;
    uint8_t count = (uint8_t)(2 + engine->wave + difficulty_extra_rocks(engine->difficulty));
    float speed_scale = difficulty_speed_scale(engine->difficulty);

    if (count > 8) {
        count = 8;
    }

    for (i = 0; i < count; i++) {
        float x;
        float y;
        float vx;
        float vy;
        uint8_t safe;

        safe = 0;
        while (!safe) {
            float dx;
            float dy;
            x = (float)Random_U16((uint16_t)AST_SCREEN_W);
            y = (float)Random_U16((uint16_t)AST_SCREEN_H);
            dx = x - engine->ship_x;
            dy = y - engine->ship_y;
            safe = ((dx * dx) + (dy * dy) > (70.0f * 70.0f));
        }

        vx = ((float)((int16_t)Random_U16(200) - 100) / 100.0f) * (0.8f + (0.08f * engine->wave)) * speed_scale;
        vy = ((float)((int16_t)Random_U16(200) - 100) / 100.0f) * (0.8f + (0.08f * engine->wave)) * speed_scale;

        if (vx > -0.2f && vx < 0.2f) {
            vx = 0.35f;
        }
        if (vy > -0.2f && vy < 0.2f) {
            vy = -0.3f;
        }

        spawn_rock(engine, x, y, vx, vy, 12);
    }
}

static uint8_t spawn_single_bullet(AsteroidsEngine_t* engine, float angle_offset_deg, float side_offset,
                                   float speed, uint8_t ttl, uint8_t damage,
                                   uint8_t pierce_remaining, uint8_t type)
{
    uint8_t i;
    float shot_angle = engine->ship_angle_deg + angle_offset_deg;
    float dir_x;
    float dir_y;
    compass_to_screen_vector(shot_angle, &dir_x, &dir_y);
    float side_x = -dir_y;
    float side_y = dir_x;

    for (i = 0; i < ASTEROIDS_MAX_BULLETS; i++) {
        if (!engine->bullets[i].active) {
            engine->bullets[i].x = engine->ship_x + (dir_x * 10.0f) + (side_x * side_offset);
            engine->bullets[i].y = engine->ship_y + (dir_y * 10.0f) + (side_y * side_offset);
            engine->bullets[i].vx = engine->ship_vx + (dir_x * speed);
            engine->bullets[i].vy = engine->ship_vy + (dir_y * speed);
            engine->bullets[i].ttl = ttl;
            engine->bullets[i].damage = damage;
            engine->bullets[i].pierce_remaining = pierce_remaining;
            engine->bullets[i].type = type;
            engine->bullets[i].active = 1;
            return 1;
        }
    }

    return 0;
}

static uint8_t spawn_bullet(AsteroidsEngine_t* engine)
{
    uint8_t fired = 0;

    if (engine->ship_type == SHIP_SPREAD) {
        fired |= spawn_single_bullet(engine, 0.0f, 0.0f, 5.8f, 36U, 1U, 0U, AST_BULLET_SPREAD);
        fired |= spawn_single_bullet(engine, -22.0f, -3.5f, 5.5f, 34U, 1U, 0U, AST_BULLET_SPREAD);
        fired |= spawn_single_bullet(engine, 22.0f, 3.5f, 5.5f, 34U, 1U, 0U, AST_BULLET_SPREAD);
        if (engine->weapon_timer > 0U) {
            fired |= spawn_single_bullet(engine, -38.0f, -6.0f, 5.2f, 32U, 1U, 0U, AST_BULLET_SPREAD);
            fired |= spawn_single_bullet(engine, 38.0f, 6.0f, 5.2f, 32U, 1U, 0U, AST_BULLET_SPREAD);
        }
        return fired;
    }

    if (engine->ship_type == SHIP_LANCE) {
        uint8_t pierce = (engine->weapon_timer > 0U) ? 4U : 3U;
        return spawn_single_bullet(engine, 0.0f, 0.0f, 6.9f, 54U, 1U, pierce, AST_BULLET_LANCE);
    }

    if (engine->weapon_timer > 0U) {
        fired |= spawn_single_bullet(engine, 0.0f, -4.0f, 7.6f, 58U, 2U, 0U, AST_BULLET_RAIL);
        fired |= spawn_single_bullet(engine, 0.0f, 4.0f, 7.6f, 58U, 2U, 0U, AST_BULLET_RAIL);
        return fired;
    }

    return spawn_single_bullet(engine, 0.0f, 0.0f, 8.0f, 62U, 3U, 0U, AST_BULLET_RAIL);
}

static void spawn_powerup(AsteroidsEngine_t* engine, float x, float y)
{
    uint8_t i;
    uint8_t roll = (uint8_t)Random_U16(100);
    uint8_t type;

    if (roll >= 42U) {
        return;
    }

    if (roll < 12U) {
        type = AST_POWERUP_HEALTH;
    } else if (roll < 25U) {
        type = AST_POWERUP_DOUBLE_SHOT;
    } else if (roll < 35U) {
        type = AST_POWERUP_SHIELD;
    } else {
        type = AST_POWERUP_MAGNET;
    }

    for (i = 0; i < ASTEROIDS_MAX_POWERUPS; i++) {
        AsteroidPowerup_t* powerup = &engine->powerups[i];
        if (!powerup->active) {
            float angle = deg_to_rad((float)Random_U16(360));
            powerup->x = x;
            powerup->y = y;
            powerup->vx = cosf(angle) * 0.25f;
            powerup->vy = sinf(angle) * 0.25f;
            powerup->ttl = AST_POWERUP_TTL;
            powerup->type = type;
            powerup->active = 1;
            return;
        }
    }
}

static void spawn_burst(AsteroidsEngine_t* engine, float x, float y)
{
    uint8_t i;

    for (i = 0; i < ASTEROIDS_MAX_BURSTS; i++) {
        AsteroidBurst_t* burst = &engine->bursts[i];
        if (!burst->active) {
            burst->x = x;
            burst->y = y;
            burst->frame = 0;
            burst->tick = 0;
            burst->active = 1;
            return;
        }
    }
}

static void spawn_explosion(AsteroidsEngine_t* engine, float x, float y, uint8_t count, float speed, uint8_t colour, uint8_t ttl)
{
    uint8_t i;

    spawn_burst(engine, x, y);

    for (i = 0; i < ASTEROIDS_MAX_PARTICLES && count > 0; i++) {
        AsteroidParticle_t* p = &engine->particles[i];
        float angle_deg;
        float angle_rad;
        float burst_speed;

        if (p->active) {
            continue;
        }

        angle_deg = (float)Random_U16(360);
        angle_rad = deg_to_rad(angle_deg);
        burst_speed = speed + ((float)Random_U16(100) / 100.0f);

        p->x = x;
        p->y = y;
        p->vx = cosf(angle_rad) * burst_speed;
        p->vy = sinf(angle_rad) * burst_speed;
        p->ttl = ttl;
        p->colour = colour;
        p->active = 1;
        count--;
    }
}

static void update_ship(AsteroidsEngine_t* engine, UserInput input, uint8_t hyperspace_pressed)
{
    float speed;
    float thrust_ratio = 0.0f;
    uint8_t thrusting = 0;

    if (input.angle >= 0.0f && input.magnitude > 0.05f) {
        engine->ship_angle_deg = input.angle;
        thrust_ratio = input.magnitude;
        if (thrust_ratio < AST_SHIP_MIN_THRUST_RATIO) {
            thrust_ratio = AST_SHIP_MIN_THRUST_RATIO;
        }
        thrusting = 1;
    }

    engine->thrust_active = thrusting;

    if (thrusting) {
        float thrust_x;
        float thrust_y;
        compass_to_screen_vector(engine->ship_angle_deg, &thrust_x, &thrust_y);
        engine->ship_vx += thrust_x * (AST_SHIP_THRUST * thrust_ratio);
        engine->ship_vy += thrust_y * (AST_SHIP_THRUST * thrust_ratio);
    }

    if (hyperspace_pressed) {
        engine->ship_x = (float)Random_U16((uint16_t)AST_SCREEN_W);
        engine->ship_y = (float)Random_U16((uint16_t)AST_SCREEN_H);
        engine->ship_vx = 0.0f;
        engine->ship_vy = 0.0f;
    }

    engine->ship_vx *= AST_SHIP_DRAG;
    engine->ship_vy *= AST_SHIP_DRAG;

    speed = sqrtf((engine->ship_vx * engine->ship_vx) + (engine->ship_vy * engine->ship_vy));
    if (speed > AST_SHIP_MAX_SPEED) {
        float scale = AST_SHIP_MAX_SPEED / speed;
        engine->ship_vx *= scale;
        engine->ship_vy *= scale;
    }

    engine->ship_x = wrapf(engine->ship_x + engine->ship_vx, AST_SCREEN_W);
    engine->ship_y = wrapf(engine->ship_y + engine->ship_vy, AST_SCREEN_H);
}

static void update_particles(AsteroidsEngine_t* engine)
{
    uint8_t i;

    for (i = 0; i < ASTEROIDS_MAX_PARTICLES; i++) {
        AsteroidParticle_t* p = &engine->particles[i];
        if (!p->active) {
            continue;
        }

        p->x += p->vx;
        p->y += p->vy;
        p->vx *= 0.92f;
        p->vy *= 0.92f;

        if (p->ttl > 0) {
            p->ttl--;
        }
        if (p->ttl == 0) {
            p->active = 0;
        }
    }
}

static void update_bursts(AsteroidsEngine_t* engine)
{
    uint8_t i;

    for (i = 0; i < ASTEROIDS_MAX_BURSTS; i++) {
        AsteroidBurst_t* burst = &engine->bursts[i];
        if (!burst->active) {
            continue;
        }

        burst->tick++;
        if ((burst->tick % 3U) == 0U) {
            burst->frame++;
            if (burst->frame >= AST_EXPLOSION_SPRITE_COUNT) {
                burst->active = 0;
            }
        }
    }
}

static void update_bullets(AsteroidsEngine_t* engine)
{
    uint8_t i;
    const float offscreen_x = AST_SCREEN_CENTER_X + (float)AST_BULLET_SPRITE_W;
    const float offscreen_y = AST_SCREEN_CENTER_Y + (float)AST_BULLET_SPRITE_H;

    for (i = 0; i < ASTEROIDS_MAX_BULLETS; i++) {
        AsteroidBullet_t* b = &engine->bullets[i];
        float dx;
        float dy;

        if (!b->active) {
            continue;
        }

        b->x += b->vx;
        b->y += b->vy;

        dx = b->x - engine->ship_x;
        dy = b->y - engine->ship_y;
        if (dx < -offscreen_x || dx > offscreen_x || dy < -offscreen_y || dy > offscreen_y) {
            b->active = 0;
            continue;
        }

        if (b->ttl > 0) {
            b->ttl--;
        }
        if (b->ttl == 0) {
            b->active = 0;
        }
    }
}

static void update_powerups(AsteroidsEngine_t* engine)
{
    uint8_t i;

    for (i = 0; i < ASTEROIDS_MAX_POWERUPS; i++) {
        AsteroidPowerup_t* powerup = &engine->powerups[i];
        if (!powerup->active) {
            continue;
        }

        if (engine->magnet_timer > 0U) {
            float dx = wrapped_delta(engine->ship_x, powerup->x, AST_SCREEN_W);
            float dy = wrapped_delta(engine->ship_y, powerup->y, AST_SCREEN_H);
            float d2 = (dx * dx) + (dy * dy);
            if (d2 < (AST_MAGNET_RADIUS * AST_MAGNET_RADIUS) && d2 > 1.0f) {
                float d = sqrtf(d2);
                float pull = 0.18f + ((AST_MAGNET_RADIUS - d) * 0.006f);
                powerup->vx += (dx / d) * pull;
                powerup->vy += (dy / d) * pull;
            }
        }

        powerup->vx *= 0.96f;
        powerup->vy *= 0.96f;
        powerup->x = wrapf(powerup->x + powerup->vx, AST_SCREEN_W);
        powerup->y = wrapf(powerup->y + powerup->vy, AST_SCREEN_H);

        if (powerup->ttl > 0U) {
            powerup->ttl--;
        }
        if (powerup->ttl == 0U) {
            powerup->active = 0;
        }
    }
}

static void update_rocks(AsteroidsEngine_t* engine)
{
    uint8_t i;

    for (i = 0; i < ASTEROIDS_MAX_ROCKS; i++) {
        AsteroidRock_t* r = &engine->rocks[i];
        if (!r->active) {
            continue;
        }

        r->x = wrapf(r->x + r->vx, AST_SCREEN_W);
        r->y = wrapf(r->y + r->vy, AST_SCREEN_H);
    }
}

static uint8_t collide_circle(float x1, float y1, float r1, float x2, float y2, float r2)
{
    float dx = x1 - x2;
    float dy = y1 - y2;
    float rr = r1 + r2;
    return ((dx * dx + dy * dy) <= (rr * rr));
}

static uint8_t split_rock(AsteroidsEngine_t* engine, AsteroidRock_t* rock)
{
    float speed = 1.3f + ((float)engine->wave * 0.1f);
    uint8_t ok1;
    uint8_t ok2;

    ok1 = spawn_rock(engine, rock->x, rock->y, speed, -speed, 7);
    ok2 = spawn_rock(engine, rock->x, rock->y, -speed, speed, 7);
    return (ok1 || ok2);
}

static uint8_t handle_collisions(AsteroidsEngine_t* engine)
{
    uint8_t i;
    uint8_t j;
    uint8_t events = AST_EVENT_NONE;

    for (i = 0; i < ASTEROIDS_MAX_BULLETS; i++) {
        AsteroidBullet_t* b = &engine->bullets[i];
        if (!b->active) {
            continue;
        }

        for (j = 0; j < ASTEROIDS_MAX_ROCKS; j++) {
            AsteroidRock_t* r = &engine->rocks[j];
            if (!r->active) {
                continue;
            }

            Vec2 bullet_screen = world_to_screen_no_wrap(engine, b->x, b->y);
            Vec2 rock_screen = world_to_screen(engine, r->x, r->y);

            if (!point_on_screen(bullet_screen, 8.0f)) {
                continue;
            }

            if (collide_circle(bullet_screen.x, bullet_screen.y, 1.5f,
                               rock_screen.x, rock_screen.y, (float)r->radius)) {
                if (b->type == AST_BULLET_LANCE && b->pierce_remaining > 1U) {
                    b->pierce_remaining--;
                } else {
                    b->active = 0;
                }

                if (r->radius >= 10U && b->damage < 2U) {
                    r->radius = 7U;
                    engine->score += 20;
                    spawn_explosion(engine, r->x, r->y, 7, AST_PARTICLE_SPEED, 6, 12);
                    events |= AST_EVENT_ROCK_HIT;
                    break;
                }

                r->active = 0;
                engine->rocks_destroyed++;
                engine->score += (r->radius >= 10) ? 20 : 50;
                events |= AST_EVENT_ROCK_HIT;
                spawn_explosion(engine, r->x, r->y, (r->radius >= 10) ? 12 : 7,
                                AST_PARTICLE_SPEED + 0.4f, 6, (r->radius >= 10) ? 18 : 13);
                spawn_powerup(engine, r->x, r->y);

                if (r->radius >= 10) {
                    split_rock(engine, r);
                }
                if (!b->active) {
                    break;
                }
            }
        }
    }

    for (i = 0; i < ASTEROIDS_MAX_POWERUPS; i++) {
        AsteroidPowerup_t* powerup = &engine->powerups[i];
        if (!powerup->active) {
            continue;
        }

        if (collide_circle(engine->ship_x, engine->ship_y, 9.0f, powerup->x, powerup->y, 9.0f)) {
            uint8_t max_lives = difficulty_start_lives(engine->difficulty);

            if (powerup->type == AST_POWERUP_HEALTH) {
                if (engine->lives < max_lives) {
                    engine->lives++;
                }
            } else if (powerup->type == AST_POWERUP_DOUBLE_SHOT) {
                engine->weapon_timer = AST_DOUBLE_SHOT_FRAMES;
            } else if (powerup->type == AST_POWERUP_SHIELD) {
                engine->shield_active = 1U;
            } else if (powerup->type == AST_POWERUP_MAGNET) {
                engine->magnet_timer = AST_MAGNET_FRAMES;
            }

            powerup->active = 0;
            spawn_explosion(engine, powerup->x, powerup->y, 6, 1.4f, 14, 12);
            events |= AST_EVENT_POWERUP;
        }
    }

    if (engine->invuln_frames == 0) {
        for (j = 0; j < ASTEROIDS_MAX_ROCKS; j++) {
            AsteroidRock_t* r = &engine->rocks[j];
            if (!r->active) {
                continue;
            }

            if (collide_circle(engine->ship_x, engine->ship_y, 7.0f, r->x, r->y, (float)r->radius)) {
                float hit_x = engine->ship_x;
                float hit_y = engine->ship_y;

                if (engine->shield_active) {
                    engine->shield_active = 0U;
                    spawn_explosion(engine, hit_x, hit_y, 8, AST_PARTICLE_SPEED + 0.2f, 14, 16);
                    engine->invuln_frames = AST_INVULN_FRAMES / 2U;
                    events |= AST_EVENT_SHIELD_BLOCK;
                    break;
                } else if (engine->lives > 0) {
                    engine->lives--;
                }

                spawn_explosion(engine, hit_x, hit_y, 10, AST_PARTICLE_SPEED + 0.6f, 7, 18);
                engine->ship_x = AST_SCREEN_W * 0.5f;
                engine->ship_y = AST_SCREEN_H * 0.5f;
                engine->ship_vx = 0.0f;
                engine->ship_vy = 0.0f;
                engine->invuln_frames = AST_INVULN_FRAMES;
                events |= AST_EVENT_SHIP_HIT;

                if (engine->lives == 0) {
                    engine->game_over = 1;
                }
                break;
            }
        }
    }

    return events;
}

void AsteroidsEngine_Init(AsteroidsEngine_t* engine)
{
    engine->difficulty = AST_DIFFICULTY_NORMAL;
    engine->ship_type = SHIP_RAIL;
    AsteroidsEngine_StartGame(engine);
}

void AsteroidsEngine_StartGame(AsteroidsEngine_t* engine)
{
    AsteroidsDifficulty_t difficulty = engine->difficulty;

    engine->ship_x = AST_SCREEN_W * 0.5f;
    engine->ship_y = AST_SCREEN_H * 0.5f;
    engine->ship_vx = 0.0f;
    engine->ship_vy = 0.0f;
    engine->ship_angle_deg = 0.0f;

    engine->score = 0;
    engine->difficulty = difficulty;
    engine->lives = difficulty_start_lives(engine->difficulty);
    engine->wave = 1;
    engine->game_over = 0;
    engine->fire_cooldown = 0;
    engine->invuln_frames = 0;
    engine->thrust_active = 0;
    engine->shield_active = 0;
    if (engine->ship_type >= SHIP_COUNT) {
        engine->ship_type = SHIP_RAIL;
    }
    engine->weapon_timer = 0;
    engine->magnet_timer = 0;
    engine->frame_counter = 0;
    engine->survival_frames = 0U;
    engine->rocks_destroyed = 0U;
    engine->max_wave_reached = 1U;

    clear_entities(engine);
    init_starfield(engine);
    spawn_wave(engine);
}

void AsteroidsEngine_SetDifficulty(AsteroidsEngine_t* engine, AsteroidsDifficulty_t difficulty)
{
    if (difficulty > AST_DIFFICULTY_HARD) {
        difficulty = AST_DIFFICULTY_NORMAL;
    }
    engine->difficulty = difficulty;
}

void AsteroidsEngine_SetShipType(AsteroidsEngine_t* engine, SpaceShipType_t type)
{
    if (type >= SHIP_COUNT) {
        type = SHIP_RAIL;
    }
    engine->ship_type = (uint8_t)type;
}

SpaceShipType_t AsteroidsEngine_GetShipType(AsteroidsEngine_t* engine)
{
    if (engine->ship_type >= SHIP_COUNT) {
        return SHIP_RAIL;
    }
    return (SpaceShipType_t)engine->ship_type;
}

uint8_t AsteroidsEngine_Update(AsteroidsEngine_t* engine, UserInput input, uint8_t fire_pressed, uint8_t hyperspace_pressed)
{
    uint8_t events = AST_EVENT_NONE;

    if (engine->game_over) {
        return AST_EVENT_NONE;
    }

    engine->frame_counter++;
    engine->survival_frames++;
    update_ship(engine, input, hyperspace_pressed);

    if (engine->fire_cooldown > 0) {
        engine->fire_cooldown--;
    }
    if (engine->weapon_timer > 0U) {
        engine->weapon_timer--;
    }
    if (engine->magnet_timer > 0U) {
        engine->magnet_timer--;
    }

    if (fire_pressed && engine->fire_cooldown == 0) {
        if (spawn_bullet(engine)) {
            events |= AST_EVENT_FIRE;
            engine->fire_cooldown = AST_FIRE_COOLDOWN_FRAMES;
        }
    }

    update_bullets(engine);
    update_rocks(engine);
    update_powerups(engine);
    update_particles(engine);
    update_bursts(engine);

    events |= handle_collisions(engine);

    if (engine->invuln_frames > 0) {
        engine->invuln_frames--;
    }

    if (count_active_rocks(engine) == 0 && !engine->game_over) {
        engine->wave++;
        if (engine->wave > engine->max_wave_reached) {
            engine->max_wave_reached = engine->wave;
        }
        spawn_wave(engine);
        events |= AST_EVENT_WAVE_CLEAR;
    }

    return events;
}

static Vec2 ship_vertex(float cx, float cy, float ang_deg, float length)
{
    Vec2 p;
    float r = deg_to_rad(ang_deg);
    p.x = cx + (cosf(r) * length);
    p.y = cy + (sinf(r) * length);
    return p;
}

static float wrapped_delta(float object_pos, float camera_pos, float world_size)
{
    float delta = object_pos - camera_pos;
    float half = world_size * 0.5f;

    if (delta > half) {
        delta -= world_size;
    } else if (delta < -half) {
        delta += world_size;
    }

    return delta;
}

static Vec2 world_to_screen(AsteroidsEngine_t* engine, float world_x, float world_y)
{
    Vec2 p;
    p.x = AST_SCREEN_CENTER_X + wrapped_delta(world_x, engine->ship_x, AST_SCREEN_W);
    p.y = AST_SCREEN_CENTER_Y + wrapped_delta(world_y, engine->ship_y, AST_SCREEN_H);
    return p;
}

static Vec2 world_to_screen_no_wrap(AsteroidsEngine_t* engine, float world_x, float world_y)
{
    Vec2 p;
    p.x = AST_SCREEN_CENTER_X + (world_x - engine->ship_x);
    p.y = AST_SCREEN_CENTER_Y + (world_y - engine->ship_y);
    return p;
}

static uint8_t point_on_screen(Vec2 p, float margin)
{
    return (p.x >= -margin && p.x < (AST_SCREEN_W + margin) &&
            p.y >= -margin && p.y < (AST_SCREEN_H + margin));
}

static uint8_t ship_sprite_index(float angle_deg)
{
    int16_t sector;

    while (angle_deg < 0.0f) {
        angle_deg += 360.0f;
    }
    while (angle_deg >= 360.0f) {
        angle_deg -= 360.0f;
    }

    sector = (int16_t)((angle_deg + 22.5f) / 45.0f);
    return (uint8_t)(sector & 7);
}

static uint8_t vector_sprite_index(float vx, float vy)
{
    float angle_deg = atan2f(vy, vx) * (180.0f / 3.14159265f);
    return ship_sprite_index(angle_deg);
}

static void draw_sprite_centered(Vec2 center, uint16_t width, uint16_t height, const uint8_t* sprite)
{
    int16_t x = (int16_t)(center.x - ((float)width * 0.5f));
    int16_t y = (int16_t)(center.y - ((float)height * 0.5f));
    int16_t sy;

    for (sy = 0; sy < (int16_t)height; sy++) {
        int16_t sx;
        int16_t screen_y = y + sy;

        if (screen_y < 0 || screen_y >= (int16_t)AST_SCREEN_H) {
            continue;
        }
        for (sx = 0; sx < (int16_t)width; sx++) {
            int16_t screen_x = x + sx;
            uint8_t pixel;

            if (screen_x < 0 || screen_x >= (int16_t)AST_SCREEN_W) {
                continue;
            }

            pixel = sprite[(sy * width) + sx];
            if (pixel != 255U) {
                LCD_Set_Pixel((uint16_t)screen_x, (uint16_t)screen_y, pixel);
            }
        }
    }
}

static void draw_rotated_sprite_centered(Vec2 center, uint16_t width, uint16_t height,
                                         const uint8_t* sprite, float angle_deg, float base_angle_deg)
{
    int16_t half_w = (int16_t)(width / 2U);
    int16_t half_h = (int16_t)(height / 2U);
    int16_t origin_x = (int16_t)(center.x - (float)half_w);
    int16_t origin_y = (int16_t)(center.y - (float)half_h);
    float delta = deg_to_rad(angle_deg - base_angle_deg);
    float c = cosf(delta);
    float s = sinf(delta);
    int16_t dy;

    for (dy = 0; dy < (int16_t)height; dy++) {
        int16_t dx;
        int16_t target_y = origin_y + dy;
        float rel_y = (float)dy - (float)half_h;

        if (target_y < 0 || target_y >= (int16_t)AST_SCREEN_H) {
            continue;
        }

        for (dx = 0; dx < (int16_t)width; dx++) {
            int16_t target_x = origin_x + dx;
            float rel_x = (float)dx - (float)half_w;
            int16_t src_x;
            int16_t src_y;
            uint8_t pixel;

            if (target_x < 0 || target_x >= (int16_t)AST_SCREEN_W) {
                continue;
            }

            src_x = (int16_t)((rel_x * c) + (rel_y * s) + (float)half_w);
            src_y = (int16_t)((-rel_x * s) + (rel_y * c) + (float)half_h);

            if (src_x < 0 || src_x >= (int16_t)width || src_y < 0 || src_y >= (int16_t)height) {
                continue;
            }

            pixel = sprite[(src_y * width) + src_x];
            if (pixel != 255U) {
                LCD_Set_Pixel((uint16_t)target_x, (uint16_t)target_y, pixel);
            }
        }
    }
}

void AsteroidsEngine_Draw(AsteroidsEngine_t* engine)
{
    uint8_t i;


    for (i = 0; i < ASTEROIDS_STAR_COUNT; i++) {
        AsteroidStar_t* s = &engine->stars[i];
        uint8_t colour = s->colour;

        if (((engine->frame_counter + i) % 24u) == 0u) {
            colour = 14;
        }

        LCD_Set_Pixel((uint16_t)wrapf((float)s->x - (engine->ship_x * 0.35f) + AST_SCREEN_CENTER_X, AST_SCREEN_W),
                      (uint16_t)wrapf((float)s->y - (engine->ship_y * 0.35f) + AST_SCREEN_CENTER_Y, AST_SCREEN_H),
                      colour);
    }

    for (i = 0; i < ASTEROIDS_MAX_ROCKS; i++) {
        AsteroidRock_t* r = &engine->rocks[i];
        if (!r->active) {
            continue;
        }

        Vec2 screen_pos = world_to_screen(engine, r->x, r->y);
        if (point_on_screen(screen_pos, (float)r->radius)) {
            if (r->radius >= 10) {
                draw_sprite_centered(screen_pos, AST_ROCK_LARGE_W, AST_ROCK_LARGE_H, AST_ROCK_LARGE);
            } else if (r->radius >= 7) {
                draw_sprite_centered(screen_pos, AST_ROCK_MEDIUM_W, AST_ROCK_MEDIUM_H, AST_ROCK_MEDIUM);
            } else {
                draw_sprite_centered(screen_pos, AST_ROCK_SMALL_W, AST_ROCK_SMALL_H, AST_ROCK_SMALL);
            }
        }
    }

    for (i = 0; i < ASTEROIDS_MAX_BULLETS; i++) {
        AsteroidBullet_t* b = &engine->bullets[i];
        if (!b->active) {
            continue;
        }

        Vec2 screen_pos = world_to_screen_no_wrap(engine, b->x, b->y);
        if (point_on_screen(screen_pos, 8.0f)) {
            uint8_t sprite_index = vector_sprite_index(b->vx, b->vy);
            draw_sprite_centered(screen_pos, AST_BULLET_SPRITE_W, AST_BULLET_SPRITE_H, AST_BULLET_SPRITES[sprite_index]);
        }
    }

    for (i = 0; i < ASTEROIDS_MAX_POWERUPS; i++) {
        AsteroidPowerup_t* powerup = &engine->powerups[i];
        if (powerup->active) {
            Vec2 screen_pos = world_to_screen(engine, powerup->x, powerup->y);
            uint8_t type = powerup->type;

            if (type >= AST_POWERUP_SPRITE_COUNT) {
                type = 0U;
            }
            if (powerup->ttl < 120U && ((powerup->ttl / 10U) & 1U) != 0U) {
                continue;
            }
            if (point_on_screen(screen_pos, 10.0f)) {
                draw_sprite_centered(screen_pos, AST_POWERUP_SPRITE_W, AST_POWERUP_SPRITE_H, AST_POWERUP_SPRITES[type]);
            }
        }
    }

    for (i = 0; i < ASTEROIDS_MAX_BURSTS; i++) {
        AsteroidBurst_t* burst = &engine->bursts[i];
        if (burst->active) {
            Vec2 screen_pos = world_to_screen(engine, burst->x, burst->y);
            if (point_on_screen(screen_pos, 18.0f)) {
                uint8_t frame = burst->frame;
                if (frame >= AST_EXPLOSION_SPRITE_COUNT) {
                    frame = AST_EXPLOSION_SPRITE_COUNT - 1U;
                }
                draw_sprite_centered(screen_pos, AST_EXPLOSION_SPRITE_W, AST_EXPLOSION_SPRITE_H, AST_EXPLOSION_SPRITES[frame]);
                if (frame < 3U && screen_pos.x >= 0.0f && screen_pos.x < AST_SCREEN_W &&
                    screen_pos.y >= 0.0f && screen_pos.y < AST_SCREEN_H) {
                    LCD_Draw_Circle((uint16_t)screen_pos.x, (uint16_t)screen_pos.y,
                                    (uint16_t)(8U + (frame * 4U)), (frame == 0U) ? 14U : 6U, 0);
                }
            }
        }
    }

    if ((engine->invuln_frames == 0) || ((engine->invuln_frames / 6) % 2 == 0)) {
        float ship_screen_angle = compass_to_screen_angle(engine->ship_angle_deg);
        Vec2 rear = ship_vertex(AST_SCREEN_CENTER_X, AST_SCREEN_CENTER_Y, ship_screen_angle + 180.0f, 6.0f);

        draw_rotated_sprite_centered((Vec2){AST_SCREEN_CENTER_X, AST_SCREEN_CENTER_Y},
                                     STRIKER_PLAYER_SHIP_W,
                                     STRIKER_PLAYER_SHIP_H,
                                     STRIKER_PLAYER_SHIPS[engine->ship_type],
                                     ship_screen_angle,
                                     270.0f);

        if (engine->magnet_timer > 0U && ((engine->frame_counter / 8U) & 1U) == 0U) {
            LCD_Draw_Circle((uint16_t)AST_SCREEN_CENTER_X, (uint16_t)AST_SCREEN_CENTER_Y, 38, 11, 0);
        }

        if (engine->shield_active) {
            uint8_t shield_colour = ((engine->frame_counter / 6U) & 1U) ? 14U : 1U;
            LCD_Draw_Circle((uint16_t)AST_SCREEN_CENTER_X, (uint16_t)AST_SCREEN_CENTER_Y, 27, shield_colour, 0);
            LCD_Draw_Circle((uint16_t)AST_SCREEN_CENTER_X, (uint16_t)AST_SCREEN_CENTER_Y, 28, 14, 0);
        }

        if (engine->thrust_active) {
            float flame_len = ((engine->frame_counter & 1u) == 0u) ? 12.0f : 9.0f;
            Vec2 flame_tip = ship_vertex(AST_SCREEN_CENTER_X, AST_SCREEN_CENTER_Y, ship_screen_angle + 180.0f, flame_len);
            Vec2 flame_left = ship_vertex(rear.x, rear.y, ship_screen_angle + 110.0f, 3.0f);
            Vec2 flame_right = ship_vertex(rear.x, rear.y, ship_screen_angle - 110.0f, 3.0f);
            Vec2 flame_core = ship_vertex(AST_SCREEN_CENTER_X, AST_SCREEN_CENTER_Y, ship_screen_angle + 180.0f, flame_len - 4.0f);

            LCD_Draw_Line((uint16_t)flame_left.x, (uint16_t)flame_left.y, (uint16_t)flame_tip.x, (uint16_t)flame_tip.y, 5);
            LCD_Draw_Line((uint16_t)flame_right.x, (uint16_t)flame_right.y, (uint16_t)flame_tip.x, (uint16_t)flame_tip.y, 6);
            LCD_Draw_Line((uint16_t)rear.x, (uint16_t)rear.y, (uint16_t)flame_core.x, (uint16_t)flame_core.y, 14);
            LCD_Set_Pixel((uint16_t)flame_tip.x, (uint16_t)flame_tip.y, 1);
        }
    }

    for (i = 0; i < ASTEROIDS_MAX_PARTICLES; i++) {
        AsteroidParticle_t* p = &engine->particles[i];
        uint8_t colour;

        if (!p->active) {
            continue;
        }

        colour = (p->ttl > 6) ? p->colour : 1;
        Vec2 screen_pos = world_to_screen(engine, p->x, p->y);
        if (point_on_screen(screen_pos, 2.0f)) {
            LCD_Draw_Rect((uint16_t)screen_pos.x, (uint16_t)screen_pos.y, 2, 2, colour, 1);
        }
    }
}

uint8_t AsteroidsEngine_IsGameOver(AsteroidsEngine_t* engine)
{
    return engine->game_over;
}

uint16_t AsteroidsEngine_GetScore(AsteroidsEngine_t* engine)
{
    return engine->score;
}

uint8_t AsteroidsEngine_GetLives(AsteroidsEngine_t* engine)
{
    return engine->lives;
}

uint8_t AsteroidsEngine_GetWave(AsteroidsEngine_t* engine)
{
    return engine->wave;
}

uint8_t AsteroidsEngine_HasShield(AsteroidsEngine_t* engine)
{
    return engine->shield_active;
}

uint16_t AsteroidsEngine_GetWeaponTimer(AsteroidsEngine_t* engine)
{
    return engine->weapon_timer;
}

uint16_t AsteroidsEngine_GetMagnetTimer(AsteroidsEngine_t* engine)
{
    return engine->magnet_timer;
}

AsteroidsDifficulty_t AsteroidsEngine_GetDifficulty(AsteroidsEngine_t* engine)
{
    return engine->difficulty;
}

uint16_t AsteroidsEngine_GetSurvivalSeconds(AsteroidsEngine_t* engine)
{
    return (uint16_t)(engine->survival_frames / 60U);
}

uint16_t AsteroidsEngine_GetRocksDestroyed(AsteroidsEngine_t* engine)
{
    return engine->rocks_destroyed;
}

uint8_t AsteroidsEngine_GetMaxWaveReached(AsteroidsEngine_t* engine)
{
    return engine->max_wave_reached;
}
