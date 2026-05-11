#include "Defines.h"
#include "Map.h"
#include "TombGame.h"
#include "FixedMath.h"
#include "SceneRenderer.h"
#include "Platform.h"
#include "Enemy.h"

namespace
{
	constexpr uint8_t kMinimapWallTint = 9;
	constexpr uint8_t kMinimapPathTint = 6;
	constexpr uint8_t kMinimapGateTint = 5;
	constexpr uint8_t kMinimapChestTint = 11;
	constexpr uint8_t kMinimapJarTint = 12;
	constexpr uint8_t kMinimapItemTint = 13;
	constexpr uint8_t kMinimapAccentTint = 5;
	constexpr uint8_t kMinimapPlayerTint = 3;
	constexpr uint8_t kMinimapPlayerCoreTint = 2;
	constexpr uint8_t kMinimapBorderTint = 10;
	constexpr uint8_t kMinimapFallbackTint = 8;

	void PlotMinimapPixel(uint8_t x, uint8_t y, uint8_t colour, uint8_t tint = 0)
	{
		Platform::PutPixel(x, y, colour);
		if (colour == COLOUR_WHITE)
		{
			Platform::SetPixelTint(x, y, tint != 0 ? tint : kMinimapWallTint);
		}
		else
		{
			Platform::SetPixelTint(x, y, 0);
		}
	}

	void FillMinimapRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t colour, uint8_t tint = 0)
	{
		for (uint8_t py = 0; py < h; py++)
		{
			for (uint8_t px = 0; px < w; px++)
			{
				PlotMinimapPixel((uint8_t)(x + px), (uint8_t)(y + py), colour, tint);
			}
		}
	}

	void FillMinimapTile(uint8_t outX, uint8_t outY, uint8_t tint)
	{
		FillMinimapRect(outX, outY, 2, 2, COLOUR_WHITE, tint);
	}

	void DrawMinimapPlayer(uint8_t outX, uint8_t outY)
	{
		FillMinimapRect((uint8_t)(outX - 1), outY, 4, 2, COLOUR_WHITE, kMinimapPlayerTint);
		FillMinimapRect(outX, (uint8_t)(outY - 1), 2, 4, COLOUR_WHITE, kMinimapPlayerTint);
		FillMinimapRect(outX, outY, 2, 2, COLOUR_WHITE, kMinimapPlayerCoreTint);
	}

	void DrawMinimapCell(uint8_t outX, uint8_t outY, CellType cellType)
	{
		switch (cellType)
		{
		case CellType::TombWall:
			FillMinimapTile(outX, outY, kMinimapWallTint);
			break;
		case CellType::TombGate:
			FillMinimapTile(outX, outY, kMinimapGateTint);
			break;
		case CellType::CanopicJar:
			FillMinimapTile(outX, outY, kMinimapPathTint);
			FillMinimapRect((uint8_t)(outX + 1), outY, 1, 2, COLOUR_WHITE, kMinimapJarTint);
			break;
		case CellType::RelicCasket:
			FillMinimapTile(outX, outY, kMinimapPathTint);
			FillMinimapRect(outX, outY, 2, 1, COLOUR_WHITE, kMinimapChestTint);
			PlotMinimapPixel(outX, (uint8_t)(outY + 1), COLOUR_WHITE, kMinimapChestTint);
			PlotMinimapPixel((uint8_t)(outX + 1), (uint8_t)(outY + 1), COLOUR_WHITE, kMinimapAccentTint);
			break;
		case CellType::OpenedCasket:
			FillMinimapTile(outX, outY, kMinimapPathTint);
			PlotMinimapPixel(outX, outY, COLOUR_WHITE, kMinimapChestTint);
			PlotMinimapPixel((uint8_t)(outX + 1), outY, COLOUR_WHITE, kMinimapChestTint);
			PlotMinimapPixel(outX, (uint8_t)(outY + 1), COLOUR_WHITE, kMinimapAccentTint);
			break;
		case CellType::LifeEssence:
		case CellType::RelicShard:
		case CellType::SunIdol:
		case CellType::RuneTablet:
			FillMinimapTile(outX, outY, kMinimapPathTint);
			PlotMinimapPixel(outX, outY, COLOUR_WHITE, kMinimapItemTint);
			PlotMinimapPixel((uint8_t)(outX + 1), outY, COLOUR_WHITE, kMinimapAccentTint);
			PlotMinimapPixel((uint8_t)(outX + 1), (uint8_t)(outY + 1), COLOUR_WHITE, kMinimapItemTint);
			break;
		case CellType::Empty:
		case CellType::Brazier:
		case CellType::EntrySeal:
		case CellType::StoneTablet:
		case CellType::Monster:
			FillMinimapTile(outX, outY, kMinimapPathTint);
			break;
		default:
			if (cellType >= CellType::FirstCollidableCell)
			{
				FillMinimapTile(outX, outY, kMinimapFallbackTint);
			}
			else
			{
				FillMinimapTile(outX, outY, kMinimapPathTint);
			}
			break;
		}
	}
}

