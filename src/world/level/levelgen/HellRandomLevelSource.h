#ifndef NET_MINECRAFT_WORLD_LEVEL_LEVELGEN__HellRandomLevelSource_H__
#define NET_MINECRAFT_WORLD_LEVEL_LEVELGEN__HellRandomLevelSource_H__

#include <map>
#include "../chunk/ChunkSource.h"
#include "../biome/Biome.h"
#include "synth/PerlinNoise.h"
#include "../../../util/Random.h"

class Level;
class LevelChunk;
class MobCategory;

class HellRandomLevelSource : public ChunkSource {
public:
	static const int CHUNK_HEIGHT = 8;
	static const int CHUNK_WIDTH = 4;
	static const int NOISE_SIZE_X = 16 / CHUNK_WIDTH + 1;
	static const int NOISE_SIZE_Y = 128 / CHUNK_HEIGHT + 1;
	static const int NOISE_SIZE_Z = 16 / CHUNK_WIDTH + 1;

	HellRandomLevelSource(Level* level, long seed);
	virtual ~HellRandomLevelSource();

	virtual bool hasChunk(int x, int z) override;
	virtual LevelChunk* create(int x, int z) override;
	virtual LevelChunk* getChunk(int xOffs, int zOffs) override;

	void prepareHeights(int xOffs, int zOffs, unsigned char* blocks);
	void buildSurfaces(int xOffs, int zOffs, unsigned char* blocks);
	virtual void postProcess(ChunkSource* parent, int xt, int zt) override;

	virtual bool tick() override;
	virtual bool shouldSave() override;
	virtual std::string gatherStats() override;

	virtual Biome::MobList getMobsAt(const MobCategory& mobCategory, int x, int y, int z) override;

private:
	void getHeights(float* noiseBuffer, int x, int y, int z);

	typedef std::map<int, LevelChunk*> ChunkMap;
	ChunkMap chunkMap;

	Level* level;
	Random random;

	PerlinNoise lperlinNoise1;
	PerlinNoise lperlinNoise2;
	PerlinNoise perlinNoise1;
	PerlinNoise perlinNoise2;
	PerlinNoise perlinNoise3;
	PerlinNoise scaleNoise;
	PerlinNoise depthNoise;

	float* buffer;
	float sandBuffer[16 * 16];
	float gravelBuffer[16 * 16];
	float depthBuffer[16 * 16];
};

#endif /* NET_MINECRAFT_WORLD_LEVEL_LEVELGEN__HellRandomLevelSource_H__ */