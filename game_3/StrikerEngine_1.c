#include "StrikerEngine.h"

#include <math.h>
#include "AsteroidsAssets.h"
#include "LCD.h"
#include "Utils.h"

#define STRIKER_SCREEN_W 240.0f
#define STRIKER_SCREEN_H 240.0f
#define STRIKER_MAX_HP 100U
#define STRIKER_HEAL_AMOUNT 20U
#define STRIKER_KILL_HEAL_LIGHT 3U
#define STRIKER_KILL_HEAL_HEAVY 7U
#define STRIKER_BULLET_DAMAGE 15U
#define STRIKER_COLLISION_DAMAGE 25U
#define STRIKER_PLAYER_Y 194.0f
#define STRIKER_PLAYER_SPEED 4.2f
#define STRIKER_BULLET_SPEED 7.5f
#define STRIKER_ENEMY_BULLET_SPEED 2.7f
#define STRIKER_FIRE_COOLDOWN 7U
#define STRIKER_DOUBLE_SHOT_FRAMES 600U
#define STRIKER_MAGNET_FRAMES 600U
#define STRIKER_POWERUP_TTL 620U
#define STRIKER_POWERUP_FALL_SPEED 0.78f
#define STRIKER_POWERUP_DRIFT_SPEED 0.16f
#define STRIKER_POWERUP_MAX_SPEED 2.4f
#define STRIKER_MAGNET_RADIUS 72.0f
#define STRIKER_BOSS_INTERVAL_FRAMES 3600U
#define STRIKER_BOSS_ENTRY_Y 42.0f
#define STRIKER_BOSS_BASE_HP 80U
#define STRIKER_BOSS_HP_STEP 35U
#define STRIKER_BOSS_LASER_DAMAGE 28U
#define STRIKER_BOSS_SCORE 500U
#define STRIKER_MISSION_FRAMES 1800U
#define STRIKER_FINAL_BOSS_PROGRESS 55U
#define STRIKER_CORE_DAMAGE_BULLET 8U
#define STRIKER_CORE_DAMAGE_COLLISION 14U
#define STRIKER_CORE_DAMAGE_LASER 20U

#define STRIKER_POWERUP_HEALTH 0U
#define STRIKER_POWERUP_DOUBLE_SHOT 1U
#define STRIKER_POWERUP_SHIELD 2U
#define STRIKER_POWERUP_MAGNET 3U

#define STRIKER_ENEMY_BULLET_NORMAL 0U
#define STRIKER_ENEMY_BULLET_SCATTER 1U
#define STRIKER_ENEMY_BULLET_HOMING 2U

static float deg_to_rad(float deg)
{
    return deg * (3.14159265f / 180.0f);
}

static float clampf(float v, float lo, float hi)
{
    if (v < lo) {
        return lo;
    }
    if (v > hi) {
        return hi;
    }
    return v;
}

static uint8_t collide_circle(float x1, float y1, float r1, float x2, float y2, float r2)
{
    float dx = x1 - x2;
    float dy = y1 - y2;
    float rr = r1 + r2;
    return ((dx * dx) + (dy * dy)) <= (rr * rr);
}

static void sync_lives_from_hp(StrikerEngine_t* engine)
{
    if (engine->hp == 0U) {
        engine->lives = 0U;
    } else if (engine->hp <= 34U) {
        engine->lives = 1U;
    } else if (engine->hp <= 67U) {
        engine->lives = 2U;
    } else {
        engine->lives = 3U;
    }
}

static void heal_player(StrikerEngine_t* engine, uint8_t amount)
{
    uint16_t hp = (uint16_t)engine->hp + amount;

    if (hp > engine->max_hp) {
        hp = engine->max_hp;
    }
    engine->hp = (uint8_t)hp;
    sync_lives_from_hp(engine);
}

static void damage_player(StrikerEngine_t* engine, uint8_t amount)
{
    if (amount >= engine->hp) {
        engine->hp = 0U;
        engine->game_over = 1U;
    } else {
        engine->hp = (uint8_t)(engine->hp - amount);
    }
    sync_lives_from_hp(engine);
}

static void damage_signal_core(StrikerEngine_t* engine, uint8_t amount)
{
    if (amount >= engine->signal_core) {
        engine->signal_core = 0U;
        engine->game_over = 1U;
    } else {
        engine->signal_core = (uint8_t)(engine->signal_core - amount);
    }
}

static void draw_sprite_clipped(int16_t x0, int16_t y0, uint16_t width, uint16_t height, const uint8_t* sprite)
{
    int16_t y;

    for (y = 0; y < (int16_t)height; y++) {
        int16_t x;
        int16_t screen_y = y0 + y;

        if (screen_y < 0 || screen_y >= (int16_t)STRIKER_SCREEN_H) {
            continue;
        }
        for (x = 0; x < (int16_t)width; x++) {
            int16_t screen_x = x0 + x;
            uint8_t pixel;

            if (screen_x < 0 || screen_x >= (int16_t)STRIKER_SCREEN_W) {
                continue;
            }
            pixel = sprite[(y * width) + x];
            if (pixel != 255U) {
                LCD_Set_Pixel((uint16_t)screen_x, (uint16_t)screen_y, pixel);
            }
        }
    }
}

static void draw_sprite_clipped_flip_y(int16_t x0, int16_t y0, uint16_t width, uint16_t height, const uint8_t* sprite)
{
    int16_t y;

    for (y = 0; y < (int16_t)height; y++) {
        int16_t x;
        int16_t screen_y = y0 + y;
        int16_t src_y = (int16_t)height - 1 - y;

        if (screen_y < 0 || screen_y >= (int16_t)STRIKER_SCREEN_H) {
            continue;
        }
        for (x = 0; x < (int16_t)width; x++) {
            int16_t screen_x = x0 + x;
            uint8_t pixel;

            if (screen_x < 0 || screen_x >= (int16_t)STRIKER_SCREEN_W) {
                continue;
            }
            pixel = sprite[(src_y * width) + x];
            if (pixel != 255U) {
                LCD_Set_Pixel((uint16_t)screen_x, (uint16_t)screen_y, pixel);
            }
        }
    }
}

