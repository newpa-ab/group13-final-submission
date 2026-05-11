#include "Defines.h"
#include "Platform.h"
#include "TitleFlow.h"
#include "Font.h"
#include "TombGame.h"
#include "FixedMath.h"
#include "SceneRenderer.h"
#include "Generated/SpriteTypes.h"

static void PrintCentered(const char* text, uint8_t line, uint8_t colour = COLOUR_WHITE)
{
	uint8_t textWidth = (uint8_t)(strlen_P(text) * Font::glyphWidth);
	uint8_t x = textWidth >= DISPLAY_WIDTH ? 0 : (uint8_t)((DISPLAY_WIDTH - textWidth) / 2);
	Font::PrintString(text, line, x, colour);
}

void TitleFlow::Init()
{
	selection = 0;
}

void TitleFlow::Draw()
{
	Platform::FillScreen(COLOUR_BLACK);
	Platform::TintRect(20, 8, 88, 8, 5);
	PrintCentered(PSTR("MISSION 1: LOCATE"), 1);
	PrintCentered(PSTR("FIND SIGNAL SOURCE"), 3);

	if (selection == 0)
	{
		Platform::TintRect(18, 32, 92, 8, 10);
		Font::PrintString(PSTR(">"), 4, 18, COLOUR_WHITE);
		Font::PrintString(PSTR("<"), 4, 106, COLOUR_WHITE);
	}
	else
	{
		Platform::TintRect(18, 40, 92, 8, 10);
		Font::PrintString(PSTR(">"), 5, 18, COLOUR_WHITE);
		Font::PrintString(PSTR("<"), 5, 106, COLOUR_WHITE);
	}

	Font::PrintString(PSTR("START SEARCH"), 4, 40, COLOUR_WHITE);
	if (Platform::IsAudioEnabled())
	{
		Font::PrintString(PSTR("AUDIO : ON"), 5, 44, COLOUR_WHITE);
	}
	else
	{
		Font::PrintString(PSTR("AUDIO : OFF"), 5, 42, COLOUR_WHITE);
	}

	Font::PrintString(PSTR("A/B CONFIRM"), 7, 42, COLOUR_WHITE);
	
	const uint16_t* torchSprite = TombGame::globalTickFrame & 4 ? torchSpriteData1 : torchSpriteData2;
	Platform::SetDrawTint(10);
	SceneRenderer::DrawScaled(torchSprite, 6, 22, 9, 255);
	SceneRenderer::DrawScaled(torchSprite, DISPLAY_WIDTH - 24, 22, 9, 255);
	Platform::SetDrawTint(0);
}

void TitleFlow::Tick()
{
	static uint8_t lastInput = 0;
	uint8_t input = Platform::GetInput();
	
	// Spin the RNG to have a unique(ish) starting level
	Random();

	if ((input & (INPUT_DOWN | INPUT_RIGHT)) && !(lastInput & (INPUT_DOWN | INPUT_RIGHT)))
	{
		selection = 1;
	}
	if ((input & (INPUT_UP | INPUT_LEFT)) && !(lastInput & (INPUT_UP | INPUT_LEFT)))
	{
		selection = 0;
	}

	if ((input & (INPUT_A | INPUT_B)) && !(lastInput & (INPUT_A | INPUT_B)))
	{
		switch (selection)
		{
		case 0:			
			TombGame::StartGame();
			break;
		case 1:
			Platform::SetAudioEnabled(!Platform::IsAudioEnabled());
			break;
		}
	}

	lastInput = input;
}

void TitleFlow::ResetTimer()
{
	timer = 0;
}

void TitleFlow::TickEnteringLevel()
{
	constexpr uint8_t showTime = 30;
	
	if(timer < showTime)
	{
		timer++;
	}
	
	if(timer == showTime && Platform::GetInput() == 0)
	{
		TombGame::StartLevel();
	}
}

void TitleFlow::DrawEnteringLevel()
{
	Platform::FillScreen(COLOUR_BLACK);
	Platform::TintRect(24, 8, 80, 8, 5);
	Platform::TintRect(48, 32, 32, 8, 10);
	PrintCentered(PSTR("INFILTRATION"), 1);
	PrintCentered(PSTR("ONE SECTOR ONLY"), 3);
	PrintCentered(PSTR("TARGET: SOURCE"), 4);
	PrintCentered(PSTR("FOLLOW THE SIGNAL"), 7);
}	

