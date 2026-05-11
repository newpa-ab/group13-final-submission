#include "VersusEngine.h"

#include <math.h>
#include "AsteroidsAssets.h"
#include "LCD.h"
#include "Utils.h"

#define VERSUS_SCREEN_W 240.0f
#define VERSUS_SCREEN_H 240.0f
#define VERSUS_MAX_HP 100U
#define VERSUS_PLAYER_SPEED 3.0f
#define VERSUS_DASH_DISTANCE 24.0f
#define VERSUS_DASH_COOLDOWN 80U
#define VERSUS_FIRE_COOLDOWN_TTL 26U
#define VERSUS_DOUBLE_TIMER 520U
#define VERSUS_POWERUP_INTERVAL 420U
#define VERSUS_P1_MIN_Y 144.0f
#define VERSUS_P1_MAX_Y 206.0f
#define VERSUS_P2_MIN_Y 34.0f
#define VERSUS_P2_MAX_Y 96.0f
#define VERSUS_POWERUP_MIN_Y 112.0f
#define VERSUS_POWERUP_MAX_Y 128.0f

#define VERSUS_POWERUP_HEAL 0U
#define VERSUS_POWERUP_DOUBLE 1U
#define VERSUS_POWERUP_SHIELD 2U

#define VERSUS_BULLET_RAIL 0U
#define VERSUS_BULLET_SPREAD 1U
#define VERSUS_BULLET_LANCE 2U

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static float deg_to_rad(float deg)
{
    return deg * ((float)M_PI / 180.0f);
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

static void draw_sprite_clipped(int16_t x0, int16_t y0, uint16_t width, uint16_t height, const uint8_t* sprite)
{
    int16_t y;

    for (y = 0; y < (int16_t)height; y++) {
        int16_t x;
        int16_t screen_y = y0 + y;
        if (screen_y < 0 || screen_y >= (int16_t)VERSUS_SCREEN_H) {
            continue;
        }
        for (x = 0; x < (int16_t)width; x++) {
            int16_t screen_x = x0 + x;
            uint8_t pixel;
            if (screen_x < 0 || screen_x >= (int16_t)VERSUS_SCREEN_W) {
                continue;
            }
            pixel = sprite[(y * width) + x];
            if (pixel != 255U) {
                LCD_Set_Pixel((uint16_t)screen_x, (uint16_t)screen_y, pixel);
            }
        }
    }
}

static void draw_sprite_rotated_clipped(float cx, float cy, float angle_deg, uint16_t width, uint16_t height, const uint8_t* sprite)
{
    float r = deg_to_rad(angle_deg);
    float c = cosf(r);
    float s = sinf(r);
    float half_w = (float)width * 0.5f;
    float half_h = (float)height * 0.5f;
    int16_t x0 = (int16_t)(cx - half_w);
    int16_t y0 = (int16_t)(cy - half_h);
    int16_t y;

    for (y = 0; y < (int16_t)height; y++) {
        int16_t x;
        int16_t screen_y = y0 + y;
        float dy = ((float)y + 0.5f) - half_h;

        if (screen_y < 0 || screen_y >= (int16_t)VERSUS_SCREEN_H) {
            continue;
        }
        for (x = 0; x < (int16_t)width; x++) {
            int16_t screen_x = x0 + x;
            float dx = ((float)x + 0.5f) - half_w;
            float src_xf;
            float src_yf;
            int16_t src_x;
            int16_t src_y;
            uint8_t pixel;

            if (screen_x < 0 || screen_x >= (int16_t)VERSUS_SCREEN_W) {
                continue;
            }

            src_xf = (c * dx) + (s * dy) + half_w;
            src_yf = (-s * dx) + (c * dy) + half_h;
            src_x = (int16_t)src_xf;
            src_y = (int16_t)src_yf;

            if (src_x < 0 || src_x >= (int16_t)width || src_y < 0 || src_y >= (int16_t)height) {
                continue;
            }

            pixel = sprite[(src_y * width) + src_x];
            if (pixel != 255U) {
                LCD_Set_Pixel((uint16_t)screen_x, (uint16_t)screen_y, pixel);
            }
        }
    }
}

static void clear_entities(VersusEngine_t* engine)
{
    uint8_t i;

    for (i = 0; i < VERSUS_MAX_BULLETS; i++) {
        engine->bullets[i].active = 0U;
    }
    for (i = 0; i < VERSUS_MAX_POWERUPS; i++) {
        engine->powerups[i].active = 0U;
    }
}

static void setup_player(VersusPlayer_t* p, float x, float y, float angle, SpaceShipType_t ship)
{
    p->x = x;
    p->y = y;
    p->angle = angle;
    p->hp = VERSUS_MAX_HP;
    p->shield = 0U;
    p->double_timer = 0U;
    p->dash_cooldown = 0U;
    p->hits = 0U;
    p->hit_flash = 0U;
    p->ship = (ship >= SHIP_COUNT) ? SHIP_RAIL : (uint8_t)ship;
}

static uint8_t spawn_bullet_at(VersusEngine_t* engine,
                               uint8_t owner,
                               float x,
                               float y,
                               float angle,
                               uint8_t damage,
                               uint8_t type)
{
    uint8_t i;
    float r = deg_to_rad(angle);

    for (i = 0; i < VERSUS_MAX_BULLETS; i++) {
        VersusBullet_t* b = &engine->bullets[i];
        if (!b->active) {
            b->x = x;
            b->y = y;
            b->vx = sinf(r) * ((type == VERSUS_BULLET_RAIL) ? 5.7f : 4.8f);
            b->vy = -cosf(r) * ((type == VERSUS_BULLET_RAIL) ? 5.7f : 4.8f);
            b->owner = owner;
            b->damage = damage;
            b->type = type;
            b->ttl = (type == VERSUS_BULLET_LANCE) ? 70U : 52U;
            b->active = 1U;
            return 1U;
        }
    }
    return 0U;
}

static uint8_t spawn_player_bullets(VersusEngine_t* engine, VersusPlayer_t* p, uint8_t owner)
{
    uint8_t fired = 0U;

    if (p->ship == SHIP_SPREAD) {
        fired |= spawn_bullet_at(engine, owner, p->x, p->y, p->angle, 7U, VERSUS_BULLET_SPREAD);
        fired |= spawn_bullet_at(engine, owner, p->x, p->y, p->angle - 18.0f, 6U, VERSUS_BULLET_SPREAD);
        fired |= spawn_bullet_at(engine, owner, p->x, p->y, p->angle + 18.0f, 6U, VERSUS_BULLET_SPREAD);
        if (p->double_timer > 0U) {
            fired |= spawn_bullet_at(engine, owner, p->x, p->y, p->angle - 34.0f, 5U, VERSUS_BULLET_SPREAD);
            fired |= spawn_bullet_at(engine, owner, p->x, p->y, p->angle + 34.0f, 5U, VERSUS_BULLET_SPREAD);
        }
        return fired;
    }

    if (p->ship == SHIP_LANCE) {
        return spawn_bullet_at(engine, owner, p->x, p->y, p->angle, (p->double_timer > 0U) ? 18U : 14U, VERSUS_BULLET_LANCE);
    }

    if (p->double_timer > 0U) {
        fired |= spawn_bullet_at(engine, owner, p->x - 5.0f, p->y, p->angle, 15U, VERSUS_BULLET_RAIL);
        fired |= spawn_bullet_at(engine, owner, p->x + 5.0f, p->y, p->angle, 15U, VERSUS_BULLET_RAIL);
        return fired;
    }
    return spawn_bullet_at(engine, owner, p->x, p->y, p->angle, 20U, VERSUS_BULLET_RAIL);
}

static void clamp_player_to_half(VersusPlayer_t* p, uint8_t owner)
{
    p->x = clampf(p->x, 24.0f, 216.0f);
    if (owner == 1U) {
        p->y = clampf(p->y, VERSUS_P1_MIN_Y, VERSUS_P1_MAX_Y);
    } else {
        p->y = clampf(p->y, VERSUS_P2_MIN_Y, VERSUS_P2_MAX_Y);
    }
}

static void move_player(VersusPlayer_t* p, UserInput input, uint8_t owner)
{
    if (input.angle >= 0.0f && input.magnitude > 0.06f) {
        float r = deg_to_rad(input.angle);
        float speed = VERSUS_PLAYER_SPEED * input.magnitude;
        p->angle = input.angle;
        p->x += sinf(r) * speed;
        p->y -= cosf(r) * speed;
    }

    clamp_player_to_half(p, owner);
}

static uint8_t dash_player(VersusPlayer_t* p, UserInput input, uint8_t owner)
{
    float angle = (input.angle >= 0.0f && input.magnitude > 0.06f) ? input.angle : p->angle;
    float r;

    if (p->dash_cooldown > 0U) {
        return 0U;
    }
    r = deg_to_rad(angle);
    p->x += sinf(r) * VERSUS_DASH_DISTANCE;
    p->y -= cosf(r) * VERSUS_DASH_DISTANCE;
    clamp_player_to_half(p, owner);
    p->dash_cooldown = VERSUS_DASH_COOLDOWN;
    return 1U;
}

static void damage_player(VersusEngine_t* engine, VersusPlayer_t* target, VersusPlayer_t* shooter, uint8_t damage)
{
    if (target->shield) {
        target->shield = 0U;
        target->hit_flash = 10U;
        return;
    }

    shooter->hits++;
    target->hit_flash = 14U;
    if (damage >= target->hp) {
        target->hp = 0U;
        engine->game_over = 1U;
        engine->winner = (target == &engine->p1) ? VERSUS_WINNER_P2 : VERSUS_WINNER_P1;
    } else {
        target->hp = (uint8_t)(target->hp - damage);
    }
}

static void spawn_powerup(VersusEngine_t* engine)
{
    uint8_t i;

    for (i = 0; i < VERSUS_MAX_POWERUPS; i++) {
        VersusPowerup_t* p = &engine->powerups[i];
        if (!p->active) {
            p->x = 44.0f + (float)Random_U16(152);
            p->y = VERSUS_POWERUP_MIN_Y + (float)Random_U16((uint16_t)(VERSUS_POWERUP_MAX_Y - VERSUS_POWERUP_MIN_Y + 1.0f));
            p->vx = ((float)((int16_t)Random_U16(60) - 30) / 100.0f);
            p->vy = ((float)((int16_t)Random_U16(50) - 25) / 100.0f);
            if (fabsf(p->vy) < 0.08f) {
                p->vy = (Random_U16(2) == 0U) ? -0.16f : 0.16f;
            }
            p->type = (uint8_t)Random_U16(3);
            p->ttl = 520U;
            p->active = 1U;
            engine->powerup_flash = 18U;
            return;
        }
    }
}

static void apply_powerup(VersusPlayer_t* player, uint8_t type)
{
    if (type == VERSUS_POWERUP_HEAL) {
        uint16_t hp = (uint16_t)player->hp + 24U;
        player->hp = (uint8_t)((hp > VERSUS_MAX_HP) ? VERSUS_MAX_HP : hp);
    } else if (type == VERSUS_POWERUP_DOUBLE) {
        player->double_timer = VERSUS_DOUBLE_TIMER;
    } else {
        player->shield = 1U;
    }
}

void VersusEngine_Init(VersusEngine_t* engine)
{
    VersusEngine_SetShips(engine, SHIP_RAIL, SHIP_RAIL);
    VersusEngine_StartGame(engine);
}

void VersusEngine_SetShips(VersusEngine_t* engine, SpaceShipType_t p1_ship, SpaceShipType_t p2_ship)
{
    engine->p1.ship = (p1_ship >= SHIP_COUNT) ? SHIP_RAIL : (uint8_t)p1_ship;
    engine->p2.ship = (p2_ship >= SHIP_COUNT) ? SHIP_RAIL : (uint8_t)p2_ship;
}

void VersusEngine_StartGame(VersusEngine_t* engine)
{
    SpaceShipType_t p1_ship = (engine->p1.ship >= SHIP_COUNT) ? SHIP_RAIL : (SpaceShipType_t)engine->p1.ship;
    SpaceShipType_t p2_ship = (engine->p2.ship >= SHIP_COUNT) ? SHIP_RAIL : (SpaceShipType_t)engine->p2.ship;

    setup_player(&engine->p1, 72.0f, 184.0f, 0.0f, p1_ship);
    setup_player(&engine->p2, 168.0f, 56.0f, 180.0f, p2_ship);
    engine->frame_counter = 0U;
    engine->powerup_timer = 180U;
    engine->powerup_flash = 0U;
    engine->game_over = 0U;
    engine->winner = VERSUS_WINNER_NONE;
    clear_entities(engine);
}

uint8_t VersusEngine_Update(VersusEngine_t* engine,
                            UserInput p1_input,
                            UserInput p2_input,
                            uint8_t p1_fire,
                            uint8_t p2_fire,
                            uint8_t p1_dash,
                            uint8_t p2_dash)
{
    uint8_t i;
    uint8_t events = VERSUS_EVENT_NONE;

    if (engine->game_over) {
        return VERSUS_EVENT_NONE;
    }

    engine->frame_counter++;
    move_player(&engine->p1, p1_input, 1U);
    move_player(&engine->p2, p2_input, 2U);

    if (engine->p1.dash_cooldown > 0U) {
        engine->p1.dash_cooldown--;
    }
    if (engine->p2.dash_cooldown > 0U) {
        engine->p2.dash_cooldown--;
    }
    if (engine->p1.double_timer > 0U) {
        engine->p1.double_timer--;
    }
    if (engine->p2.double_timer > 0U) {
        engine->p2.double_timer--;
    }
    if (engine->p1.hit_flash > 0U) {
        engine->p1.hit_flash--;
    }
    if (engine->p2.hit_flash > 0U) {
        engine->p2.hit_flash--;
    }
    if (engine->powerup_flash > 0U) {
        engine->powerup_flash--;
    }

    if (p1_dash && dash_player(&engine->p1, p1_input, 1U)) {
        events |= VERSUS_EVENT_DASH;
    }
    if (p2_dash && dash_player(&engine->p2, p2_input, 2U)) {
        events |= VERSUS_EVENT_DASH;
    }

    if (p1_fire && spawn_player_bullets(engine, &engine->p1, 1U)) {
        events |= VERSUS_EVENT_P1_FIRE;
    }
    if (p2_fire && spawn_player_bullets(engine, &engine->p2, 2U)) {
        events |= VERSUS_EVENT_P2_FIRE;
    }

    if (engine->powerup_timer > 0U) {
        engine->powerup_timer--;
    } else {
        spawn_powerup(engine);
        engine->powerup_timer = VERSUS_POWERUP_INTERVAL;
    }

    for (i = 0; i < VERSUS_MAX_BULLETS; i++) {
        VersusBullet_t* b = &engine->bullets[i];
        if (!b->active) {
            continue;
        }
        b->x += b->vx;
        b->y += b->vy;
        if (b->ttl > 0U) {
            b->ttl--;
        }
        if (b->ttl == 0U || b->x < -8.0f || b->x > 248.0f || b->y < -8.0f || b->y > 248.0f) {
            b->active = 0U;
            continue;
        }
        if (b->owner == 1U && collide_circle(b->x, b->y, 3.0f, engine->p2.x, engine->p2.y, 14.0f)) {
            uint8_t shield_was_active = engine->p2.shield;
            b->active = 0U;
            damage_player(engine, &engine->p2, &engine->p1, b->damage);
            events |= shield_was_active ? VERSUS_EVENT_SHIELD_BLOCK : VERSUS_EVENT_HIT;
        } else if (b->owner == 2U && collide_circle(b->x, b->y, 3.0f, engine->p1.x, engine->p1.y, 14.0f)) {
            uint8_t shield_was_active = engine->p1.shield;
            b->active = 0U;
            damage_player(engine, &engine->p1, &engine->p2, b->damage);
            events |= shield_was_active ? VERSUS_EVENT_SHIELD_BLOCK : VERSUS_EVENT_HIT;
        }
    }

    for (i = 0; i < VERSUS_MAX_POWERUPS; i++) {
        VersusPowerup_t* p = &engine->powerups[i];
        if (!p->active) {
            continue;
        }
        p->x += p->vx;
        p->y += p->vy;
        if (p->x < 18.0f || p->x > 222.0f) {
            p->vx = -p->vx;
        }
        if (p->y < VERSUS_POWERUP_MIN_Y) {
            p->y = VERSUS_POWERUP_MIN_Y;
            p->vy = fabsf(p->vy);
        } else if (p->y > VERSUS_POWERUP_MAX_Y) {
            p->y = VERSUS_POWERUP_MAX_Y;
            p->vy = -fabsf(p->vy);
        }
        if (p->ttl > 0U) {
            p->ttl--;
        }
        if (p->ttl == 0U) {
            p->active = 0U;
            continue;
        }
        if (collide_circle(engine->p1.x, engine->p1.y, 14.0f, p->x, p->y, 10.0f)) {
            apply_powerup(&engine->p1, p->type);
            p->active = 0U;
            events |= VERSUS_EVENT_POWERUP;
        } else if (collide_circle(engine->p2.x, engine->p2.y, 14.0f, p->x, p->y, 10.0f)) {
            apply_powerup(&engine->p2, p->type);
            p->active = 0U;
            events |= VERSUS_EVENT_POWERUP;
        }
    }

    return events;
}

static void draw_player(VersusPlayer_t* p, uint8_t label_colour, const char* label)
{
    if (p->hit_flash > 0U && ((p->hit_flash / 3U) & 1U) == 0U) {
        LCD_Draw_Circle((uint16_t)p->x, (uint16_t)p->y, 31, 2U, 0);
        LCD_Draw_Circle((uint16_t)p->x, (uint16_t)p->y, 24, 1U, 0);
    }
    draw_sprite_rotated_clipped(p->x,
                                p->y,
                                p->angle,
                                STRIKER_PLAYER_SHIP_W,
                                STRIKER_PLAYER_SHIP_H,
                                STRIKER_PLAYER_SHIPS[p->ship]);
    LCD_printString(label, (uint16_t)(p->x - 8.0f), (uint16_t)(p->y + 22.0f), label_colour, 1);
    if (p->shield) {
        LCD_Draw_Circle((uint16_t)p->x, (uint16_t)p->y, 27, 14, 0);
    }
}

static void draw_arena_frame(VersusEngine_t* engine)
{
    uint8_t i;
    uint8_t pulse = ((engine->frame_counter / 8U) & 1U) ? 13U : 1U;

    for (i = 0; i < 12U; i++) {
        uint16_t x = (uint16_t)(12U + (i * 19U));
        uint8_t colour = (i & 1U) ? 2U : 14U;
        LCD_Draw_Line(x, 120, (uint16_t)(x + 9U), 120, colour);
        if (engine->powerup_flash > 0U) {
            LCD_Set_Pixel((uint16_t)(x + 4U), 118, pulse);
            LCD_Set_Pixel((uint16_t)(x + 4U), 122, pulse);
        }
    }

    LCD_Draw_Line(4, 30, 36, 30, 2U);
    LCD_Draw_Line(4, 30, 4, 62, 2U);
    LCD_Draw_Line(236, 30, 204, 30, 2U);
    LCD_Draw_Line(236, 30, 236, 62, 2U);
    LCD_Draw_Line(4, 210, 36, 210, 14U);
    LCD_Draw_Line(4, 210, 4, 178, 14U);
    LCD_Draw_Line(236, 210, 204, 210, 14U);
    LCD_Draw_Line(236, 210, 236, 178, 14U);
}

static void draw_bullet(VersusBullet_t* b)
{
    uint8_t colour = (b->owner == 1U) ? 14U : 2U;

    if (b->type == VERSUS_BULLET_LANCE) {
        LCD_Draw_Line((uint16_t)b->x, (uint16_t)b->y,
                      (uint16_t)clampf(b->x - (b->vx * 2.4f), 0.0f, 239.0f),
                      (uint16_t)clampf(b->y - (b->vy * 2.4f), 0.0f, 239.0f), 11);
    } else {
        LCD_Draw_Circle((uint16_t)b->x, (uint16_t)b->y, (b->type == VERSUS_BULLET_RAIL) ? 3U : 2U, colour, 1);
        LCD_Set_Pixel((uint16_t)b->x, (uint16_t)b->y, 1);
    }
}

static void draw_hp_bar(uint16_t x, uint16_t y, uint8_t hp, uint8_t colour, const char* label)
{
    uint16_t fill = (uint16_t)(((uint16_t)hp * 72U) / VERSUS_MAX_HP);

    LCD_printString(label, x, y, colour, 1);
    LCD_Draw_Rect((uint16_t)(x + 24U), (uint16_t)(y + 1U), 76, 8, 13, 0);
    if (fill > 0U) {
        LCD_Draw_Rect((uint16_t)(x + 26U), (uint16_t)(y + 3U), fill, 4, colour, 1);
    }
}

void VersusEngine_Draw(VersusEngine_t* engine)
{
    uint8_t i;

    for (i = 0; i < 20U; i++) {
        uint16_t x = (uint16_t)((i * 53U + engine->frame_counter) % 240U);
        uint16_t y = (uint16_t)((i * 31U + (engine->frame_counter / 2U)) % 240U);
        LCD_Set_Pixel(x, y, (i % 3U) ? 13U : 1U);
    }

    draw_arena_frame(engine);
    draw_hp_bar(8, 8, engine->p2.hp, 2U, "P2");
    draw_hp_bar(8, 222, engine->p1.hp, 14U, "P1");

    for (i = 0; i < VERSUS_MAX_POWERUPS; i++) {
        VersusPowerup_t* p = &engine->powerups[i];
        if (p->active) {
            uint8_t type = (p->type < AST_POWERUP_SPRITE_COUNT) ? p->type : 0U;
            uint8_t ring_colour = ((engine->frame_counter / 6U) & 1U) ? 14U : 1U;
            LCD_Draw_Circle((uint16_t)p->x, (uint16_t)p->y, 13, ring_colour, 0);
            draw_sprite_clipped((int16_t)(p->x - 9.0f), (int16_t)(p->y - 9.0f),
                                AST_POWERUP_SPRITE_W, AST_POWERUP_SPRITE_H, AST_POWERUP_SPRITES[type]);
        }
    }

    for (i = 0; i < VERSUS_MAX_BULLETS; i++) {
        if (engine->bullets[i].active) {
            draw_bullet(&engine->bullets[i]);
        }
    }

    draw_player(&engine->p1, 14U, "P1");
    draw_player(&engine->p2, 2U, "P2");
}

uint8_t VersusEngine_IsGameOver(VersusEngine_t* engine)
{
    return engine->game_over;
}

VersusWinner_t VersusEngine_GetWinner(VersusEngine_t* engine)
{
    return (VersusWinner_t)engine->winner;
}

uint8_t VersusEngine_GetP1Hp(VersusEngine_t* engine)
{
    return engine->p1.hp;
}

uint8_t VersusEngine_GetP2Hp(VersusEngine_t* engine)
{
    return engine->p2.hp;
}

uint16_t VersusEngine_GetP1Hits(VersusEngine_t* engine)
{
    return engine->p1.hits;
}

uint16_t VersusEngine_GetP2Hits(VersusEngine_t* engine)
{
    return engine->p2.hits;
}