static void clear_entities(StrikerEngine_t* engine)
{
    uint8_t i;

    for (i = 0; i < STRIKER_MAX_BULLETS; i++) {
        engine->bullets[i].active = 0;
    }
    for (i = 0; i < STRIKER_MAX_ENEMY_BULLETS; i++) {
        engine->enemy_bullets[i].active = 0;
    }
    for (i = 0; i < STRIKER_MAX_ENEMIES; i++) {
        engine->enemies[i].active = 0;
    }
    for (i = 0; i < STRIKER_MAX_POWERUPS; i++) {
        engine->powerups[i].active = 0;
    }
    for (i = 0; i < STRIKER_MAX_BURSTS; i++) {
        engine->bursts[i].active = 0;
    }
}

static void init_stars(StrikerEngine_t* engine)
{
    uint8_t i;

    for (i = 0; i < STRIKER_STAR_COUNT; i++) {
        engine->stars[i].x = (uint8_t)Random_U16(240);
        engine->stars[i].y = (uint8_t)Random_U16(240);
        engine->stars[i].speed = (uint8_t)(1U + Random_U16(3));
        engine->stars[i].colour = (engine->stars[i].speed >= 3U) ? 14U : ((i % 3U) ? 13U : 1U);
    }
}

static uint8_t spawn_bullet_at(StrikerEngine_t* engine, float x, float vx, float vy,
                               uint8_t damage, uint8_t pierce_remaining, uint8_t type)
{
    uint8_t i;

    for (i = 0; i < STRIKER_MAX_BULLETS; i++) {
        StrikerBullet_t* b = &engine->bullets[i];
        if (!b->active) {
            b->x = x;
            b->y = engine->player_y - 20.0f;
            b->vx = vx;
            b->vy = vy;
            b->damage = damage;
            b->pierce_remaining = pierce_remaining;
            b->type = type;
            b->active = 1;
            return 1;
        }
    }
    return 0;
}

static uint8_t spawn_bullet(StrikerEngine_t* engine)
{
    if (engine->ship_type == SHIP_SPREAD) {
        uint8_t fired = 0U;
        fired |= spawn_bullet_at(engine, engine->player_x, 0.0f, -6.6f, 1U, 0U, STRIKER_BULLET_SPREAD);
        fired |= spawn_bullet_at(engine, engine->player_x - 8.0f, -1.35f, -6.2f, 1U, 0U, STRIKER_BULLET_SPREAD);
        fired |= spawn_bullet_at(engine, engine->player_x + 8.0f, 1.35f, -6.2f, 1U, 0U, STRIKER_BULLET_SPREAD);
        if (engine->weapon_timer > 0U) {
            fired |= spawn_bullet_at(engine, engine->player_x - 15.0f, -2.15f, -5.8f, 1U, 0U, STRIKER_BULLET_SPREAD);
            fired |= spawn_bullet_at(engine, engine->player_x + 15.0f, 2.15f, -5.8f, 1U, 0U, STRIKER_BULLET_SPREAD);
        }
        return fired;
    }

    if (engine->ship_type == SHIP_LANCE) {
        uint8_t pierce = (engine->weapon_timer > 0U) ? 4U : 3U;
        return spawn_bullet_at(engine, engine->player_x, 0.0f, -7.0f, 1U, pierce, STRIKER_BULLET_PIERCE);
    }

    if (engine->weapon_timer > 0U) {
        return (uint8_t)(spawn_bullet_at(engine, engine->player_x - 5.0f, 0.0f, -8.3f, 2U, 0U, STRIKER_BULLET_LINEAR) |
                         spawn_bullet_at(engine, engine->player_x + 5.0f, 0.0f, -8.3f, 2U, 0U, STRIKER_BULLET_LINEAR));
    }

    return spawn_bullet_at(engine, engine->player_x, 0.0f, -8.6f, 3U, 0U, STRIKER_BULLET_LINEAR);
}

static void spawn_enemy_bullet_typed(StrikerEngine_t* engine, float x, float y, float vx, float vy, uint8_t type)
{
    uint8_t i;

    for (i = 0; i < STRIKER_MAX_ENEMY_BULLETS; i++) {
        StrikerEnemyBullet_t* b = &engine->enemy_bullets[i];
        if (!b->active) {
            b->x = x;
            b->y = y + 12.0f;
            b->vx = vx;
            b->vy = vy;
            b->type = type;
            b->active = 1U;
            return;
        }
    }
}

static void spawn_enemy_bullet(StrikerEngine_t* engine, float x, float y)
{
    float aim = clampf((engine->player_x - x) * 0.014f, -1.15f, 1.15f);
    spawn_enemy_bullet_typed(engine,
                             x,
                             y,
                             aim,
                             STRIKER_ENEMY_BULLET_SPEED + ((float)engine->wave * 0.06f),
                             STRIKER_ENEMY_BULLET_NORMAL);
}

static void spawn_enemy_bullet_vector(StrikerEngine_t* engine, float x, float y, float vx, float vy)
{
    spawn_enemy_bullet_typed(engine, x, y, vx, vy, STRIKER_ENEMY_BULLET_NORMAL);
}

static void fire_enemy_pattern(StrikerEngine_t* engine, StrikerEnemy_t* enemy)
{
    float speed = STRIKER_ENEMY_BULLET_SPEED + ((float)engine->wave * 0.07f);
    float aim = clampf((engine->player_x - enemy->x) * 0.015f, -1.25f, 1.25f);

    if (enemy->type == 0U) {
        spawn_enemy_bullet_vector(engine, enemy->x - 5.0f, enemy->y, aim - 0.55f, speed);
        spawn_enemy_bullet_vector(engine, enemy->x + 5.0f, enemy->y, aim + 0.55f, speed);
        if (engine->wave >= 4U) {
            spawn_enemy_bullet_vector(engine, enemy->x, enemy->y, aim, speed + 0.25f);
        }
    } else {
        spawn_enemy_bullet_vector(engine, enemy->x, enemy->y, aim - 1.35f, speed * 0.94f);
        spawn_enemy_bullet_vector(engine, enemy->x, enemy->y, aim - 0.68f, speed);
        spawn_enemy_bullet_vector(engine, enemy->x, enemy->y, aim, speed + 0.18f);
        spawn_enemy_bullet_vector(engine, enemy->x, enemy->y, aim + 0.68f, speed);
        spawn_enemy_bullet_vector(engine, enemy->x, enemy->y, aim + 1.35f, speed * 0.94f);
        if (engine->wave >= 3U) {
            spawn_enemy_bullet(engine, enemy->x, enemy->y);
        }
    }
}

