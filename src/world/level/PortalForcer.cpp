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

	// Clamp safely inside the 256x256 finite world (away from invisible bedrock borders)
	if (ix < 30) ix = 30;
	if (ix > 220) ix = 220;
	if (iz < 30) iz = 30;
	if (iz > 220) iz = 220;

	// 1. Search for an EXISTING portal nearby (16-block radius)
	int bestDistSq = 999999;
	int foundX = -1, foundY = -1, foundZ = -1;

	for (int x = ix - 16; x <= ix + 16; x += 2) {
		for (int z = iz - 16; z <= iz + 16; z += 2) {
			for (int y = 20; y < 100; y += 2) {
				if (level->getTile(x, y, z) == Tile::portalTile->id) {
					int dist = (x - ix) * (x - ix) + (z - iz) * (z - iz);
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

	float eyeOffset = 1.62f;
	Player* p = Player::asPlayer(entity);
	if (p != NULL) {
		eyeOffset = p->heightOffset;
	}

	// If existing portal found, place player safely in front of it!
	if (foundX != -1) {
		entity->moveTo((float)foundX + 0.5f, (float)foundY + eyeOffset, (float)foundZ + 2.5f, entity->yRot, entity->xRot);
		entity->xd = entity->yd = entity->zd = 0.0f;
		return true;
	}

	// 2. No portal found: Build a new one
	int by = 45;
	if (level->dimension->id == 1) {
		// Nether cavern floor
		for (int testY = 35; testY < 85; testY++) {
			if (level->getTile(ix, testY, iz) == 0 && level->getTile(ix, testY + 1, iz) == 0) {
				by = testY - 1;
				break;
			}
		}
	} else {
		// Overworld ground
		by = level->getTopSolidBlock(ix, iz);
		if (by <= 0 || by > 115) by = 64;
	}

	int bx = ix;
	int bz = iz;

	// Carve a safe walking room of air & solid floor
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

	// Teleport player safely 2.5 blocks in front of the new portal
	entity->moveTo((float)bx + 1.5f, (float)by + 1.0f + eyeOffset, (float)bz + 2.5f, entity->yRot, entity->xRot);
	entity->xd = entity->yd = entity->zd = 0.0f;

	return true;
}
