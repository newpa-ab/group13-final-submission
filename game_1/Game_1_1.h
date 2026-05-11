#ifndef GAME_1_H
#define GAME_1_H

#include "Menu.h"

#ifdef __cplusplus
extern "C" {
#endif

MenuState Game1_Run(void);

void Game1_Init(void);
void Game1_Tick(void);
void Game1_Draw(void);
void Game1_RunFrame(void);
int Game1_IsInWorldView(void);

#ifdef __cplusplus
}
#endif

#endif