static void spawn_burst(StrikerEngine_t* engine, float x, float y)
{
    uint8_t i;

    for (i = 0; i < STRIKER_MAX_BURSTS; i++) {
        StrikerBurst_t* burst = &engine->bursts[i];
        if (!burst->active) {
            burst->x = x;
            burst->y = y;
            burst->tick = 0U;
            burst->frame = 0U;
            burst->active = 1U;
            return;
        }
    }
}

static void spawn_enemy(StrikerEngine_t* engine)
{
    uint8_t i;
    uint8_t type = (Random_U16(100) < (uint16_t)(22U + engine->wave)) ? 1U : 0U;

    for (i = 0; i < STRIKER_MAX_ENEMIES; i++) {
        StrikerEnemy_t* e = &engine->enemies[i];
        if (!e->active) {
            e->x = 18.0f + (float)Random_U16(204);
            e->y = -16.0f;
            e->vx = ((float)((int16_t)Random_U16(80) - 40) / 100.0f);
            e->vy = (type == 0U) ? (1.25f + ((float)engine->wave * 0.08f)) : (0.85f + ((float)engine->wave * 0.06f));
            e->fire_timer = (uint16_t)((type == 0U) ? (16U + Random_U16(14)) : (8U + Random_U16(10)));
            e->hp = (type == 0U) ? 1U : 3U;
            e->type = type;
            e->active = 1;
            return;
        }
    }
}

static void spawn_powerup(StrikerEngine_t* engine, float x, float y, uint8_t source_type)
{
    uint8_t i;
    uint8_t roll = (uint8_t)Random_U16(100);
    uint8_t type_roll;
    uint8_t type;
    uint8_t drop_chance = (source_type == 0U) ? 30U : 52U;
    uint8_t heal_chance = (source_type == 0U) ? 14U : 28U;

    if (roll >= drop_chance) {
        return;
    }
    type_roll = (uint8_t)Random_U16(100);
    if (type_roll < heal_chance) {
        type = STRIKER_POWERUP_HEALTH;
    } else if (type_roll < (uint8_t)(heal_chance + 28U)) {
        type = STRIKER_POWERUP_DOUBLE_SHOT;
    } else if (type_roll < (uint8_t)(heal_chance + 52U)) {
        type = STRIKER_POWERUP_SHIELD;
    } else {
        type = STRIKER_POWERUP_MAGNET;
    }

    for (i = 0; i < STRIKER_MAX_POWERUPS; i++) {
        StrikerPowerup_t* p = &engine->powerups[i];
        if (!p->active) {
            p->x = x;
            p->y = y;
            p->vx = ((float)((int16_t)Random_U16(40) - 20) / 100.0f);
            p->vy = 0.38f;
            p->ttl = STRIKER_POWERUP_TTL;
            p->type = type;
            p->active = 1;
            return;
        }
    }
}

static void spawn_forced_powerup(StrikerEngine_t* engine, float x, float y, uint8_t type)
{
    uint8_t i;

    for (i = 0; i < STRIKER_MAX_POWERUPS; i++) {
        StrikerPowerup_t* p = &engine->powerups[i];
        if (!p->active) {
            p->x = x;
            p->y = y;
            p->vx = ((float)((int16_t)Random_U16(44) - 22) / 100.0f);
            p->vy = 0.36f;
            p->ttl = STRIKER_POWERUP_TTL;
            p->type = type;
            p->active = 1U;
            return;
        }
    }
}

static void start_boss(StrikerEngine_t* engine)
{
    uint16_t hp = (uint16_t)(STRIKER_BOSS_BASE_HP + 90U + (engine->boss_kills * STRIKER_BOSS_HP_STEP));

    engine->boss_active = 1U;
    engine->boss_x = 120.0f;
    engine->boss_y = -34.0f;
    engine->boss_max_hp = hp;
    engine->boss_hp = hp;
    engine->boss_attack_timer = 55U;
    engine->boss_attack_phase = 0U;
    engine->laser_warning_timer = 0U;
    engine->laser_fire_timer = 0U;
    engine->laser_hit_done = 0U;
    engine->laser_x = engine->player_x;
}

static void update_mission(StrikerEngine_t* engine)
{
    uint32_t progress = (engine->survival_frames * 100U) / STRIKER_MISSION_FRAMES;

    if (progress > 100U) {
        progress = 100U;
    }
    if (engine->final_boss_spawned && engine->boss_active && progress >= 100U) {
        progress = 99U;
    }
    engine->escape_progress = (uint8_t)progress;

    if (!engine->final_boss_spawned && engine->escape_progress >= STRIKER_FINAL_BOSS_PROGRESS) {
        engine->final_boss_spawned = 1U;
        start_boss(engine);
    }
}

static void spawn_boss_rewards(StrikerEngine_t* engine)
{
    uint8_t boss_number = (uint8_t)(engine->boss_kills + 1U);

    spawn_forced_powerup(engine, engine->boss_x - 16.0f, engine->boss_y + 24.0f, STRIKER_POWERUP_HEALTH);
    spawn_powerup(engine, engine->boss_x + 8.0f, engine->boss_y + 24.0f, 1U);
    if ((boss_number % 2U) == 0U) {
        uint8_t type = (uint8_t)(1U + Random_U16(3));
        spawn_forced_powerup(engine, engine->boss_x + 28.0f, engine->boss_y + 20.0f, type);
    }
}

static void fire_boss_scatter(StrikerEngine_t* engine)
{
    static const float offsets[7] = {-2.4f, -1.6f, -0.8f, 0.0f, 0.8f, 1.6f, 2.4f};
    uint8_t enraged = (engine->boss_max_hp > 0U && engine->boss_hp < (engine->boss_max_hp / 3U));
    uint8_t count = (enraged || engine->boss_kills >= 2U) ? 7U : 5U;
    uint8_t start = (count == 7U) ? 0U : 1U;
    uint8_t i;

    for (i = 0; i < count; i++) {
        float vx = offsets[start + i];
        spawn_enemy_bullet_typed(engine,
                                 engine->boss_x,
                                 engine->boss_y + 16.0f,
                                 vx,
                                 2.6f + (fabsf(vx) * 0.10f),
                                 STRIKER_ENEMY_BULLET_SCATTER);
    }
}

