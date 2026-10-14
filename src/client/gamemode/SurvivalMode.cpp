#include "SurvivalMode.h"
#include "../Minecraft.h"
#include "../player/LocalPlayer.h"
#ifndef STANDALONE_SERVER
#include "../particle/ParticleEngine.h"
#include "../sound/SoundEngine.h"
#endif
#include "../../world/level/Level.h"
#include "../../world/entity/player/Abilities.h"

#include <queue>
#include <set>
#include <cstdlib>

// ============================================================================
// Tree Capitator & Vein Miner Logic
// ============================================================================

struct BlockCoord {
	int x, y, z;
	bool operator<(const BlockCoord& o) const {
		if (x != o.x) return x < o.x;
		if (y != o.y) return y < o.y;
		return z < o.z;
	}
};

struct LeafNode {
	BlockCoord pos;
	int dist;
};

static bool isAxeTool(int id) {
	// Wood(271), Stone(275), Iron(258), Diamond(279), Gold(286)
	return (id == 271 || id == 275 || id == 258 || id == 279 || id == 286);
}

static bool isPickaxeTool(int id) {
	// Wood(270), Stone(274), Iron(257), Diamond(278), Gold(285)
	return (id == 270 || id == 274 || id == 257 || id == 278 || id == 285);
}

static bool isOreTile(int id) {
	return (id == 14 || // Gold Ore
	        id == 15 || // Iron Ore
	        id == 16 || // Coal Ore
	        id == 21 || // Lapis Ore
	        id == 56 || // Diamond Ore
	        id == 73 || id == 74); // Redstone Ore (unlit / lit)
}

// ---------------- 1. Tree Capitator ----------------
static bool processTreeChop(Minecraft* mc, int startX, int startY, int startZ, BlockCoord& outReplantPos, int& outSaplingMeta) {
	Level* level = mc->level;
	LocalPlayer* player = mc->player;

	// 1. Trace down to find the tree's base
	int lowestY = startY;
	while (lowestY > 0 && level->getTile(startX, lowestY - 1, startZ) == 17) {
		lowestY--;
	}

	// Base block must sit on Dirt (3) or Grass (2) to protect player builds
	int groundId = level->getTile(startX, lowestY - 1, startZ);
	if (groundId != 2 && groundId != 3) {
		return false; // Player building or floating wood
	}

	// 2. BFS to find all connected logs in the trunk and branches
	std::set<BlockCoord> logs;
	std::queue<BlockCoord> queue;

	BlockCoord root = {startX, lowestY, startZ};
	queue.push(root);
	logs.insert(root);

	const int MAX_LOGS = 128;

	while (!queue.empty() && (int)logs.size() < MAX_LOGS) {
		BlockCoord cur = queue.front();
		queue.pop();

		for (int dx = -1; dx <= 1; dx++) {
			for (int dy = -1; dy <= 1; dy++) {
				for (int dz = -1; dz <= 1; dz++) {
					if (dx == 0 && dy == 0 && dz == 0) continue;
					BlockCoord next = {cur.x + dx, cur.y + dy, cur.z + dz};

					int tile = level->getTile(next.x, next.y, next.z);
					if (tile == 17) { // 17 = Log
						if (logs.find(next) == logs.end()) {
							logs.insert(next);
							queue.push(next);
						}
					}
				}
			}
		}
	}

	// 3. Flood-fill all connected leaves (BFS up to 5 blocks from any log)
	std::set<BlockCoord> leaves;
	std::queue<LeafNode> leafQueue;

	for (std::set<BlockCoord>::iterator logIt = logs.begin(); logIt != logs.end(); ++logIt) {
		for (int dx = -1; dx <= 1; dx++) {
			for (int dy = -1; dy <= 1; dy++) {
				for (int dz = -1; dz <= 1; dz++) {
					BlockCoord neighbor = {logIt->x + dx, logIt->y + dy, logIt->z + dz};
					if (level->getTile(neighbor.x, neighbor.y, neighbor.z) == 18) { // 18 = Leaves
						if (leaves.find(neighbor) == leaves.end()) {
							leaves.insert(neighbor);
							LeafNode node = {neighbor, 1};
							leafQueue.push(node);
						}
					}
				}
			}
		}
	}

	const int MAX_LEAF_DIST = 5;
	const int MAX_LEAVES = 500;

	while (!leafQueue.empty() && (int)leaves.size() < MAX_LEAVES) {
		LeafNode cur = leafQueue.front();
		leafQueue.pop();

		if (cur.dist >= MAX_LEAF_DIST) continue;

		for (int dx = -1; dx <= 1; dx++) {
			for (int dy = -1; dy <= 1; dy++) {
				for (int dz = -1; dz <= 1; dz++) {
					if (dx == 0 && dy == 0 && dz == 0) continue;
					BlockCoord next = {cur.pos.x + dx, cur.pos.y + dy, cur.pos.z + dz};

					if (level->getTile(next.x, next.y, next.z) == 18) {
						if (leaves.find(next) == leaves.end()) {
							leaves.insert(next);
							LeafNode nextNode = {next, cur.dist + 1};
							leafQueue.push(nextNode);
						}
					}
				}
			}
		}
	}

	// Must have at least 6 leaves attached to count as a natural tree
	if (leaves.size() < 6) {
		return false;
	}

	ItemInstance* item = player->inventory->getSelected();
	int woodMeta = level->getData(startX, startY, startZ) & 0x3;
	bool saplingDropped = false;

	// Break all connected logs (except the one clicked, handled by destroyBlock)
	for (std::set<BlockCoord>::iterator it = logs.begin(); it != logs.end(); ++it) {
		if (it->x == startX && it->y == startY && it->z == startZ) {
			continue;
		}

		int t = level->getTile(it->x, it->y, it->z);
		if (t == 17) {
			int data = level->getData(it->x, it->y, it->z);
			Tile::tiles[t]->playerDestroy(level, player, it->x, it->y, it->z, data);
			level->setTile(it->x, it->y, it->z, 0);

			if (item != NULL) {
				item->mineBlock(t, it->x, it->y, it->z);
				if (item->count == 0) {
					player->inventory->clearSlot(player->inventory->selected);
					item = NULL;
					break;
				}
			}
		}
	}

	// Break all canopy leaves
	for (std::set<BlockCoord>::iterator it = leaves.begin(); it != leaves.end(); ++it) {
		int t = level->getTile(it->x, it->y, it->z);
		if (t == 18) {
			int data = level->getData(it->x, it->y, it->z);
			if ((std::rand() % 20) == 0) {
				saplingDropped = true;
			} else {
				Tile::tiles[t]->playerDestroy(level, player, it->x, it->y, it->z, data);
			}
			level->setTile(it->x, it->y, it->z, 0);
		}
	}

	// Guarantee at least one sapling replants
	if (!leaves.empty()) {
		saplingDropped = true;
	}

	if (saplingDropped) {
		outReplantPos.x = startX;
		outReplantPos.y = lowestY;
		outReplantPos.z = startZ;
		outSaplingMeta = woodMeta;
	}

	return true;
}

