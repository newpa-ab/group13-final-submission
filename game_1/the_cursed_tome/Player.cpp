#include "Player.h"
#include "TombGame.h"
#include "FixedMath.h"
#include "Projectile.h"
#include "Platform.h"
#include "SceneRenderer.h"
#include "Enemy.h"
#include "Map.h"
#include "Sounds.h"
#include "Particle.h"

#define USE_ROTATE_BOB 0
#define STRAFE_TILT 14
#define ROTATE_TILT 3

const char SignMessage1[] PROGMEM = "Turn back while the seal still sleeps.";

void Player::Init()
{
	NextLevel();
	hp = maxHP;
}

void Player::NextLevel()
{
	x = CELL_SIZE * 1 + CELL_SIZE / 2;
	y = CELL_SIZE * 1 + CELL_SIZE / 2;
	angle = FIXED_ANGLE_45;
	mana = maxMana;
	damageTime = 0;
	shakeTime = 0;
	reloadTime = 0;
	velocityX = 0;
	velocityY = 0;
	angularVelocity = 0;
	lastInput = 0;
	runeEmpowerTime = 0;
}

void Player::Fire()
{
	uint8_t manaCost = runeEmpowerTime > 0 ? (uint8_t)(manaFireCost / 2) : manaFireCost;
	uint8_t fireDelay = runeEmpowerTime > 0 ? 4 : 8;

	if (mana >= manaCost)
	{
		reloadTime = fireDelay;
		shakeTime = 6;

		int16_t projectileX = x + FixedCos(angle + FIXED_ANGLE_90 / 2) / 4;
		int16_t projectileY = y + FixedSin(angle + FIXED_ANGLE_90 / 2) / 4;

		ProjectileManager::FireProjectile(this, projectileX, projectileY, angle);
		mana -= manaCost;
		Platform::PlaySound(Sounds::Attack);
	}
}

void Player::Tick()
{
	uint8_t input = Platform::GetInput();
	bool confirmPressed = (input & (INPUT_A | INPUT_B)) != 0 && (lastInput & (INPUT_A | INPUT_B)) == 0;
	int8_t turnDelta = 0;
	int8_t targetTilt = 0;
	int8_t moveDelta = 0;
	int8_t strafeDelta = 0;

	if (input & INPUT_A)
	{
		if (input & INPUT_LEFT)
		{
			strafeDelta--;
		}
		if (input & INPUT_RIGHT)
		{
			strafeDelta++;
		}
	}
	else
	{
		if (input & INPUT_LEFT)
		{
			turnDelta -= TURN_SPEED * 2;
		}
		if (input & INPUT_RIGHT)
		{
			turnDelta += TURN_SPEED * 2;
		}
	}

	// Testing shooting / recoil mechanic

	if (reloadTime > 0)
	{
		reloadTime--;
	}
	else if (input & INPUT_B)
	{
		Fire();
	}


	if (angularVelocity < turnDelta)
	{
		angularVelocity++;
	}
	else if (angularVelocity > turnDelta)
	{
		angularVelocity--;
	}

	angle += angularVelocity >> 1;

	if (input & INPUT_UP)
	{
		moveDelta++;
	}
	if (input & INPUT_DOWN)
	{
		moveDelta--;
	}

	static int tiltTimer = 0;
	tiltTimer++;
	if (moveDelta && USE_ROTATE_BOB)
	{
		targetTilt = (int8_t)(FixedSin(tiltTimer * 10) / 32);
	}
	else
	{
		targetTilt = 0;
	}

	targetTilt += angularVelocity * ROTATE_TILT;
	targetTilt += strafeDelta * STRAFE_TILT;
	int8_t targetBob = moveDelta || strafeDelta ? FixedSin(tiltTimer * 10) / 128 : 0;

	if (shakeTime > 0)
	{
		shakeTime--;
		targetBob += (Random() & 3) - 1;
		targetTilt += (Random() & 31) - 16;
	}

	constexpr int tiltRate = 6;

	if (SceneRenderer::camera.tilt < targetTilt)
	{
		SceneRenderer::camera.tilt += tiltRate;
		if (SceneRenderer::camera.tilt > targetTilt)
		{
			SceneRenderer::camera.tilt = targetTilt;
		}
	}
	else if (SceneRenderer::camera.tilt > targetTilt)
	{
		SceneRenderer::camera.tilt -= tiltRate;
		if (SceneRenderer::camera.tilt < targetTilt)
		{
			SceneRenderer::camera.tilt = targetTilt;
		}
	}

	constexpr int bobRate = 3;

	if (SceneRenderer::camera.bob < targetBob)
	{
		SceneRenderer::camera.bob += bobRate;
		if (SceneRenderer::camera.bob > targetBob)
		{
			SceneRenderer::camera.bob = targetBob;
		}
	}
	else if (SceneRenderer::camera.bob > targetBob)
	{
		SceneRenderer::camera.bob -= bobRate;
		if (SceneRenderer::camera.bob < targetBob)
		{
			SceneRenderer::camera.bob = targetBob;
		}
	}

	int16_t cosAngle = FixedCos(angle);
	int16_t sinAngle = FixedSin(angle);

	int16_t cos90Angle = FixedCos(angle + FIXED_ANGLE_90);
	int16_t sin90Angle = FixedSin(angle + FIXED_ANGLE_90);
	//camera.x += (moveDelta * cosAngle) >> 4;
	//camera.y += (moveDelta * sinAngle) >> 4;
	velocityX += (moveDelta * cosAngle) / 24;
	velocityY += (moveDelta * sinAngle) / 24;

	velocityX += (strafeDelta * cos90Angle) / 24;
	velocityY += (strafeDelta * sin90Angle) / 24;

	Move(velocityX / 4, velocityY / 4, confirmPressed);

	velocityX = (velocityX * 7) / 8;
	velocityY = (velocityY * 7) / 8;

	if (mana < maxMana && reloadTime == 0)
	{
		mana += manaRechargeRate;
	}

	if (runeEmpowerTime > 0)
	{
		runeEmpowerTime--;
	}

	if (damageTime > 0)
		damageTime--;

	uint8_t cellX = x / CELL_SIZE;
	uint8_t cellY = y / CELL_SIZE;

	switch (Map::GetCellSafe(cellX, cellY))
	{
	case CellType::LifeEssence:
		if (hp < maxHP)
		{
			if (hp + potionStrength > maxHP)
				hp = maxHP;
			else
				hp += potionStrength;
			Map::SetCell(cellX, cellY, CellType::Empty);
			Platform::PlaySound(Sounds::Pickup);
			TombGame::ShowMessage(PSTR("Absorbed a vial of life essence"));
		}
		break;
	case CellType::RelicShard:
		Map::SetCell(cellX, cellY, CellType::Empty);
		Platform::PlaySound(Sounds::Pickup);
		if (mana + relicShardMana > maxMana)
			mana = maxMana;
		else
			mana += relicShardMana;
		reloadTime = 0;
		TombGame::ShowMessage(PSTR("Relic shards restore essence"));
		TombGame::stats.shardsCollected++;
		break;
	case CellType::SunIdol:
		Map::SetCell(cellX, cellY, CellType::Empty);
		Platform::PlaySound(Sounds::Pickup);
		hp = maxHP;
		mana = maxMana;
		reloadTime = 0;
		TombGame::ShowMessage(PSTR("Sun idol floods you with power"));
		TombGame::stats.idolsCollected++;
		break;
	case CellType::RuneTablet:
		Map::SetCell(cellX, cellY, CellType::Empty);
		Platform::PlaySound(Sounds::Pickup);
		if (mana + runeTabletMana > maxMana)
			mana = maxMana;
		else
			mana += runeTabletMana;
		runeEmpowerTime = runeEmpowerDuration;
		TombGame::ShowMessage(PSTR("Rune tablet empowers your staff"));
		TombGame::stats.tabletsCollected++;
		break;
	}

	lastInput = input;
}