static void fire_boss_aimed(StrikerEngine_t* engine)
{
    uint8_t i;
    uint8_t enraged = (engine->boss_max_hp > 0U && engine->boss_hp < (engine->boss_max_hp / 3U));
    uint8_t count = enraged ? 3U : ((engine->boss_kills >= 1U) ? 3U : 1U);
    float base_x = engine->boss_x;
    float base_y = engine->boss_y + 18.0f;

    for (i = 0; i < count; i++) {
        float offset = ((float)i - ((float)(count - 1U) * 0.5f)) * 12.0f;
        float dx = engine->player_x - (base_x + offset);
        float dy = engine->player_y - base_y;
        float len = sqrtf((dx * dx) + (dy * dy));
        float speed = 3.15f + ((float)engine->boss_kills * 0.08f) + (enraged ? 0.45f : 0.0f);

        if (len < 1.0f) {
            len = 1.0f;
        }
        spawn_enemy_bullet_typed(engine,
                                 base_x + offset,
                                 base_y,
                                 (dx / len) * speed,
                                 (dy / len) * speed,
                                 STRIKER_ENEMY_BULLET_HOMING);
    }
}

static uint8_t update_boss(StrikerEngine_t* engine)
{
    uint8_t events = STRIKER_EVENT_NONE;

    if (!engine->boss_active) {
        return events;
    }

    if (engine->boss_y < STRIKER_BOSS_ENTRY_Y) {
        engine->boss_y += 1.2f;
        if (engine->boss_y > STRIKER_BOSS_ENTRY_Y) {
            engine->boss_y = STRIKER_BOSS_ENTRY_Y;
        }
    } else {
        engine->boss_x += sinf((float)engine->frame_counter * 0.025f) * 0.65f;
        engine->boss_x = clampf(engine->boss_x, 74.0f, 166.0f);
    }

    if (engine->laser_warning_timer > 0U) {
        engine->laser_warning_timer--;
        if (engine->laser_warning_timer == 0U) {
            engine->laser_fire_timer = 22U;
            engine->laser_hit_done = 0U;
        }
    } else if (engine->laser_fire_timer > 0U) {
        engine->laser_fire_timer--;
        if (!engine->laser_hit_done && fabsf(engine->player_x - engine->laser_x) < 11.0f) {
            engine->laser_hit_done = 1U;
            if (engine->shield_active) {
                engine->shield_active = 0U;
                events |= STRIKER_EVENT_SHIELD_BLOCK;
            } else {
                damage_player(engine, STRIKER_BOSS_LASER_DAMAGE);
                damage_signal_core(engine, STRIKER_CORE_DAMAGE_LASER);
                events |= STRIKER_EVENT_PLAYER_HIT;
            }
        }
    }

    if (engine->boss_attack_timer > 0U) {
        engine->boss_attack_timer--;
    } else {
        uint8_t enraged = (engine->boss_max_hp > 0U && engine->boss_hp < (engine->boss_max_hp / 3U));
        uint16_t interval;

        if (engine->boss_attack_phase == 0U) {
            fire_boss_scatter(engine);
        } else if (engine->boss_attack_phase == 1U) {
            fire_boss_aimed(engine);
        } else {
            engine->laser_x = clampf(engine->player_x, 22.0f, 218.0f);
            engine->laser_warning_timer = enraged ? 28U : 34U;
        }
        engine->boss_attack_phase = (uint8_t)((engine->boss_attack_phase + 1U) % 3U);
        interval = (uint16_t)(64U - ((engine->boss_kills > 5U) ? 24U : (engine->boss_kills * 5U)));
        if (enraged && interval > 12U) {
            interval = (uint16_t)(interval - 12U);
        }
        if (interval < 34U) {
            interval = 34U;
        }
        engine->boss_attack_timer = interval;
    }

    return events;
}

static void update_stars(StrikerEngine_t* engine)
{
    uint8_t i;

    for (i = 0; i < STRIKER_STAR_COUNT; i++) {
        StrikerStar_t* s = &engine->stars[i];
        s->y = (uint8_t)(s->y + s->speed);
        if (s->y >= 240U) {
            s->y = 0U;
            s->x = (uint8_t)Random_U16(240);
        }
    }
}

static void update_player(StrikerEngine_t* engine, UserInput input)
{
    if (input.angle >= 0.0f && input.magnitude > 0.06f) {
        float r = deg_to_rad(input.angle);
        float speed = STRIKER_PLAYER_SPEED * input.magnitude;
        engine->player_x += sinf(r) * speed;
        engine->player_y -= cosf(r) * speed;
    }

    engine->player_x = clampf(engine->player_x, 24.0f, 216.0f);
    engine->player_y = clampf(engine->player_y, 145.0f, 204.0f);
}

static void update_bullets(StrikerEngine_t* engine)
{
    uint8_t i;

    for (i = 0; i < STRIKER_MAX_BULLETS; i++) {
        StrikerBullet_t* b = &engine->bullets[i];
        if (b->active) {
            b->x += b->vx;
            b->y += b->vy;
            if (b->y < -18.0f || b->x < -18.0f || b->x > 258.0f) {
                b->active = 0;
            }
        }
    }
}

static void update_enemy_bullets(StrikerEngine_t* engine)
{
    uint8_t i;

    for (i = 0; i < STRIKER_MAX_ENEMY_BULLETS; i++) {
        StrikerEnemyBullet_t* b = &engine->enemy_bullets[i];
        if (b->active) {
            b->x += b->vx;
            b->y += b->vy;
            if (b->x < -8.0f || b->x > 248.0f || b->y > 250.0f) {
                b->active = 0U;
            }
        }
    }
}