uint8_t Map::level[Map::width * Map::height / 2];

bool Map::IsBlocked(uint8_t x, uint8_t y)
{
	return GetCellSafe(x, y) >= CellType::FirstCollidableCell;
}

bool Map::IsSolid(uint8_t x, uint8_t y)
{
	return GetCellSafe(x, y) >= CellType::FirstSolidCell;
}

CellType Map::GetCell(uint8_t x, uint8_t y) 
{
	int index = y * Map::width + x;
	uint8_t cellData = level[index / 2];
	
	if(index & 1)
	{
		return (CellType)(cellData >> 4);
	}
	else
	{
		return (CellType)(cellData & 0xf);
	}
}

CellType Map::GetCellSafe(uint8_t x, uint8_t y) 
{
	if(x >= Map::width || y >= Map::height)
		return CellType::TombWall;
	
	int index = y * Map::width + x;
	uint8_t cellData = level[index / 2];
	
	if(index & 1)
	{
		return (CellType)(cellData >> 4);
	}
	else
	{
		return (CellType)(cellData & 0xf);
	}
}

void Map::SetCell(uint8_t x, uint8_t y, CellType type)
{
	if (x >= Map::width || y >= Map::height)
	{
		return;
	}

	int index = (y * Map::width + x) / 2;
	uint8_t cellType = (uint8_t)type;
	
	if(x & 1)
	{
		level[index] = (level[index] & 0xf) | (cellType << 4);
	}
	else
	{
		level[index] = (level[index] & 0xf0) | (cellType & 0xf);
	}
}

void Map::DebugDraw()
{
	for(int y = 0; y < Map::height; y++)
	{
		for(int x = 0; x < Map::width; x++)
		{
			Platform::PutPixel(x, y, GetCell(x, y) == CellType::TombWall ? 1 : 0);

			if (x == SceneRenderer::camera.cellX && y == SceneRenderer::camera.cellY && (TombGame::globalTickFrame & 8) != 0)
			{
				Platform::PutPixel(x, y, 1);
			}
		}
	}

	if ((TombGame::globalTickFrame & 2) != 0)
	{
		for (uint8_t n = 0; n < EnemyManager::maxEnemies; n++)
		{
			Enemy& enemy = EnemyManager::enemies[n];

			if (enemy.IsValid())
			{
				Platform::PutPixel(enemy.x / CELL_SIZE, enemy.y / CELL_SIZE, 1);
			}
		}
	}
}

bool Map::IsClearLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2)
{
	int cellX1 = x1 / CELL_SIZE;
	int cellX2 = x2 / CELL_SIZE;
	int cellY1 = y1 / CELL_SIZE;
	int cellY2 = y2 / CELL_SIZE;

    int xdist = ABS(cellX2 - cellX1);

	int partial, delta;
	int deltafrac;
	int xfrac, yfrac;
	int xstep, ystep;
	int32_t ltemp;
	int x, y;

    if (xdist > 0)
    {
        if (cellX2 > cellX1)
        {
            partial = (CELL_SIZE * (cellX1 + 1) - x1);
            xstep = 1;
        }
        else
        {
            partial = (x1 - CELL_SIZE * (cellX1));
            xstep = -1;
        }

        deltafrac = ABS(x2 - x1);
        delta = y2 - y1;
        ltemp = ((int32_t)delta * CELL_SIZE) / deltafrac;
        if (ltemp > 0x7fffl)
            ystep = 0x7fff;
        else if (ltemp < -0x7fffl)
            ystep = -0x7fff;
        else
            ystep = ltemp;
        yfrac = y1 + (((int32_t)ystep*partial) / CELL_SIZE);

        x = cellX1 + xstep;
        cellX2 += xstep;
        do
        {
            y = (yfrac) / CELL_SIZE;
            yfrac += ystep;

			if (IsSolid(x, y))
				return false;

            x += xstep;

            //
            // see if the door is open enough
            //
            /*value &= ~0x80;
            intercept = yfrac-ystep/2;

            if (intercept>doorposition[value])
                return false;*/

        } while (x != cellX2);
    }

    int ydist = ABS(cellY2 - cellY1);

    if (ydist > 0)
    {
        if (cellY2 > cellY1)
        {
            partial = (CELL_SIZE * (cellY1 + 1) - y1);
            ystep = 1;
        }
        else
        {
            partial = (y1 - CELL_SIZE * (cellY1));
            ystep = -1;
        }

        deltafrac = ABS(y2 - y1);
        delta = x2 - x1;
        ltemp = ((int32_t)delta * CELL_SIZE)/deltafrac;
        if (ltemp > 0x7fffl)
            xstep = 0x7fff;
        else if (ltemp < -0x7fffl)
            xstep = -0x7fff;
        else
            xstep = ltemp;
        xfrac = x1 + (((int32_t)xstep*partial) / CELL_SIZE);

        y = cellY1 + ystep;
        cellY2 += ystep;
        do
        {
            x = (xfrac) / CELL_SIZE;
            xfrac += xstep;

			if (IsSolid(x, y))
				return false;
            y += ystep;

            //
            // see if the door is open enough
            //
            /*value &= ~0x80;
            intercept = xfrac-xstep/2;

            if (intercept>doorposition[value])
                return false;*/
        } while (y != cellY2);
    }

    return true;
}