bool Player::IsWorldColliding() const
{
	return Map::IsBlockedAtWorldPosition(x - collisionSize, y - collisionSize)
		|| Map::IsBlockedAtWorldPosition(x + collisionSize, y - collisionSize)
		|| Map::IsBlockedAtWorldPosition(x + collisionSize, y + collisionSize)
		|| Map::IsBlockedAtWorldPosition(x - collisionSize, y + collisionSize);
}

bool Player::CheckCollisions(bool confirmPressed)
{
	int16_t lookAheadX = (x + (FixedCos(angle) * lookAheadDistance) / FIXED_ONE);
	int16_t lookAheadY = (y + (FixedSin(angle) * lookAheadDistance) / FIXED_ONE);
	uint8_t lookAheadCellX = (uint8_t)(lookAheadX / CELL_SIZE);
	uint8_t lookAheadCellY = (uint8_t)(lookAheadY / CELL_SIZE);

	CellType lookAheadCell = Map::GetCellSafe(lookAheadCellX, lookAheadCellY);
	switch (lookAheadCell)
	{
	case CellType::RelicCasket:
		if (confirmPressed)
		{
			Map::SetCell(lookAheadCellX, lookAheadCellY, CellType::OpenedCasket);
			ParticleSystemManager::CreateExplosion(lookAheadX, lookAheadY, true);
			Platform::PlaySound(Sounds::Pickup);
			TombGame::ShowMessage(PSTR("Unsealed a relic casket"));
			TombGame::stats.casketsOpened++;
		}
		break;
	case CellType::StoneTablet:
		TombGame::ShowMessage(SignMessage1);
		break;
	}

	if (IsWorldColliding())
	{
		return true;
	}

	if (EnemyManager::GetOverlappingEnemy(*this))
	{
		return true;
	}

	return false;
}

void Player::Move(int16_t deltaX, int16_t deltaY, bool confirmPressed)
{
	x += deltaX;
	y += deltaY;

	if (CheckCollisions(confirmPressed))
	{
		y -= deltaY;
		if (CheckCollisions(confirmPressed))
		{
			x -= deltaX;
			y += deltaY;

			if (CheckCollisions(confirmPressed))
			{
				y -= deltaY;
			}
		}
	}
}

void Player::Damage(uint8_t damageAmount)
{
	if(shakeTime < 6)
		shakeTime = 6;

	damageTime = 8;

	Platform::PlaySound(Sounds::Ouch);
	if (damageAmount >= hp)
	{
		hp = 0;
	}
	else
	{
		hp -= damageAmount;
	}
}