static void update_enemies(StrikerEngine_t* engine)
{
    uint8_t i;

    for (i = 0; i < STRIKER_MAX_ENEMIES; i++) {
        StrikerEnemy_t* e = &engine->enemies[i];
        if (e->active) {
            e->x += e->vx;
            e->y += e->vy;
            if (e->x < 14.0f || e->x > 226.0f) {
                e->vx = -e->vx;
            }
            if (e->y > 255.0f) {
                e->active = 0;
            }
            if (e->y > 20.0f && e->y < 176.0f &&
                (e->type != 0U || engine->wave >= 1U)) {
                if (e->fire_timer > 0U) {
                    e->fire_timer--;
                } else {
                    fire_enemy_pattern(engine, e);
                    if (e->type == 0U) {
                        e->fire_timer = (uint16_t)(28U - ((engine->wave > 6U) ? 10U : (engine->wave * 2U)) + Random_U16(12));
                    } else {
                        e->fire_timer = (uint16_t)(18U - ((engine->wave > 6U) ? 8U : engine->wave) + Random_U16(10));
                    }
                }
            }
        }
    }
}

static void update_bursts(StrikerEngine_t* engine)
{
    uint8_t i;

    for (i = 0; i < STRIKER_MAX_BURSTS; i++) {
        StrikerBurst_t* burst = &engine->bursts[i];
        if (!burst->active) {
            continue;
        }
        burst->tick++;
        if ((burst->tick % 3U) == 0U) {
            burst->frame++;
            if (burst->frame >= AST_EXPLOSION_SPRITE_COUNT) {
                burst->active = 0U;
            }
        }
    }
}

static void update_powerups(StrikerEngine_t* engine)
{
    uint8_t i;

    for (i = 0; i < STRIKER_MAX_POWERUPS; i++) {
        StrikerPowerup_t* p = &engine->powerups[i];
        uint8_t attracted = 0U;
        if (!p->active) {
            continue;
        }

        if (engine->magnet_timer > 0U) {
            float dx = engine->player_x - p->x;
            float dy = engine->player_y - p->y;
            float d2 = (dx * dx) + (dy * dy);
            if (d2 < (STRIKER_MAGNET_RADIUS * STRIKER_MAGNET_RADIUS) && d2 > 1.0f) {
                float d = sqrtf(d2);
                p->vx += (dx / d) * 0.28f;
                p->vy += (dy / d) * 0.28f;
                attracted = 1U;
            }
        }

        if (engine->magnet_timer == 0U || !attracted) {
            float drift = sinf(((float)engine->frame_counter + ((float)i * 23.0f)) * 0.045f) * STRIKER_POWERUP_DRIFT_SPEED;
            p->vx = (p->vx * 0.92f) + (drift * 0.08f);
            p->vy += (STRIKER_POWERUP_FALL_SPEED - p->vy) * 0.08f;
            p->vy = clampf(p->vy, 0.42f, 1.12f);
        } else {
            p->vx *= 0.95f;
            p->vy *= 0.96f;
            p->vy = clampf(p->vy, -STRIKER_POWERUP_MAX_SPEED, STRIKER_POWERUP_MAX_SPEED);
        }
        p->x += p->vx;
        p->y += p->vy;
        if (p->x < 12.0f) {
            p->x = 12.0f;
            p->vx = -p->vx * 0.5f;
        } else if (p->x > 228.0f) {
            p->x = 228.0f;
            p->vx = -p->vx * 0.5f;
        }

        if (p->ttl > 0U) {
            p->ttl--;
        }
        if (p->ttl == 0U || p->y > 250.0f) {
            p->active = 0;
        }
    }
}

static uint8_t handle_collisions(StrikerEngine_t* engine)
{
    uint8_t i;
    uint8_t j;
    uint8_t events = STRIKER_EVENT_NONE;

    for (i = 0; i < STRIKER_MAX_BULLETS; i++) {
        StrikerBullet_t* b = &engine->bullets[i];
        if (!b->active) {
            continue;
        }
        if (engine->boss_active && engine->boss_y > 0.0f &&
            b->x >= (engine->boss_x - 54.0f) && b->x <= (engine->boss_x + 54.0f) &&
            b->y >= (engine->boss_y - 24.0f) && b->y <= (engine->boss_y + 30.0f)) {
            if (b->damage >= engine->boss_hp) {
                engine->boss_hp = 0U;
            } else {
                engine->boss_hp = (uint16_t)(engine->boss_hp - b->damage);
            }
            if (b->type == STRIKER_BULLET_PIERCE && b->pierce_remaining > 0U) {
                b->pierce_remaining--;
                if (b->pierce_remaining == 0U) {
                    b->active = 0U;
                }
            } else {
                b->active = 0U;
            }
            events |= STRIKER_EVENT_ENEMY_HIT;
            if (engine->boss_hp == 0U) {
                engine->boss_active = 0U;
                engine->laser_warning_timer = 0U;
                engine->laser_fire_timer = 0U;
                engine->score = (uint16_t)(engine->score + STRIKER_BOSS_SCORE + (engine->boss_kills * 120U));
                spawn_burst(engine, engine->boss_x - 26.0f, engine->boss_y);
                spawn_burst(engine, engine->boss_x, engine->boss_y + 8.0f);
                spawn_burst(engine, engine->boss_x + 26.0f, engine->boss_y);
                spawn_boss_rewards(engine);
                engine->boss_kills++;
                engine->escape_progress = 100U;
                engine->mission_complete = 1U;
                engine->game_over = 1U;
            }
            continue;
        }
        for (j = 0; j < STRIKER_MAX_ENEMIES; j++) {
            StrikerEnemy_t* e = &engine->enemies[j];
            if (e->active && collide_circle(b->x, b->y, 2.0f, e->x, e->y, (e->type == 0U) ? 10.0f : 14.0f)) {
                if (b->damage >= e->hp) {
                    e->hp = 0U;
                } else {
                    e->hp = (uint8_t)(e->hp - b->damage);
                }
                if (e->hp == 0U) {
                    e->active = 0;
                    engine->enemy_kills++;
                    engine->score += (e->type == 0U) ? 15U : 40U;
                    heal_player(engine, (e->type == 0U) ? STRIKER_KILL_HEAL_LIGHT : STRIKER_KILL_HEAL_HEAVY);
                    spawn_burst(engine, e->x, e->y);
                    spawn_powerup(engine, e->x, e->y, e->type);
                }
                if (b->type == STRIKER_BULLET_PIERCE && b->pierce_remaining > 0U) {
                    b->pierce_remaining--;
                    if (b->pierce_remaining == 0U) {
                        b->active = 0U;
                    }
                } else {
                    b->active = 0;
                }
                events |= STRIKER_EVENT_ENEMY_HIT;
                break;
            }
        }
    }

    for (i = 0; i < STRIKER_MAX_POWERUPS; i++) {
        StrikerPowerup_t* p = &engine->powerups[i];
        if (p->active && collide_circle(engine->player_x, engine->player_y, 11.0f, p->x, p->y, 9.0f)) {
            if (p->type == STRIKER_POWERUP_HEALTH) {
                heal_player(engine, STRIKER_HEAL_AMOUNT);
            } else if (p->type == STRIKER_POWERUP_DOUBLE_SHOT) {
                engine->weapon_timer = STRIKER_DOUBLE_SHOT_FRAMES;
            } else if (p->type == STRIKER_POWERUP_SHIELD) {
                engine->shield_active = 1U;
            } else if (p->type == STRIKER_POWERUP_MAGNET) {
                engine->magnet_timer = STRIKER_MAGNET_FRAMES;
            }
            p->active = 0;
            events |= STRIKER_EVENT_POWERUP;
        }
    }

    for (i = 0; i < STRIKER_MAX_ENEMIES; i++) {
        StrikerEnemy_t* e = &engine->enemies[i];
        if (!e->active) {
            continue;
        }
        if (collide_circle(engine->player_x, engine->player_y, 12.0f, e->x, e->y, (e->type == 0U) ? 10.0f : 14.0f)) {
            e->active = 0;
            spawn_burst(engine, e->x, e->y);
            if (engine->shield_active) {
                engine->shield_active = 0U;
                events |= STRIKER_EVENT_SHIELD_BLOCK;
            } else {
                damage_player(engine, STRIKER_COLLISION_DAMAGE);
                damage_signal_core(engine, STRIKER_CORE_DAMAGE_COLLISION);
                events |= STRIKER_EVENT_PLAYER_HIT;
            }
        }
    }

    for (i = 0; i < STRIKER_MAX_ENEMY_BULLETS; i++) {
        StrikerEnemyBullet_t* b = &engine->enemy_bullets[i];
        if (!b->active) {
            continue;
        }
        if (collide_circle(engine->player_x, engine->player_y, 11.0f, b->x, b->y, 3.0f)) {
            b->active = 0U;
            spawn_burst(engine, b->x, b->y);
            if (engine->shield_active) {
                engine->shield_active = 0U;
                events |= STRIKER_EVENT_SHIELD_BLOCK;
            } else {
                damage_player(engine, STRIKER_BULLET_DAMAGE);
                damage_signal_core(engine, STRIKER_CORE_DAMAGE_BULLET);
                events |= STRIKER_EVENT_PLAYER_HIT;
            }
        }
    }

    return events;
}

