#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE__PortalTile_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE__PortalTile_H__

#include "Tile.h"

class PortalTile : public Tile {
public:
	static const int AXIS_UNKNOWN = 0;
	static const int AXIS_X = 1;
	static const int AXIS_Z = 2;

	static const int MIN_WIDTH = 2;
	static const int MAX_WIDTH = 21;
	static const int MIN_HEIGHT = 3;
	static const int MAX_HEIGHT = 21;

	PortalTile(int id, int tex);

	virtual void updateShape(LevelSource* level, int x, int y, int z) override;
	virtual AABB* getAABB(Level* level, int x, int y, int z) override;
	virtual bool isSolidRender() override;
	virtual bool isCubeShaped() override;
	virtual int getRenderLayer() override;
	virtual bool shouldRenderFace(LevelSource* level, int x, int y, int z, int face) override;
	virtual int getResourceCount(Random* random) override;

	virtual void neighborChanged(Level* level, int x, int y, int z, int type) override;
	virtual void entityInside(Level* level, int x, int y, int z, Entity* entity) override;
	virtual void animateTick(Level* level, int x, int y, int z, Random* random) override;

	bool trySpawnPortal(Level* level, int x, int y, int z);
};

#endif /* NET_MINECRAFT_WORLD_LEVEL_TILE__PortalTile_H__ */