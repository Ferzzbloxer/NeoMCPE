#ifndef NET_MINECRAFT_CLIENT_RENDERER__DynamicLights_H__
#define NET_MINECRAFT_CLIENT_RENDERER__DynamicLights_H__

class Minecraft;
class Level;

class DynamicLights {
public:
	// Called every tick to update the light around the player
	static void update(Minecraft* mc);

	// Restores all blocks back to their original darkness
	static void clear(Level* level);
};

#endif /* NET_MINECRAFT_CLIENT_RENDERER__DynamicLights_H__ */