void StrikerEngine_Init(StrikerEngine_t* engine)
{
    engine->ship_type = SHIP_RAIL;
    StrikerEngine_StartGame(engine);
}

void StrikerEngine_StartGame(StrikerEngine_t* engine)
{
    engine->player_x = 120.0f;
    engine->player_y = STRIKER_PLAYER_Y;
    engine->score = 0U;
    engine->max_hp = STRIKER_MAX_HP;
    engine->hp = engine->max_hp;
    engine->lives = 3U;
    engine->wave = 1U;
    engine->game_over = 0U;
    engine->fire_cooldown = 0U;
    engine->shield_active = 0U;
    if (engine->ship_type >= SHIP_COUNT) {
        engine->ship_type = SHIP_RAIL;
    }
    engine->weapon_timer = 0U;
    engine->magnet_timer = 0U;
    engine->spawn_timer = 20U;
    engine->frame_counter = 0U;
    engine->survival_frames = 0U;
    engine->enemy_kills = 0U;
    engine->boss_kills = 0U;
    engine->boss_active = 0U;
    engine->boss_x = 120.0f;
    engine->boss_y = -34.0f;
    engine->boss_hp = 0U;
    engine->boss_max_hp = 0U;
    engine->boss_timer = STRIKER_BOSS_INTERVAL_FRAMES;
    engine->boss_attack_timer = 0U;
    engine->boss_attack_phase = 0U;
    engine->laser_warning_timer = 0U;
    engine->laser_fire_timer = 0U;
    engine->laser_hit_done = 0U;
    engine->laser_x = 120.0f;
    engine->mission_complete = 0U;
    engine->signal_core = 100U;
    engine->escape_progress = 0U;
    engine->final_boss_spawned = 0U;
    clear_entities(engine);
    init_stars(engine);
}

void StrikerEngine_SetShipType(StrikerEngine_t* engine, SpaceShipType_t type)
{
    if (type >= SHIP_COUNT) {
        type = SHIP_RAIL;
    }
    engine->ship_type = (uint8_t)type;
}

SpaceShipType_t StrikerEngine_GetShipType(StrikerEngine_t* engine)
{
    if (engine->ship_type >= SHIP_COUNT) {
        return SHIP_RAIL;
    }
    return (SpaceShipType_t)engine->ship_type;
}

uint8_t StrikerEngine_Update(StrikerEngine_t* engine, UserInput input, uint8_t fire_pressed)
{
    uint8_t events = STRIKER_EVENT_NONE;
    (void)fire_pressed;

    if (engine->game_over) {
        return STRIKER_EVENT_NONE;
    }

    engine->frame_counter++;
    engine->survival_frames++;
    if ((engine->frame_counter % 60U) == 0U) {
        engine->score = (uint16_t)(engine->score + 3U);
    }
    if ((engine->frame_counter % 720U) == 0U && engine->wave < 9U) {
        engine->wave++;
        events |= STRIKER_EVENT_WAVE;
    }
    update_mission(engine);

    update_stars(engine);
    update_player(engine, input);

    if (engine->fire_cooldown > 0U) {
        engine->fire_cooldown--;
    }
    if (engine->weapon_timer > 0U) {
        engine->weapon_timer--;
    }
    if (engine->magnet_timer > 0U) {
        engine->magnet_timer--;
    }

    if (engine->fire_cooldown == 0U) {
        if (spawn_bullet(engine)) {
            if ((engine->frame_counter % 42U) == 0U) {
                events |= STRIKER_EVENT_FIRE;
            }
            engine->fire_cooldown = STRIKER_FIRE_COOLDOWN;
        }
    }

    events |= update_boss(engine);

    if (!engine->boss_active) {
        if (engine->spawn_timer > 0U) {
            engine->spawn_timer--;
        } else {
            spawn_enemy(engine);
            engine->spawn_timer = (uint16_t)((engine->wave < 6U) ? (42U - (engine->wave * 4U)) : 18U);
        }
    }

    update_bullets(engine);
    update_enemy_bullets(engine);
    update_enemies(engine);
    update_powerups(engine);
    update_bursts(engine);
    events |= handle_collisions(engine);

    return events;
}