// ---------------- 2. Vein Miner ----------------
static void processVeinMine(Minecraft* mc, int startX, int startY, int startZ, int oreId) {
	Level* level = mc->level;
	LocalPlayer* player = mc->player;
	ItemInstance* item = player->inventory->getSelected();
	if (item == NULL) return;

	int matchId = (oreId == 74) ? 73 : oreId;

	std::set<BlockCoord> visited;
	std::queue<BlockCoord> queue;

	BlockCoord start = {startX, startY, startZ};
	visited.insert(start);
	queue.push(start);

	const int MAX_VEIN = 32;

	while (!queue.empty() && (int)visited.size() <= MAX_VEIN) {
		BlockCoord cur = queue.front();
		queue.pop();

		if (!(cur.x == startX && cur.y == startY && cur.z == startZ)) {
			int t = level->getTile(cur.x, cur.y, cur.z);
			int normT = (t == 74) ? 73 : t;

			if (normT == matchId) {
				int data = level->getData(cur.x, cur.y, cur.z);
				Tile::tiles[t]->playerDestroy(level, player, cur.x, cur.y, cur.z, data);
				level->setTile(cur.x, cur.y, cur.z, 0);

				if (item != NULL) {
					item->mineBlock(t, cur.x, cur.y, cur.z);
					if (item->count == 0) {
						player->inventory->clearSlot(player->inventory->selected);
						break;
					}
				}
			}
		}

		for (int dx = -1; dx <= 1; dx++) {
			for (int dy = -1; dy <= 1; dy++) {
				for (int dz = -1; dz <= 1; dz++) {
					if (dx == 0 && dy == 0 && dz == 0) continue;
					BlockCoord next = {cur.x + dx, cur.y + dy, cur.z + dz};

					if (visited.find(next) == visited.end()) {
						int nextTile = level->getTile(next.x, next.y, next.z);
						int nextNorm = (nextTile == 74) ? 73 : nextTile;
						if (nextNorm == matchId) {
							visited.insert(next);
							queue.push(next);
						}
					}
				}
			}
		}
	}
}

