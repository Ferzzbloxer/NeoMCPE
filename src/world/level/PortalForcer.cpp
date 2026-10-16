#include "PortalForcer.h"
#include "Level.h"
#include "tile/Tile.h"
#include "tile/PortalTile.h"
#include "../entity/Entity.h"
#include "../phys/Vec3.h"

bool PortalForcer::findOrCreatePortal(Level* level, Entity* entity, float targetX, float targetY, float targetZ) {
	int ix = (int)targetX;
	int iz = (int)targetZ;

	// Clamp within 0.6.1 world boundaries (256x256)
	if (ix < 30) ix = 30;
	if (ix > 220) ix = 220;
	if (iz < 30) iz = 30;
	if (iz > 220) iz = 220;

	// In the Nether, spawn at safe cavern height
	int by = (level->dimension->id == 1) ? 45 : 64;
	int bx = ix;
	int bz = iz;

	// Build 4x5 Obsidian Frame
	for (int w = 0; w < 4; w++) {
		level->setTile(bx + w, by, bz, Tile::obsidian->id);
		level->setTile(bx + w, by + 4, bz, Tile::obsidian->id);
	}

	for (int h = 1; h <= 3; h++) {
		level->setTile(bx, by + h, bz, Tile::obsidian->id);
		level->setTile(bx + 3, by + h, bz, Tile::obsidian->id);

		level->setTileAndData(bx + 1, by + h, bz, Tile::portalTile->id, PortalTile::AXIS_X);
		level->setTileAndData(bx + 2, by + h, bz, Tile::portalTile->id, PortalTile::AXIS_X);

		// Clear walking space in front and behind
		level->setTile(bx + 1, by + h, bz - 1, 0);
		level->setTile(bx + 2, by + h, bz - 1, 0);
		level->setTile(bx + 1, by + h, bz + 1, 0);
		level->setTile(bx + 2, by + h, bz + 1, 0);
	}

	entity->moveTo((float)bx + 1.5f, (float)by + 1.0f, (float)bz + 0.5f, entity->yRot, entity->xRot);
	return true;
}