static void draw_player_bullet(StrikerBullet_t* b)
{
    if (b->y < 0.0f || b->y > 239.0f) {
        return;
    }
    if (b->type == STRIKER_BULLET_PIERCE) {
        LCD_Draw_Line((uint16_t)b->x, (uint16_t)b->y, (uint16_t)b->x, (uint16_t)(b->y + 24.0f), 4);
        LCD_Draw_Line((uint16_t)(b->x + 1.0f), (uint16_t)b->y, (uint16_t)(b->x + 1.0f), (uint16_t)(b->y + 24.0f), 1);
    } else if (b->type == STRIKER_BULLET_LINEAR) {
        LCD_Draw_Line((uint16_t)b->x, (uint16_t)b->y, (uint16_t)b->x, (uint16_t)(b->y + 20.0f), 2);
        LCD_Draw_Line((uint16_t)(b->x + 1.0f), (uint16_t)b->y, (uint16_t)(b->x + 1.0f), (uint16_t)(b->y + 20.0f), 5);
    } else {
        draw_sprite_clipped((int16_t)(b->x - (STRIKER_PLAYER_LASER_W / 2U)),
                            (int16_t)b->y,
                            STRIKER_PLAYER_LASER_W,
                            STRIKER_PLAYER_LASER_H,
                            STRIKER_PLAYER_LASER);
    }
}

static void draw_enemy_bullet(StrikerEnemyBullet_t* b)
{
    if (b->y < 0.0f || b->y > 239.0f || b->x < 0.0f || b->x > 239.0f) {
        return;
    }
    if (b->type == STRIKER_ENEMY_BULLET_SCATTER) {
        draw_sprite_clipped((int16_t)(b->x - (STRIKER_BOSS_SCATTER_BULLET_W / 2U)),
                            (int16_t)(b->y - (STRIKER_BOSS_SCATTER_BULLET_H / 2U)),
                            STRIKER_BOSS_SCATTER_BULLET_W,
                            STRIKER_BOSS_SCATTER_BULLET_H,
                            STRIKER_BOSS_SCATTER_BULLET);
        return;
    }
    if (b->type == STRIKER_ENEMY_BULLET_HOMING) {
        draw_sprite_clipped((int16_t)(b->x - (STRIKER_BOSS_HOMING_BULLET_W / 2U)),
                            (int16_t)(b->y - (STRIKER_BOSS_HOMING_BULLET_H / 2U)),
                            STRIKER_BOSS_HOMING_BULLET_W,
                            STRIKER_BOSS_HOMING_BULLET_H,
                            STRIKER_BOSS_HOMING_BULLET);
        return;
    }
    draw_sprite_clipped((int16_t)(b->x - (STRIKER_ENEMY_BULLET_W / 2U)),
                        (int16_t)b->y,
                        STRIKER_ENEMY_BULLET_W,
                        STRIKER_ENEMY_BULLET_H,
                        STRIKER_ENEMY_BULLET);
}

static void draw_enemy(StrikerEnemy_t* e)
{
    if (e->y < -26.0f || e->y > 250.0f) {
        return;
    }

    if (e->type == 0U) {
        draw_sprite_clipped_flip_y((int16_t)(e->x - (STRIKER_ENEMY_LIGHT_W / 2U)),
                                   (int16_t)(e->y - (STRIKER_ENEMY_LIGHT_H / 2U)),
                                   STRIKER_ENEMY_LIGHT_W,
                                   STRIKER_ENEMY_LIGHT_H,
                                   STRIKER_ENEMY_LIGHT);
    } else {
        draw_sprite_clipped_flip_y((int16_t)(e->x - (STRIKER_ENEMY_HEAVY_W / 2U)),
                                   (int16_t)(e->y - (STRIKER_ENEMY_HEAVY_H / 2U)),
                                   STRIKER_ENEMY_HEAVY_W,
                                   STRIKER_ENEMY_HEAVY_H,
                                   STRIKER_ENEMY_HEAVY);
    }
}

static void draw_boss(StrikerEngine_t* engine)
{
    uint16_t fill_w;
    const uint8_t* sprite;

    if (!engine->boss_active) {
        return;
    }

    sprite = (engine->boss_hp < (engine->boss_max_hp / 2U)) ? STRIKER_BOSS_DAMAGED : STRIKER_BOSS;
    draw_sprite_clipped((int16_t)(engine->boss_x - (STRIKER_BOSS_W / 2U)),
                        (int16_t)(engine->boss_y - (STRIKER_BOSS_H / 2U)),
                        STRIKER_BOSS_W,
                        STRIKER_BOSS_H,
                        sprite);

    LCD_Draw_Rect(48, 28, 144, 8, 9, 0);
    if (engine->boss_max_hp > 0U) {
        fill_w = (uint16_t)(((uint32_t)engine->boss_hp * 140U) / engine->boss_max_hp);
        if (fill_w > 0U) {
            LCD_Draw_Rect(50, 30, fill_w, 4, (engine->boss_hp < (engine->boss_max_hp / 3U)) ? 2U : 5U, 1);
        }
    }
}

static void draw_boss_laser(StrikerEngine_t* engine)
{
    if (engine->laser_warning_timer > 0U) {
        uint8_t colour = ((engine->laser_warning_timer / 4U) & 1U) ? 2U : 13U;
        LCD_Draw_Line((uint16_t)engine->laser_x, 36, (uint16_t)engine->laser_x, 224, colour);
        return;
    }
    if (engine->laser_fire_timer > 0U) {
        uint16_t x = (uint16_t)engine->laser_x;
        LCD_Draw_Line((uint16_t)(x - 3U), 34, (uint16_t)(x - 3U), 226, 2);
        LCD_Draw_Line((uint16_t)(x + 3U), 34, (uint16_t)(x + 3U), 226, 2);
        LCD_Draw_Line((uint16_t)(x - 1U), 34, (uint16_t)(x - 1U), 226, 5);
        LCD_Draw_Line((uint16_t)(x + 1U), 34, (uint16_t)(x + 1U), 226, 5);
        LCD_Draw_Line(x, 34, x, 226, 1);
    }
}

