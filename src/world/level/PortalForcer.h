#ifndef NET_MINECRAFT_WORLD_LEVEL__PortalForcer_H__
#define NET_MINECRAFT_WORLD_LEVEL__PortalForcer_H__

class Level;
class Entity;
class Vec3;

class PortalForcer {
public:
	// Finds an existing portal near (x, y, z) or creates a 4x5 obsidian frame
	static bool findOrCreatePortal(Level* level, Entity* entity, float targetX, float targetY, float targetZ);
};

#endif /* NET_MINECRAFT_WORLD_LEVEL__PortalForcer_H__ */