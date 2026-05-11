#pragma once

#include <stdint.h>
#include "Defines.h"
#include "Player.h"
#include "Enemy.h"
#include "TitleFlow.h"

class Entity;

struct Stats
{
	EnemyType killedBy;
	uint8_t enemyKills[(int)EnemyType::NumEnemyTypes];
	uint8_t casketsOpened;
	uint8_t idolsCollected;
	uint8_t tabletsCollected;
	uint8_t shardsCollected;

	void Reset();
};

class TombGame
{
public:
	static uint8_t globalTickFrame;
	static constexpr uint8_t kMissionFloors = 2;

	enum class State : uint8_t
	{
		TitleScreen,
		EnteringLevel,
		InGame,
		GameOver,
		MissionComplete,
		FadeOut
	};

	static void Init();
	static void Tick();
	static void Draw();

	static void StartGame();
	static void StartLevel();
	static void NextLevel();
	static void GameOver();
	static void MissionComplete();
	static bool ShouldReturnToMenu();
	static void ClearReturnToMenu();
	static void RequestReturnToMenu();
	
	static void SwitchState(State newState);
	static State GetState();

	static void ShowMessage(const char* message);

	static Player player;

	static const char* displayMessage;
	static uint8_t displayMessageTime;
	static uint8_t floor;

	static Stats stats;

private:
	static void TickInGame();
		
	static TitleFlow titleFlow;
	static State state;
	static bool returnToMenu;
};