// ============================================================================

SurvivalMode::SurvivalMode( Minecraft* minecraft )
:	super(minecraft),
	xDestroyBlock(-1),
	yDestroyBlock(-1),
	zDestroyBlock(-1)
{
}

void SurvivalMode::continueDestroyBlock( int x, int y, int z, int face ) {
	if (destroyDelay > 0) {
		destroyDelay--;
		return;
	}

	if (x == xDestroyBlock && y == yDestroyBlock && z == zDestroyBlock) {
		int t = minecraft->level->getTile(x, y, z);
		if (t == 0) return;
		Tile* tile = Tile::tiles[t];

		destroyProgress += tile->getDestroyProgress(minecraft->player);

		if ((++destroyTicks & 3) == 1) {
#ifndef STANDALONE_SERVER
			if (tile != NULL) {
				minecraft->soundEngine->play(tile->soundType->getStepSound(), x + 0.5f, y + 0.5f, z + 0.5f, (tile->soundType->getVolume() + 1) / 8, tile->soundType->getPitch() * 0.5f);
			}
#endif
		}

		if (destroyProgress >= 1) {
			destroyBlock(x, y, z, face);
			destroyProgress = 0;
			oDestroyProgress = 0;
			destroyTicks = 0;
			destroyDelay = 5;
		}
	} else {
		destroyProgress = 0;
		oDestroyProgress = 0;
		destroyTicks = 0;
		xDestroyBlock = x;
		yDestroyBlock = y;
		zDestroyBlock = z;
	}
}

bool SurvivalMode::destroyBlock( int x, int y, int z, int face ) {
	int t = minecraft->level->getTile(x, y, z);
	int data = minecraft->level->getData(x, y, z);

	BlockCoord replantPos = {0, -1, 0};
	int saplingMeta = 0;
	ItemInstance* heldItem = minecraft->player->inventory->getSelected();

	if (heldItem != NULL) {
		if (t == 17 && isAxeTool(heldItem->id)) {
			processTreeChop(minecraft, x, y, z, replantPos, saplingMeta);
		} else if (isOreTile(t) && isPickaxeTool(heldItem->id)) {
			processVeinMine(minecraft, x, y, z, t);
		}
	}

	bool changed = GameMode::destroyBlock(x, y, z, face);
	bool couldDestroy = minecraft->player->canDestroy(Tile::tiles[t]);

	ItemInstance* item = minecraft->player->inventory->getSelected();
	if (item != NULL) {
		item->mineBlock(t, x, y, z);
		if (item->count == 0) {
			minecraft->player->inventory->clearSlot(minecraft->player->inventory->selected);
		}
	}
	
	if (changed && couldDestroy) {
		ItemInstance instance(t, 1, data);
		Tile::tiles[t]->playerDestroy(minecraft->level, minecraft->player, x, y, z, data);

		// Auto-replant at the original base if leaves were broken
		if (replantPos.y >= 0) {
			int ground = minecraft->level->getTile(replantPos.x, replantPos.y - 1, replantPos.z);
			if (ground == 2 || ground == 3) {
				minecraft->level->setTileAndData(replantPos.x, replantPos.y, replantPos.z, 6, saplingMeta);
			}
		}
	}
	return changed;
}

void SurvivalMode::stopDestroyBlock() {
	destroyProgress = 0;
	destroyDelay = 0;
}

void SurvivalMode::initAbilities( Abilities& abilities ) {
	abilities.flying = false;
	abilities.mayfly = false;
	abilities.instabuild = false;
	abilities.invulnerable = false;
}

void SurvivalMode::startDestroyBlock( int x, int y, int z, int face ) {
	if(minecraft->player->getCarriedItem() != NULL && minecraft->player->getCarriedItem()->id == Item::bow->id)
		return;

	int t = minecraft->level->getTile(x, y, z);
	if (t > 0 && destroyProgress == 0) Tile::tiles[t]->attack(minecraft->level, x, y, z, minecraft->player);
	if (t > 0 && Tile::tiles[t]->getDestroyProgress(minecraft->player) >= 1)
		destroyBlock(x, y, z, face);
}