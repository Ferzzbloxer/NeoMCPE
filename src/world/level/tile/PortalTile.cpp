#include "PortalTile.h"
#include "FireTile.h"
#include "../Level.h"
#include "../LevelSource.h"
#include "../material/Material.h"
#include "../../entity/Entity.h"
#include "../../../util/Random.h"
#include "../../item/ItemCategory.h"
<<<<<<< HEAD
=======

static bool sIsSpawning = false; // Spawning guard to prevent self-deletion
>>>>>>> da5303683bcdd68fe80c6b6b20e8bc11d1399370

// ----------------------------------------------------------------------------
// Helper: PortalShape port from MCPE 1.0
// ----------------------------------------------------------------------------
class PortalShape {
public:
	PortalShape(Level* level, int x, int y, int z, int axis)
		: mLevel(level), mAxis(axis), mNumPortalBlocks(0), mBottomLeftValid(false),
		  mHeight(0), mWidth(0), mX(x), mY(y), mZ(z)
	{
		if (axis == PortalTile::AXIS_X) {
			mLeftStepX = 1;  mLeftStepZ = 0;
			mRightStepX = -1; mRightStepZ = 0;
		} else {
			mLeftStepX = 0;  mLeftStepZ = -1;
			mRightStepX = 0;  mRightStepZ = 1;
		}

		int curY = y;
		while (curY > y - PortalTile::MAX_HEIGHT && curY > 0 && isEmptyBlock(mLevel->getTile(x, curY - 1, z))) {
			curY--;
		}

		int edge = getDistanceUntilEdge(x, curY, z, mLeftStepX, mLeftStepZ) - 1;
		if (edge >= 0) {
			mBottomLeftX = x + mLeftStepX * edge;
			mBottomLeftY = curY;
			mBottomLeftZ = z + mLeftStepZ * edge;
			mBottomLeftValid = true;

			mWidth = getDistanceUntilEdge(mBottomLeftX, mBottomLeftY, mBottomLeftZ, mRightStepX, mRightStepZ);
			if (mWidth < PortalTile::MIN_WIDTH || mWidth > PortalTile::MAX_WIDTH) {
				mBottomLeftValid = false;
				mWidth = 0;
			}
		}

		if (mBottomLeftValid) {
			mHeight = calculatePortalHeight();
		}
	}

	bool isEmptyBlock(int blockId) {
		return blockId == 0 || blockId == Tile::fire->id || (Tile::portalTile && blockId == Tile::portalTile->id);
	}

	int getDistanceUntilEdge(int startX, int startY, int startZ, int stepX, int stepZ) {
		int width = 0;
		for (; width <= PortalTile::MAX_WIDTH; width++) {
			int bx = startX + stepX * width;
			int bz = startZ + stepZ * width;
			if (!isEmptyBlock(mLevel->getTile(bx, startY, bz))) {
				break;
			}
			if (mLevel->getTile(bx, startY - 1, bz) != Tile::obsidian->id) {
				break;
			}
		}

		int edgeId = mLevel->getTile(startX + stepX * width, startY, startZ + stepZ * width);
		if (edgeId == Tile::obsidian->id) {
			return width;
		}
		return 0;
	}

	int calculatePortalHeight() {
		bool shapeFailed = false;

		for (mHeight = 0; mHeight < PortalTile::MAX_HEIGHT; mHeight++) {
			for (int i = 0; i < mWidth; i++) {
				int bx = mBottomLeftX + mRightStepX * i;
				int by = mBottomLeftY + mHeight;
				int bz = mBottomLeftZ + mRightStepZ * i;

				int block = mLevel->getTile(bx, by, bz);
				if (!isEmptyBlock(block)) {
					shapeFailed = true; // Hit ceiling
					break;
				}

				if (Tile::portalTile && block == Tile::portalTile->id) {
					mNumPortalBlocks++;
				}

				if (i == 0) {
					int leftFrame = mLevel->getTile(bx + mLeftStepX, by, bz + mLeftStepZ);
					if (leftFrame != Tile::obsidian->id) {
						shapeFailed = true;
						break;
					}
				} else if (i == mWidth - 1) {
					int rightFrame = mLevel->getTile(bx + mRightStepX, by, bz + mRightStepZ);
					if (rightFrame != Tile::obsidian->id) {
						shapeFailed = true;
						break;
					}
				}
			}

			if (shapeFailed) {
				break;
			}
		}

		// Verify top obsidian frame
		for (int i = 0; i < mWidth; i++) {
			int topId = mLevel->getTile(mBottomLeftX + mRightStepX * i, mBottomLeftY + mHeight, mBottomLeftZ + mRightStepZ * i);
			if (topId != Tile::obsidian->id) {
				mHeight = 0;
				mBottomLeftValid = false;
				return 0;
			}
		}

		if (mHeight > PortalTile::MAX_HEIGHT || mHeight < PortalTile::MIN_HEIGHT) {
			mBottomLeftValid = false;
			mWidth = 0;
			mHeight = 0;
			return 0;
		}

		return mHeight;
	}

