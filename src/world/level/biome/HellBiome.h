#ifndef NET_MINECRAFT_WORLD_LEVEL_BIOME__HellBiome_H__
#define NET_MINECRAFT_WORLD_LEVEL_BIOME__HellBiome_H__

#include "Biome.h"

class HellBiome : public Biome {
public:
	HellBiome();
	virtual float getCreatureProbability() override;
};

#endif /* NET_MINECRAFT_WORLD_LEVEL_BIOME__HellBiome_H__ */