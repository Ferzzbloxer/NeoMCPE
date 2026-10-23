#include "PortalForcer.h"
#include "Level.h"
#include "tile/Tile.h"
#include "tile/PortalTile.h"
#include "../entity/Entity.h"
#include "../entity/player/Player.h"
#include "../phys/Vec3.h"

bool PortalForcer::findOrCreatePortal(Level* level, Entity* entity, float targetX, float targetY, float targetZ) {
	int ix = (int)targetX;
	int iz = (int)targetZ;

	// Clamp safely inside 1024x1024 world boundaries (30..990)
	if (ix < 30) ix = 30;
	if (ix > 990) ix = 990;
	if (iz < 30) iz = 30;
	if (iz > 990) iz = 990;

	float eyeOffset = 1.62f;
	Player* p = Player::asPlayer(entity);
	if (p != NULL) {
		eyeOffset = p->heightOffset;
	}

	// ==========================================================
	// 1. Search for an EXISTING portal nearby (16-block radius)
	// ==========================================================
	int bestDistSq = 999999;
	int foundX = -1, foundY = -1, foundZ = -1;

	// Step by 1 so we never miss a portal block
	for (int x = ix - 16; x <= ix + 16; x++) {
		for (int z = iz - 16; z <= iz + 16; z++) {
			for (int y = 10; y < 118; y++) {
				if (level->getTile(x, y, z) == Tile::portalTile->id) {
					int dist = (x - ix) * (x - ix) + (y - (int)targetY) * (y - (int)targetY) + (z - iz) * (z - iz);
					if (dist < bestDistSq) {
						bestDistSq = dist;
						foundX = x;
						foundY = y;
						foundZ = z;
					}
				}
			}
		}
	}

	// If existing portal found, place player safely in front of it!
	if (foundX != -1) {
		// Ensure we spawn at the bottom row of the portal so we don't spawn floating
		while (level->getTile(foundX, foundY - 1, foundZ) == Tile::portalTile->id) {
			foundY--;
		}

		entity->moveTo((float)foundX + 0.5f, (float)foundY + eyeOffset, (float)foundZ + 2.5f, entity->yRot, entity->xRot);
		entity->xd = entity->yd = entity->zd = 0.0f;
		return true; // Stop here! Don't build a new one.
	}


	// ==========================================================
	// 2. No portal found: Build a NEW one
	// ==========================================================
	
	// Find a good cavern floor height
	int by = 45;
	if (level->dimension->id == 1) {
		for (int testY = 35; testY < 85; testY++) {
			if (level->getTile(ix, testY, iz) == 0 && level->getTile(ix, testY + 1, iz) == 0) {
				by = testY - 1;
				break;
			}
		}
	} else {
		by = level->getTopSolidBlock(ix, iz);
		if (by <= 0 || by > 115) by = 64;
	}

	int bx = ix;
	int bz = iz;

	// Carve walking room of air and build stone floor platform
	for (int dx = -2; dx <= 5; dx++) {
		for (int dz = -3; dz <= 5; dz++) {
			for (int dy = 1; dy <= 4; dy++) {
				level->setTile(bx + dx, by + dy, bz + dz, 0); // Air
			}
			if (level->getTile(bx + dx, by, bz + dz) == 0) {
				level->setTile(bx + dx, by, bz + dz, Tile::netherrack ? Tile::netherrack->id : Tile::obsidian->id);
			}
		}
	}

	// Build 4x5 Obsidian Frame
	for (int w = 0; w < 4; w++) {
		level->setTile(bx + w, by, bz, Tile::obsidian->id);
		level->setTile(bx + w, by + 4, bz, Tile::obsidian->id);
	}

	for (int h = 1; h <= 3; h++) {
		level->setTile(bx, by + h, bz, Tile::obsidian->id);
		level->setTile(bx + 3, by + h, bz, Tile::obsidian->id);

		level->setTileAndDataNoUpdate(bx + 1, by + h, bz, Tile::portalTile->id, PortalTile::AXIS_X);
		level->setTileAndDataNoUpdate(bx + 2, by + h, bz, Tile::portalTile->id, PortalTile::AXIS_X);
		level->sendTileUpdated(bx + 1, by + h, bz);
		level->sendTileUpdated(bx + 2, by + h, bz);
	}

	// Teleport player safely ON TOP of the floor and 2.5 blocks in front
	entity->moveTo((float)bx + 1.5f, (float)by + 1.0f + eyeOffset, (float)bz + 2.5f, entity->yRot, entity->xRot);
	entity->xd = 0.0f;
	entity->yd = 0.0f;
	entity->zd = 0.0f;

	return true;
}