void TitleFlow::TickGameOver()
{
	constexpr uint8_t minShowTime = 30;
	
	if(timer < minShowTime)
	{
		timer++;
	}
	
	if(timer == minShowTime && (Platform::GetInput() & (INPUT_A | INPUT_B)))
	{
		timer++;
	}
	else if(timer == minShowTime + 1 && Platform::GetInput() == 0)
	{
		TombGame::SwitchState(TombGame::State::TitleScreen);
	}
}

void TitleFlow::DrawGameOver()
{
	Platform::FillScreen(COLOUR_BLACK);
	Platform::TintRect(22, 0, 84, 8, 5);
	Font::PrintString(PSTR("TOMB RECORD"), 0, 24, COLOUR_WHITE);

	uint16_t finalScore = 0;
	constexpr int finishBonus = 500;
	constexpr int levelBonus = 20;
	constexpr int chestBonus = 15;
	constexpr int crownBonus = 10;
	constexpr int scrollBonus = 8;
	constexpr int coinsBonus = 4;
	constexpr int skeletonKillBonus = 10;
	constexpr int mageKillBonus = 10;
	constexpr int batKillBonus = 5;
	constexpr int spiderKillBonus = 4;

	finalScore += (TombGame::floor - 1) * levelBonus;

	if (TombGame::stats.killedBy == EnemyType::None)
		finalScore += finishBonus;
	finalScore += TombGame::stats.casketsOpened * chestBonus;
	finalScore += TombGame::stats.idolsCollected * crownBonus;
	finalScore += TombGame::stats.tabletsCollected * scrollBonus;
	finalScore += TombGame::stats.shardsCollected * coinsBonus;
	finalScore += TombGame::stats.enemyKills[(int)EnemyType::Skeleton] * skeletonKillBonus;
	finalScore += TombGame::stats.enemyKills[(int)EnemyType::Mage] * mageKillBonus;
	finalScore += TombGame::stats.enemyKills[(int)EnemyType::Bat] * batKillBonus;
	finalScore += TombGame::stats.enemyKills[(int)EnemyType::Spider] * spiderKillBonus;

	switch (TombGame::stats.killedBy)
	{
	case EnemyType::None:
		Font::PrintString(PSTR("THE GATE YIELDED"), 1, 34, COLOUR_WHITE);
		break;
	case EnemyType::Mage:
		Font::PrintString(PSTR("FELLED BY A MAGE"), 1, 32, COLOUR_WHITE);
		break;
	case EnemyType::Skeleton:
		Font::PrintString(PSTR("FELLED BY A KNIGHT"), 1, 28, COLOUR_WHITE);
		break;
	case EnemyType::Bat:
		Font::PrintString(PSTR("FELLED BY A BAT"), 1, 36, COLOUR_WHITE);
		break;
	case EnemyType::Spider:
		Font::PrintString(PSTR("FELLED BY A SPIDER"), 1, 28, COLOUR_WHITE);
		break;
	}
	Platform::TintRect(8, 16, 36, 8, 10);
	Platform::TintRect(64, 16, 56, 8, 5);
	Font::PrintString(PSTR("LVL"), 2, 16, COLOUR_WHITE);
	Font::PrintInt(TombGame::floor, 2, 30, COLOUR_WHITE);
	Font::PrintString(PSTR("SCR"), 2, 72, COLOUR_WHITE);
	Font::PrintInt(finalScore, 2, 88, COLOUR_WHITE);
	
	Platform::TintRect(0, 24, 54, 8, 10);
	Platform::TintRect(74, 24, 54, 8, 8);
	Font::PrintString(PSTR("RELICS"), 3, 8, COLOUR_WHITE);
	Font::PrintString(PSTR("THREATS"), 3, 82, COLOUR_WHITE);

	constexpr uint8_t firstRow = 33;
	constexpr uint8_t secondRow = 45;

	SceneRenderer::DrawScaled(chestSpriteData, 0, firstRow, 9, 255);
	Font::PrintInt(TombGame::stats.casketsOpened, 4, 18, COLOUR_WHITE);

	SceneRenderer::DrawScaled(crownSpriteData, 0, secondRow, 9, 255);
	Font::PrintInt(TombGame::stats.idolsCollected, 6, 18, COLOUR_WHITE);

	SceneRenderer::DrawScaled(scrollSpriteData, 30, firstRow, 9, 255);
	Font::PrintInt(TombGame::stats.tabletsCollected, 4, 48, COLOUR_WHITE);

	SceneRenderer::DrawScaled(coinsSpriteData, 30, secondRow, 9, 255);
	Font::PrintInt(TombGame::stats.shardsCollected, 6, 48, COLOUR_WHITE);

	int offset = (TombGame::globalTickFrame & 8) == 0 ? 32 : 0;

	SceneRenderer::DrawScaled(skeletonSpriteData + offset, 70, firstRow, 9, 255);
	Font::PrintInt(TombGame::stats.enemyKills[(int)EnemyType::Skeleton], 4, 86, COLOUR_WHITE);

	SceneRenderer::DrawScaled(mageSpriteData + offset, 70, secondRow, 9, 255);
	Font::PrintInt(TombGame::stats.enemyKills[(int)EnemyType::Mage], 6, 86, COLOUR_WHITE);

	SceneRenderer::DrawScaled(batSpriteData + offset, 96, firstRow, 9, 255, true);
	Font::PrintInt(TombGame::stats.enemyKills[(int)EnemyType::Bat], 4, 110, COLOUR_WHITE);

	SceneRenderer::DrawScaled(spiderSpriteData + offset, 96, secondRow, 9, 255);
	Font::PrintInt(TombGame::stats.enemyKills[(int)EnemyType::Spider], 6, 110, COLOUR_WHITE);
}

