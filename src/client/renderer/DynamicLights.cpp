#include "DynamicLights.h"
#include "../Minecraft.h"
#include "../../world/level/Level.h"
#include "../../world/level/LightLayer.h"
#include "../../world/level/tile/Tile.h"
#include "../../world/item/Item.h"
#include "../../world/item/ItemInstance.h"
#include "../../world/entity/player/Player.h"
#include "../player/LocalPlayer.h"
#include "../../util/Mth.h"

#include <vector>
#include <queue>
#include <set>

struct LitBlock {
	int x, y, z;
	unsigned char prevLight;
};

struct LightNode {
	int x, y, z;
	int light;
};

static std::vector<LitBlock> sLitBlocks;
static int sLastX = -999999;
static int sLastY = -999999;
static int sLastZ = -999999;
static int sLastLightVal = 0;

static int getLightEmission(ItemInstance* item, Player* player) {
	int light = 0;

	// 1. If player is burning on fire
	if (player && player->onFire > 0) {
		light = 12;
	}

	if (!item || item->isNull()) {
		return light;
	}

	int id = item->id;

	// Torch (ID 50)
	if (Tile::torch && id == Tile::torch->id) {
		light = Mth::Max(light, 14);
	}
	// Glowstone (ID 89)
	else if (Tile::lightGem && id == Tile::lightGem->id) {
		light = Mth::Max(light, 15);
	}
	// Jack o'Lantern / Lit Pumpkin (ID 91)
	else if (Tile::litPumpkin && id == Tile::litPumpkin->id) {
		light = Mth::Max(light, 15);
	}
	// Lava Bucket (ID 327 or bucket with lava aux)
	else if (Item::bucket && id == Item::bucket->id && item->getAuxValue() == 10) {
		light = Mth::Max(light, 15);
	}
	// Redstone Torch (ID 76)
	else if (id == 76) {
		light = Mth::Max(light, 9);
	}

	return light;
}

void DynamicLights::clear(Level* level) {
	if (!level || sLitBlocks.empty()) return;

	for (size_t i = 0; i < sLitBlocks.size(); i++) {
		const LitBlock& b = sLitBlocks[i];
		level->setBrightness(LightLayer::Block, b.x, b.y, b.z, b.prevLight);
	}

	sLitBlocks.clear();
	sLastX = sLastY = sLastZ = -999999;
	sLastLightVal = 0;
}

void DynamicLights::update(Minecraft* mc) {
	if (!mc || !mc->level || !mc->player) return;

	Level* level = mc->level;
	LocalPlayer* player = mc->player;

	int currentLight = getLightEmission(player->getCarriedItem(), player);

	int px = Mth::floor(player->x);
	int py = Mth::floor(player->y + player->heightOffset);
	int pz = Mth::floor(player->z);

	// If player hasn't changed blocks and light level is identical, do nothing! (0% CPU cost)
	if (px == sLastX && py == sLastY && pz == sLastZ && currentLight == sLastLightVal) {
		return;
	}

	// 1. Restore all previously lit blocks to their true original light
	if (!sLitBlocks.empty()) {
		for (size_t i = 0; i < sLitBlocks.size(); i++) {
			const LitBlock& b = sLitBlocks[i];
			level->setBrightness(LightLayer::Block, b.x, b.y, b.z, b.prevLight);
		}

		// Mark old region dirty so it dims smoothly
		int r = sLastLightVal;
		level->setTilesDirty(sLastX - r, sLastY - r, sLastZ - r, sLastX + r, sLastY + r, sLastZ + r);
		sLitBlocks.clear();
	}

	sLastX = px;
	sLastY = py;
	sLastZ = pz;
	sLastLightVal = currentLight;

	if (currentLight <= 0) {
		return;
	}

	// 2. Fast BFS Flood-Fill to propagate dynamic light realistically through open air
	std::queue<LightNode> queue;
	std::set<long long> visited;

	queue.push({px, py, pz, currentLight});
	visited.insert(((long long)px & 0x3FFFFFF) | (((long long)pz & 0x3FFFFFF) << 26) | (((long long)py & 0xFFF) << 52));

	while (!queue.empty()) {
		LightNode cur = queue.front();
		queue.pop();

		if (cur.light <= 0) continue;

		int origLight = level->getBrightness(LightLayer::Block, cur.x, cur.y, cur.z);

		// Only brighten if our dynamic light exceeds the existing light
		if (cur.light > origLight) {
			sLitBlocks.push_back({cur.x, cur.y, cur.z, (unsigned char)origLight});
			level->setBrightness(LightLayer::Block, cur.x, cur.y, cur.z, cur.light);
		}

		// Spread to 6 adjacent neighbors
		static const int dx[] = {1, -1, 0, 0, 0, 0};
		static const int dy[] = {0, 0, 1, -1, 0, 0};
		static const int dz[] = {0, 0, 0, 0, 1, -1};

		for (int f = 0; f < 6; f++) {
			int nx = cur.x + dx[f];
			int ny = cur.y + dy[f];
			int nz = cur.z + dz[f];

			if (ny < 0 || ny >= Level::DEPTH) continue;

			// Stop light at solid blocks (light cannot leak through walls!)
			if (level->isSolidBlockingTile(nx, ny, nz)) {
				continue;
			}

			long long hash = ((long long)nx & 0x3FFFFFF) | (((long long)nz & 0x3FFFFFF) << 26) | (((long long)ny & 0xFFF) << 52);
			if (visited.find(hash) == visited.end()) {
				visited.insert(hash);
				queue.push({nx, ny, nz, cur.light - 1});
			}
		}
	}

	// 3. Mark surrounding chunk meshes dirty for immediate re-tesselation
	level->setTilesDirty(px - currentLight, py - currentLight, pz - currentLight,
	                     px + currentLight, py + currentLight, pz + currentLight);
}