void StrikerEngine_Draw(StrikerEngine_t* engine)
{
    uint8_t i;

    for (i = 0; i < STRIKER_STAR_COUNT; i++) {
        StrikerStar_t* s = &engine->stars[i];
        LCD_Set_Pixel(s->x, s->y, s->colour);
        if (s->speed >= 3U && s->y > 2U) {
            LCD_Set_Pixel(s->x, (uint16_t)(s->y - 2U), 9);
        }
    }

    for (i = 0; i < STRIKER_MAX_POWERUPS; i++) {
        StrikerPowerup_t* p = &engine->powerups[i];
        if (p->active && !(p->ttl < 90U && ((p->ttl / 8U) & 1U))) {
            uint8_t type = (p->type < AST_POWERUP_SPRITE_COUNT) ? p->type : 0U;
            draw_sprite_clipped((int16_t)(p->x - 9.0f), (int16_t)(p->y - 9.0f),
                                AST_POWERUP_SPRITE_W, AST_POWERUP_SPRITE_H, AST_POWERUP_SPRITES[type]);
        }
    }

    for (i = 0; i < STRIKER_MAX_ENEMIES; i++) {
        if (engine->enemies[i].active) {
            draw_enemy(&engine->enemies[i]);
        }
    }

    draw_boss(engine);

    for (i = 0; i < STRIKER_MAX_BULLETS; i++) {
        if (engine->bullets[i].active) {
            draw_player_bullet(&engine->bullets[i]);
        }
    }

    for (i = 0; i < STRIKER_MAX_ENEMY_BULLETS; i++) {
        if (engine->enemy_bullets[i].active) {
            draw_enemy_bullet(&engine->enemy_bullets[i]);
        }
    }

    draw_boss_laser(engine);

    for (i = 0; i < STRIKER_MAX_BURSTS; i++) {
        StrikerBurst_t* burst = &engine->bursts[i];
        if (burst->active) {
            uint8_t frame = burst->frame;
            if (frame >= AST_EXPLOSION_SPRITE_COUNT) {
                frame = AST_EXPLOSION_SPRITE_COUNT - 1U;
            }
            draw_sprite_clipped((int16_t)(burst->x - (AST_EXPLOSION_SPRITE_W / 2U)),
                                (int16_t)(burst->y - (AST_EXPLOSION_SPRITE_H / 2U)),
                                AST_EXPLOSION_SPRITE_W,
                                AST_EXPLOSION_SPRITE_H,
                                AST_EXPLOSION_SPRITES[frame]);
        }
    }

    draw_sprite_clipped((int16_t)(engine->player_x - (STRIKER_PLAYER_SHIP_W / 2U)),
                        (int16_t)(engine->player_y - (STRIKER_PLAYER_SHIP_H / 2U)),
                        STRIKER_PLAYER_SHIP_W,
                        STRIKER_PLAYER_SHIP_H,
                        STRIKER_PLAYER_SHIPS[engine->ship_type]);

    LCD_Draw_Line((uint16_t)(engine->player_x - 6.0f), (uint16_t)(engine->player_y + 20.0f),
                  (uint16_t)(engine->player_x - 2.0f), (uint16_t)(engine->player_y + 31.0f), 14);
    LCD_Draw_Line((uint16_t)(engine->player_x + 6.0f), (uint16_t)(engine->player_y + 20.0f),
                  (uint16_t)(engine->player_x + 2.0f), (uint16_t)(engine->player_y + 31.0f), 14);

    if (engine->magnet_timer > 0U && ((engine->frame_counter / 8U) & 1U) == 0U) {
        LCD_Draw_Circle((uint16_t)engine->player_x, (uint16_t)engine->player_y, 38, 11, 0);
    }
    if (engine->shield_active) {
        LCD_Draw_Circle((uint16_t)engine->player_x, (uint16_t)engine->player_y, 28, 14, 0);
        LCD_Draw_Circle((uint16_t)engine->player_x, (uint16_t)engine->player_y, 27, 1, 0);
    }
}

uint8_t StrikerEngine_IsGameOver(StrikerEngine_t* engine)
{
    return engine->game_over;
}

uint16_t StrikerEngine_GetScore(StrikerEngine_t* engine)
{
    return engine->score;
}

uint8_t StrikerEngine_GetHp(StrikerEngine_t* engine)
{
    return engine->hp;
}

uint8_t StrikerEngine_GetMaxHp(StrikerEngine_t* engine)
{
    return engine->max_hp;
}

uint8_t StrikerEngine_GetLives(StrikerEngine_t* engine)
{
    return engine->lives;
}

uint8_t StrikerEngine_GetWave(StrikerEngine_t* engine)
{
    return engine->wave;
}

uint8_t StrikerEngine_HasShield(StrikerEngine_t* engine)
{
    return engine->shield_active;
}

uint16_t StrikerEngine_GetWeaponTimer(StrikerEngine_t* engine)
{
    return engine->weapon_timer;
}

uint16_t StrikerEngine_GetMagnetTimer(StrikerEngine_t* engine)
{
    return engine->magnet_timer;
}

uint16_t StrikerEngine_GetSurvivalSeconds(StrikerEngine_t* engine)
{
    return (uint16_t)(engine->survival_frames / 60U);
}

uint16_t StrikerEngine_GetEnemyKills(StrikerEngine_t* engine)
{
    return engine->enemy_kills;
}

uint16_t StrikerEngine_GetBossKills(StrikerEngine_t* engine)
{
    return engine->boss_kills;
}

uint8_t StrikerEngine_GetSignalCore(StrikerEngine_t* engine)
{
    return engine->signal_core;
}

uint8_t StrikerEngine_GetEscapeProgress(StrikerEngine_t* engine)
{
    return engine->escape_progress;
}

uint8_t StrikerEngine_IsMissionComplete(StrikerEngine_t* engine)
{
    return engine->mission_complete;
}