void Map::DrawMinimap()
{
	constexpr uint8_t viewportCellsX = 15;
	constexpr uint8_t viewportCellsY = 6;
	constexpr uint8_t tileSize = 2;
	constexpr uint8_t panelX = 84;
	constexpr uint8_t panelY = 0;
	constexpr uint8_t panelWidth = 38;
	constexpr uint8_t panelHeight = 16;
	constexpr uint8_t mapWidth = viewportCellsX * tileSize;
	constexpr uint8_t mapHeight = viewportCellsY * tileSize;
	constexpr uint8_t mapX = panelX + (panelWidth - mapWidth) / 2;
	constexpr uint8_t mapY = panelY + (panelHeight - mapHeight) / 2;

	const uint8_t playerCellX = TombGame::player.x / CELL_SIZE;
	const uint8_t playerCellY = TombGame::player.y / CELL_SIZE;
	const int16_t startCellX = (int16_t)playerCellX - viewportCellsX / 2;
	const int16_t startCellY = (int16_t)playerCellY - viewportCellsY / 2;
	FillMinimapRect(panelX, panelY, panelWidth, panelHeight, COLOUR_WHITE, kMinimapFallbackTint);

	for (uint8_t x = 0; x < panelWidth; x++)
	{
		PlotMinimapPixel((uint8_t)(panelX + x), panelY, COLOUR_WHITE, kMinimapBorderTint);
		PlotMinimapPixel((uint8_t)(panelX + x), (uint8_t)(panelY + panelHeight - 1), COLOUR_WHITE, kMinimapBorderTint);
	}

	for (uint8_t y = 0; y < panelHeight; y++)
	{
		PlotMinimapPixel(panelX, (uint8_t)(panelY + y), COLOUR_WHITE, kMinimapBorderTint);
		PlotMinimapPixel((uint8_t)(panelX + panelWidth - 1), (uint8_t)(panelY + y), COLOUR_WHITE, kMinimapBorderTint);
	}

	for (uint8_t cellY = 0; cellY < viewportCellsY; cellY++)
	{
		for (uint8_t cellX = 0; cellX < viewportCellsX; cellX++)
		{
			const int16_t levelCellX = startCellX + cellX;
			const int16_t levelCellY = startCellY + cellY;
			const uint8_t outX = (uint8_t)(mapX + cellX * tileSize);
			const uint8_t outY = (uint8_t)(mapY + cellY * tileSize);

			if (levelCellX < 0 || levelCellY < 0 || levelCellX >= width || levelCellY >= height)
			{
				FillMinimapTile(outX, outY, kMinimapWallTint);
				continue;
			}

			const uint8_t mapCellX = (uint8_t)levelCellX;
			const uint8_t mapCellY = (uint8_t)levelCellY;
			const CellType cellType = GetCell(mapCellX, mapCellY);

			DrawMinimapCell(outX, outY, cellType);
		}
	}

	const uint8_t playerX = (uint8_t)(mapX + (viewportCellsX / 2) * tileSize);
	const uint8_t playerY = (uint8_t)(mapY + (viewportCellsY / 2) * tileSize);
	DrawMinimapPlayer(playerX, playerY);
}


