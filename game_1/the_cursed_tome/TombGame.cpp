#include "Defines.h"
#include "TombGame.h"
#include "FixedMath.h"
#include "SceneRenderer.h"
#include "Map.h"
#include "Projectile.h"
#include "Particle.h"
#include "MapGenerator.h"
#include "Platform.h"
#include "Entity.h"
#include "Enemy.h"
#include "TitleFlow.h"

Player TombGame::player;
const char* TombGame::displayMessage = nullptr;
uint8_t TombGame::displayMessageTime = 0;
TombGame::State TombGame::state = TombGame::State::TitleScreen;
uint8_t TombGame::floor = 1;
uint8_t TombGame::globalTickFrame = 0;
Stats TombGame::stats;
TitleFlow TombGame::titleFlow;
bool TombGame::returnToMenu = false;

void TombGame::Init()
{
	titleFlow.Init();
	ParticleSystemManager::Init();
	ProjectileManager::Init();
	EnemyManager::Init();
}

void TombGame::StartGame()
{
	floor = 1;
	returnToMenu = false;
	stats.Reset();
	player.Init();
	SwitchState(State::EnteringLevel);
}

void TombGame::SwitchState(State newState)
{
	if(state != newState)
	{
		state = newState;
		titleFlow.ResetTimer();
	}
}

TombGame::State TombGame::GetState()
{
	return state;
}

void TombGame::ShowMessage(const char* message)
{
	constexpr uint8_t messageDisplayTime = 90;

	displayMessage = message;
	displayMessageTime = messageDisplayTime;
}

void TombGame::NextLevel()
{
	if (floor < kMissionFloors)
	{
		floor++;
		SwitchState(State::EnteringLevel);
	}
	else
	{
		MissionComplete();
	}
}

void TombGame::StartLevel()
{
	ParticleSystemManager::Init();
	ProjectileManager::Init();
	EnemyManager::Init();
	MapGenerator::Generate();
	EnemyManager::SpawnEnemies();

	player.NextLevel();

	Platform::ExpectLoadDelay();
	SwitchState(State::InGame);
}

void TombGame::Draw()
{
	switch(state)
	{
		case State::TitleScreen:
			titleFlow.Draw();
			break;
		case State::EnteringLevel:
			titleFlow.DrawEnteringLevel();
			break;
		case State::InGame:
		{
			SceneRenderer::camera.x = player.x;
			SceneRenderer::camera.y = player.y;
			SceneRenderer::camera.angle = player.angle;

			SceneRenderer::Render();
		}
			break;
		case State::GameOver:
			titleFlow.DrawGameOver();
			break;
		case State::MissionComplete:
			titleFlow.DrawMissionComplete();
			break;
		case State::FadeOut:
			titleFlow.FadeOut();
			break;
	}
}

void TombGame::TickInGame()
{
	if (displayMessageTime > 0)
	{
		displayMessageTime--;
		if (displayMessageTime == 0)
			displayMessage = nullptr;
	}

	player.Tick();

	ProjectileManager::Update();
	ParticleSystemManager::Update();
	EnemyManager::Update();

	if (Map::GetCellSafe(player.x / CELL_SIZE, player.y / CELL_SIZE) == CellType::TombGate)
	{
		NextLevel();
	}

	if (player.hp == 0)
	{
		GameOver();
	}
}

void TombGame::Tick()
{
	globalTickFrame++;

	switch(state)
	{
		case State::InGame:
			TickInGame();
			return;
		case State::EnteringLevel:
			titleFlow.TickEnteringLevel();
			return;
		case State::TitleScreen:
			titleFlow.Tick();
			return;
		case State::GameOver:
			titleFlow.TickGameOver();
			return;
		case State::MissionComplete:
			titleFlow.TickMissionComplete();
			return;
	}
}

void TombGame::GameOver()
{
	SwitchState(State::FadeOut);
}

void TombGame::MissionComplete()
{
	SwitchState(State::MissionComplete);
}

bool TombGame::ShouldReturnToMenu()
{
	return returnToMenu;
}

void TombGame::ClearReturnToMenu()
{
	returnToMenu = false;
}

void TombGame::RequestReturnToMenu()
{
	returnToMenu = true;
}

void Stats::Reset()
{
	killedBy = EnemyType::None;
	casketsOpened = 0;
	shardsCollected = 0;
	idolsCollected = 0;
	tabletsCollected = 0;

	for (uint8_t& killCounter : enemyKills)
	{
		killCounter = 0;
	}
}



