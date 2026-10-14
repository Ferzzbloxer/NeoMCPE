#include "PortalForcer.h"
#include "Level.h"
#include "tile/Tile.h"
#include "tile/PortalTile.h"
#include "../entity/Entity.h"
#include "../phys/Vec3.h"

bool PortalForcer::findOrCreatePortal(Level* level, Entity* entity, float targetX, float targetY, float targetZ) {
	int ix = (int)targetX;
	int iz = (int)targetZ;

	// Clamp within finite world borders
	if (ix < 10) ix = 10;
	if (ix > 245) ix = 245;
	if (iz < 10) iz = 10;
	if (iz > 245) iz = 245;

	// 1. Search for an existing portal within a 16-block radius
	int bestDistSq = 999999;
	int foundX = -1, foundY = -1, foundZ = -1;

	for (int x = ix - 16; x <= ix + 16; x++) {
		for (int z = iz - 16; z <= iz + 16; z++) {
			for (int y = 10; y < 118; y++) {
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

	if (foundX != -1) {
		// Found existing portal: position player inside
		entity->moveTo(foundX + 0.5f, foundY, foundZ + 0.5f, entity->yRot, entity->xRot);
		return true;
	}

	// 2. No portal found: Build a new 4x5 Obsidian Portal Frame
	int buildY = 64;
	if (level->dimension->id == 1) { // Nether
		buildY = 40; // Safely in Nether cavern, above lava ocean (y=31)
	} else {
		buildY = level->getTopSolidBlock(ix, iz);
		if (buildY <= 0 || buildY > 115) buildY = 64;
	}

	int bx = ix;
	int by = buildY;
	int bz = iz;

	// Base & Top (Width 4)
	for (int w = 0; w < 4; w++) {
		level->setTile(bx + w, by, bz, Tile::obsidian->id);
		level->setTile(bx + w, by + 4, bz, Tile::obsidian->id);
	}

	// Sides (Height 5)
	for (int h = 1; h <= 3; h++) {
		level->setTile(bx, by + h, bz, Tile::obsidian->id);
		level->setTile(bx + 3, by + h, bz, Tile::obsidian->id);

		// Inside Portal Blocks (Axis X)
		level->setTileAndData(bx + 1, by + h, bz, Tile::portalTile->id, PortalTile::AXIS_X);
		level->setTileAndData(bx + 2, by + h, bz, Tile::portalTile->id, PortalTile::AXIS_X);

		// Clear clearance space in front and behind
		level->setTile(bx + 1, by + h, bz - 1, 0);
		level->setTile(bx + 2, by + h, bz - 1, 0);
		level->setTile(bx + 1, by + h, bz + 1, 0);
		level->setTile(bx + 2, by + h, bz + 1, 0);
	}

	// Teleport player into the newly built portal
	entity->moveTo(bx + 1.5f, by + 1.0f, bz + 0.5f, entity->yRot, entity->xRot);
	return true;
}