void TitleFlow::TickMissionComplete()
{
	constexpr uint8_t minShowTime = 75;

	if (timer < minShowTime)
	{
		timer++;
	}

	if (timer >= minShowTime && (Platform::GetInput() & (INPUT_A | INPUT_B)))
	{
		TombGame::RequestReturnToMenu();
	}
}

void TitleFlow::DrawMissionComplete()
{
	Platform::FillScreen(COLOUR_BLACK);

	for (uint8_t y = 0; y < DISPLAY_HEIGHT; y += 8)
	{
		Platform::TintRect(0, y, DISPLAY_WIDTH, 1, (uint8_t)(3 + ((y + TombGame::globalTickFrame) & 3)));
	}

	uint8_t pulse = (uint8_t)(TombGame::globalTickFrame & 15);
	uint8_t coreSize = (uint8_t)(8 + (pulse < 8 ? pulse : 15 - pulse));
	uint8_t cx = DISPLAY_WIDTH / 2;
	uint8_t cy = 30;

	Platform::TintRect(20, 4, 88, 8, 5);
	PrintCentered(PSTR("SIGNAL SOURCE"), 0);
	PrintCentered(PSTR("FOUND"), 1);

	Platform::TintRect((uint8_t)(cx - coreSize), (uint8_t)(cy - coreSize / 2), (uint8_t)(coreSize * 2), coreSize, 10);
	Platform::TintRect((uint8_t)(cx - 4), (uint8_t)(cy - 4), 8, 8, 3);

	for (uint8_t r = 0; r < 3; r++)
	{
		uint8_t offset = (uint8_t)(14 + r * 10 + ((TombGame::globalTickFrame >> 2) & 3));
		Platform::PutPixel((uint8_t)(cx - offset), cy, COLOUR_WHITE);
		Platform::PutPixel((uint8_t)(cx + offset), cy, COLOUR_WHITE);
		Platform::PutPixel((uint8_t)(cx - offset + 2), (uint8_t)(cy - 3 - r), COLOUR_WHITE);
		Platform::PutPixel((uint8_t)(cx + offset - 2), (uint8_t)(cy - 3 - r), COLOUR_WHITE);
		Platform::PutPixel((uint8_t)(cx - offset + 2), (uint8_t)(cy + 3 + r), COLOUR_WHITE);
		Platform::PutPixel((uint8_t)(cx + offset - 2), (uint8_t)(cy + 3 + r), COLOUR_WHITE);
	}

	PrintCentered(PSTR("CORE COORDINATES"), 5);
	PrintCentered(PSTR("SENT TO GAME 2"), 6);

	if (timer >= 75)
	{
		PrintCentered(PSTR("A/B RETURN MENU"), 7);
	}
}

#include <stdio.h>

void TitleFlow::FadeOut()
{
	constexpr uint16_t toggleMask = 0x1f6e;
	constexpr int fizzlesPerFrame = 255;
	constexpr uint16_t startValue = 1;

	if (fizzleFade == 0)
	{
		fizzleFade = startValue;
	}

	for (int n = 0; n < fizzlesPerFrame; n++)
	{
		bool lsb = (fizzleFade & 1) != 0;
		fizzleFade >>= 1;
		if (lsb)
		{
			fizzleFade ^= toggleMask;
		}

		uint8_t x = (uint8_t)(fizzleFade & 0x7f);
		uint8_t y = (uint8_t)(fizzleFade >> 7);
		Platform::PutPixel(x, y, COLOUR_BLACK);

		if (fizzleFade == startValue)
		{
			TombGame::SwitchState(TombGame::State::GameOver);
			return;
		}
	}

}




