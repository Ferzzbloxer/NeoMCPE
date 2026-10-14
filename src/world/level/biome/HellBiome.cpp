#include "HellBiome.h"
#include "../tile/Tile.h"
#include "../../entity/EntityTypes.h"

HellBiome::HellBiome()
	: Biome()
{
	setName("Hell");
	topMaterial = (char)Tile::netherrack->id;
	material = (char)Tile::netherrack->id;
	temperature = 2.0f;
	downfall = 0.0f;

	_enemies.clear();
	_friendlies.clear();
	_waterFriendlies.clear();

	// In 0.6.1 EntityTypes: PigZombie (ID 36)
	_enemies.push_back(MobSpawnerData(36, 100, 4, 4));
}

float HellBiome::getCreatureProbability() {
	return 0.0f; // No cows/sheep in Nether
}