	bool isValid() {
		return mBottomLeftValid && mWidth >= PortalTile::MIN_WIDTH && mWidth <= PortalTile::MAX_WIDTH
			&& mHeight >= PortalTile::MIN_HEIGHT && mHeight <= PortalTile::MAX_HEIGHT;
	}

	int getNumberOfPortalBlocks() { return mNumPortalBlocks; }
	int getWidth() { return mWidth; }
	int getHeight() { return mHeight; }

	void createPortalBlocks() {
		sIsSpawning = true;
		for (int w = 0; w < mWidth; w++) {
			for (int h = 0; h < mHeight; h++) {
				int bx = mBottomLeftX + mRightStepX * w;
				int by = mBottomLeftY + h;
				int bz = mBottomLeftZ + mRightStepZ * w;
				mLevel->setTileAndDataNoUpdate(bx, by, bz, Tile::portalTile->id, mAxis);
			}
		}
		sIsSpawning = false;

		// Once all blocks exist, send render updates
		for (int w = 0; w < mWidth; w++) {
			for (int h = 0; h < mHeight; h++) {
				int bx = mBottomLeftX + mRightStepX * w;
				int by = mBottomLeftY + h;
				int bz = mBottomLeftZ + mRightStepZ * w;
				mLevel->sendTileUpdated(bx, by, bz);
			}
		}
	}

private:
	Level* mLevel;
	int mAxis;
	int mLeftStepX, mLeftStepZ;
	int mRightStepX, mRightStepZ;
	int mNumPortalBlocks;
	bool mBottomLeftValid;
	int mBottomLeftX, mBottomLeftY, mBottomLeftZ;
	int mHeight, mWidth;
	int mX, mY, mZ;
};

// ----------------------------------------------------------------------------
// PortalTile Implementation
// ----------------------------------------------------------------------------

PortalTile::PortalTile(int id, int tex)
	: Tile(id, tex, Material::portal ? Material::portal : Material::stone)
{
	setTicking(true);
	setDestroyTime(-1.0f);
	setLightEmission(11.0f / 16.0f);
	this->category = ItemCategory::Structures;
	setDescriptionId("portal");
}

void PortalTile::updateShape(LevelSource* level, int x, int y, int z) {
	int data = level->getData(x, y, z);
	if (data == 0) {
		if (level->getTile(x - 1, y, z) == id || level->getTile(x + 1, y, z) == id) {
			data = AXIS_X;
		} else {
			data = AXIS_Z;
		}
	}

	float xr = 0.125f;
	float zr = 0.125f;

	if (data == AXIS_X) {
		xr = 0.5f;
		zr = 0.125f;
	} else {
		xr = 0.125f;
		zr = 0.5f;
	}
	setShape(0.5f - xr, 0.0f, 0.5f - zr, 0.5f + xr, 1.0f, 0.5f + zr);
}

AABB* PortalTile::getAABB(Level* level, int x, int y, int z) {
	return NULL; // Walk through
}

bool PortalTile::isSolidRender() {
	return false;
}

bool PortalTile::isCubeShaped() {
	return false;
}

int PortalTile::getRenderLayer() {
	return Tile::RENDERLAYER_ALPHATEST;
}

bool PortalTile::shouldRenderFace(LevelSource* level, int x, int y, int z, int face) {
	return true;
}

int PortalTile::getResourceCount(Random* random) {
	return 0;
}

void PortalTile::neighborChanged(Level* level, int x, int y, int z, int type) {
	if (sIsSpawning) return; // Ignore updates during creation!

	int axis = level->getData(x, y, z);
	PortalShape shape(level, x, y, z, (axis == AXIS_X) ? AXIS_X : AXIS_Z);

	if (!shape.isValid() || shape.getNumberOfPortalBlocks() < shape.getWidth() * shape.getHeight()) {
		level->setTile(x, y, z, 0);
	}
}

void PortalTile::entityInside(Level* level, int x, int y, int z, Entity* entity) {
	Player* player = Player::asPlayer(entity);
	if (player != NULL) {
		player->handleInsidePortal();
	}
}

void PortalTile::animateTick(Level* level, int x, int y, int z, Random* random) {
}

bool PortalTile::trySpawnPortal(Level* level, int x, int y, int z) {
	PortalShape xShape(level, x, y, z, AXIS_X);
	if (xShape.isValid() && xShape.getNumberOfPortalBlocks() == 0) {
		xShape.createPortalBlocks();
		return true;
	}

	PortalShape zShape(level, x, y, z, AXIS_Z);
	if (zShape.isValid() && zShape.getNumberOfPortalBlocks() == 0) {
		zShape.createPortalBlocks();
		return true;
	}

	return false;
}
