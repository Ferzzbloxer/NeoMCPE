#include "HellRandomLevelSource.h"
#include "../Level.h"
#include "../ChunkPos.h"
#include "../chunk/LevelChunk.h"
#include "../tile/Tile.h"
#include "../tile/FireTile.h"
#include "../tile/HeavyTile.h"
#include "../biome/Biome.h"
#include "feature/FeatureInclude.h"
#include "../../../util/Mth.h"

// ----------------------------------------------------------------------------
// Custom Nether Features
// ----------------------------------------------------------------------------

static void placeGlowstoneCluster(Level* level, Random* random, int startX, int startY, int startZ) {
	// Must start hanging from a Netherrack ceiling
	if (!level->isEmptyTile(startX, startY, startZ)) return;
	if (level->getTile(startX, startY + 1, startZ) != Tile::netherrack->id) return;

	level->setTile(startX, startY, startZ, Tile::lightGem->id);

	for (int i = 0; i < 1500; i++) {
		int x = startX + random->nextInt(8) - random->nextInt(8);
		int y = startY - random->nextInt(12);
		int z = startZ + random->nextInt(8) - random->nextInt(8);

		if (level->isEmptyTile(x, y, z)) {
			int glowNeighbors = 0;
			if (level->getTile(x - 1, y, z) == Tile::lightGem->id) glowNeighbors++;
			if (level->getTile(x + 1, y, z) == Tile::lightGem->id) glowNeighbors++;
			if (level->getTile(x, y - 1, z) == Tile::lightGem->id) glowNeighbors++;
			if (level->getTile(x, y + 1, z) == Tile::lightGem->id) glowNeighbors++;
			if (level->getTile(x, y, z - 1) == Tile::lightGem->id) glowNeighbors++;
			if (level->getTile(x, y, z + 1) == Tile::lightGem->id) glowNeighbors++;

			// Grow naturally off existing glowstone
			if (glowNeighbors == 1) {
				level->setTile(x, y, z, Tile::lightGem->id);
			}
		}
	}
}

static void placeNetherSpring(Level* level, int x, int y, int z) {
	// Must be placed in a wall: 3 sides Netherrack, 1 side air
	if (!level->isEmptyTile(x, y, z)) return;
	if (level->getTile(x, y + 1, z) != Tile::netherrack->id) return;

	int solidCount = 0;
	if (level->getTile(x - 1, y, z) == Tile::netherrack->id) solidCount++;
	if (level->getTile(x + 1, y, z) == Tile::netherrack->id) solidCount++;
	if (level->getTile(x, y, z - 1) == Tile::netherrack->id) solidCount++;
	if (level->getTile(x, y, z + 1) == Tile::netherrack->id) solidCount++;

	int airCount = 0;
	if (level->isEmptyTile(x - 1, y, z)) airCount++;
	if (level->isEmptyTile(x + 1, y, z)) airCount++;
	if (level->isEmptyTile(x, y, z - 1)) airCount++;
	if (level->isEmptyTile(x, y, z + 1)) airCount++;

	if (solidCount == 3 && airCount == 1) {
		level->setTile(x, y, z, Tile::lava->id);
		// Force the lava to start flowing!
		level->addToTickNextTick(x, y, z, Tile::lava->id, 0); 
	}
}

static void placeNetherOre(Level* level, Random* random, int startX, int startY, int startZ, int oreId, int count) {
	// Random walk algorithm guarantees clustered veins that NEVER float in air
	if (level->getTile(startX, startY, startZ) != Tile::netherrack->id) return;
	
	level->setTile(startX, startY, startZ, oreId);
	int placed = 1;
	int x = startX, y = startY, z = startZ;

	for (int i = 0; i < count * 4 && placed < count; i++) {
		int nx = x + random->nextInt(3) - 1;
		int ny = y + random->nextInt(3) - 1;
		int nz = z + random->nextInt(3) - 1;

		if (level->getTile(nx, ny, nz) == Tile::netherrack->id) {
			level->setTile(nx, ny, nz, oreId);
			placed++;
			x = nx; y = ny; z = nz;
		}
	}
}

// ----------------------------------------------------------------------------
// Generator Implementation
// ----------------------------------------------------------------------------

HellRandomLevelSource::HellRandomLevelSource(Level* level, long seed)
	: level(level), random(seed),
	  lperlinNoise1(&random, 16), lperlinNoise2(&random, 16),
	  perlinNoise1(&random, 8), perlinNoise2(&random, 4), perlinNoise3(&random, 4),
	  scaleNoise(&random, 10), depthNoise(&random, 16) {
	buffer = new float[NOISE_SIZE_X * NOISE_SIZE_Y * NOISE_SIZE_Z];
}

HellRandomLevelSource::~HellRandomLevelSource() {
	delete[] buffer;
}

void HellRandomLevelSource::getHeights(float* noiseBuffer, int x, int y, int z) {
	float s = 684.412f;
	float hs = 684.412f * 3.0f;

	float* scaleRegion = NULL; float* depthRegion = NULL;
	float* noiseRegionPrimary = NULL; float* noiseRegionA = NULL; float* noiseRegionB = NULL;

	scaleRegion = scaleNoise.getRegion(scaleRegion, x, z, NOISE_SIZE_X, NOISE_SIZE_Z, 1.0f, 1.0f, 0.5f);
	depthRegion = depthNoise.getRegion(depthRegion, x, z, NOISE_SIZE_X, NOISE_SIZE_Z, 100.0f, 100.0f, 0.5f);
	noiseRegionPrimary = perlinNoise1.getRegion(noiseRegionPrimary, (float)x, (float)y, (float)z, NOISE_SIZE_X, NOISE_SIZE_Y, NOISE_SIZE_Z, s / 80.0f, hs / 60.0f, s / 80.0f);
	noiseRegionA = lperlinNoise1.getRegion(noiseRegionA, (float)x, (float)y, (float)z, NOISE_SIZE_X, NOISE_SIZE_Y, NOISE_SIZE_Z, s, hs, s);
	noiseRegionB = lperlinNoise2.getRegion(noiseRegionB, (float)x, (float)y, (float)z, NOISE_SIZE_X, NOISE_SIZE_Y, NOISE_SIZE_Z, s, hs, s);

	float yoffs[NOISE_SIZE_Y];
	for (int yy = 0; yy < NOISE_SIZE_Y; yy++) {
		yoffs[yy] = Mth::cos(yy * Mth::PI * 6.0f / (float)NOISE_SIZE_Y) * 2.0f;
		float dd = (float)yy;
		if (yy > NOISE_SIZE_Y / 2) dd = (float)(NOISE_SIZE_Y - 1) - yy;
		if (dd < 4.0f) { dd = 4.0f - dd; yoffs[yy] -= dd * dd * dd * 10.0f; }
	}

	int p = 0;
	for (int xx = 0; xx < NOISE_SIZE_X; xx++) {
		for (int zz = 0; zz < NOISE_SIZE_Z; zz++) {
			for (int yy = 0; yy < NOISE_SIZE_Y; yy++) {
				float val = 0.0f;
				float yOffs = yoffs[yy];
				float bb = noiseRegionA[p] / 512.0f;
				float cc = noiseRegionB[p] / 512.0f;
				float v = (noiseRegionPrimary[p] / 10.0f + 1.0f) / 2.0f;

				if (v < 0.0f) val = bb;
				else if (v > 1.0f) val = cc;
				else val = bb + (cc - bb) * v;

				val -= yOffs;
				val -= 1.5f; // Open cavern halls

				if (yy > NOISE_SIZE_Y - 4) {
					float slide = (yy - (NOISE_SIZE_Y - 4)) / (4.0f - 1.0f);
					val = val * (1.0f - slide) + -10.0f * slide;
				}

				noiseBuffer[p] = val;
				p++;
			}
		}
	}

	delete[] scaleRegion; delete[] depthRegion; delete[] noiseRegionPrimary; delete[] noiseRegionA; delete[] noiseRegionB;
}

void HellRandomLevelSource::prepareHeights(int xOffs, int zOffs, unsigned char* blocks) {
	int xChunks = 16 / CHUNK_WIDTH;
	int lavaHeight = 32;
	getHeights(buffer, xOffs * xChunks, 0, zOffs * xChunks);

	for (int xc = 0; xc < xChunks; xc++) {
		for (int zc = 0; zc < xChunks; zc++) {
			for (int yc = 0; yc < 128 / CHUNK_HEIGHT; yc++) {
				float yStep = 1.0f / (float)CHUNK_HEIGHT;
				float s0 = buffer[((xc + 0) * NOISE_SIZE_Z + (zc + 0)) * NOISE_SIZE_Y + (yc + 0)];
				float s1 = buffer[((xc + 0) * NOISE_SIZE_Z + (zc + 1)) * NOISE_SIZE_Y + (yc + 0)];
				float s2 = buffer[((xc + 1) * NOISE_SIZE_Z + (zc + 0)) * NOISE_SIZE_Y + (yc + 0)];
				float s3 = buffer[((xc + 1) * NOISE_SIZE_Z + (zc + 1)) * NOISE_SIZE_Y + (yc + 0)];
				float s0a = (buffer[((xc + 0) * NOISE_SIZE_Z + (zc + 0)) * NOISE_SIZE_Y + (yc + 1)] - s0) * yStep;
				float s1a = (buffer[((xc + 0) * NOISE_SIZE_Z + (zc + 1)) * NOISE_SIZE_Y + (yc + 1)] - s1) * yStep;
				float s2a = (buffer[((xc + 1) * NOISE_SIZE_Z + (zc + 0)) * NOISE_SIZE_Y + (yc + 1)] - s2) * yStep;
				float s3a = (buffer[((xc + 1) * NOISE_SIZE_Z + (zc + 1)) * NOISE_SIZE_Y + (yc + 1)] - s3) * yStep;

				for (int y = 0; y < CHUNK_HEIGHT; y++) {
					float xStep = 1.0f / (float)CHUNK_WIDTH;
					float _s0 = s0; float _s1 = s1;
					float _s0a = (s2 - s0) * xStep; float _s1a = (s3 - s1) * xStep;

					for (int x = 0; x < CHUNK_WIDTH; x++) {
						float zStep = 1.0f / (float)CHUNK_WIDTH;
						float val = _s0; float vala = (_s1 - _s0) * zStep;

						int blockX = x + xc * CHUNK_WIDTH;
						int blockY = yc * CHUNK_HEIGHT + y;

						for (int z = 0; z < CHUNK_WIDTH; z++) {
							int blockZ = z + zc * CHUNK_WIDTH;

							// Native MCPE 0.6.1 array index: (Z << 11) | (X << 7) | Y
							int offs = (blockZ << 11) | (blockX << 7) | blockY;
							int tileId = 0;

							if (blockY < lavaHeight) tileId = Tile::calmLava->id;
							if (val > 0.0f) tileId = Tile::netherrack->id;

							blocks[offs] = (unsigned char)tileId;
							val += vala;
						}
						_s0 += _s0a; _s1 += _s1a;
					}
					s0 += s0a; s1 += s1a; s2 += s2a; s3 += s3a;
				}
			}
		}
	}
}

void HellRandomLevelSource::buildSurfaces(int xOffs, int zOffs, unsigned char* blocks) {
	float s = 1.0f / 32.0f;
	perlinNoise2.getRegion(sandBuffer, (float)(xOffs * 16), (float)(zOffs * 16), 0, 16, 16, 1, s, s, 1.0f);
	perlinNoise2.getRegion(gravelBuffer, (float)(xOffs * 16), 109.01340f, (float)(zOffs * 16), 16, 1, 16, s, 1.0f, s);
	perlinNoise3.getRegion(depthBuffer, (float)(xOffs * 16), (float)(zOffs * 16), 0, 16, 16, 1, s * 2.0f, s * 2.0f, s * 2.0f);

	for (int x = 0; x < 16; x++) {
		for (int z = 0; z < 16; z++) {
			bool gravel = (gravelBuffer[x + z * 16] + random.nextFloat() * 0.2f) > 0.0f;
			bool sand = (sandBuffer[x + z * 16] + random.nextFloat() * 0.2f) > 0.0f;
			int runDepth = (int)(depthBuffer[x + z * 16] / 3.0f + 3.0f + random.nextFloat() * 0.25f);

			int run = -1;
			int top = Tile::netherrack->id;
			int mat = Tile::netherrack->id;

			for (int y = 127; y >= 0; y--) {
				// Native MCPE 0.6.1 array index: (Z << 11) | (X << 7) | Y
				int offs = (z << 11) | (x << 7) | y;

				if (y >= 127 - random.nextInt(5) || y <= random.nextInt(5)) {
					blocks[offs] = (unsigned char)Tile::unbreakable->id;
				} else {
					int old = blocks[offs];
					if (old == 0) {
						run = -1;
					} else if (old == Tile::netherrack->id) {
						if (run == -1) {
							if (runDepth <= 0) {
								top = 0; mat = Tile::netherrack->id;
							} else if (y >= 28 && y <= 33) {
								top = Tile::netherrack->id; mat = Tile::netherrack->id;
								if (gravel) { top = Tile::gravel->id; mat = Tile::netherrack->id; }
								if (sand) { top = Tile::gravel->id; mat = top; }
							}

							if (y < 32 && top == 0) top = Tile::calmLava->id;

							run = runDepth;
							blocks[offs] = (unsigned char)((y >= 31) ? top : mat);
						} else if (run > 0) {
							run--;
							blocks[offs] = (unsigned char)mat;
						}
					}
				}
			}
		}
	}
}

void HellRandomLevelSource::postProcess(ChunkSource* parent, int xt, int zt) {
	level->isGeneratingTerrain = true;
	HeavyTile::instaFall = true;

	int xo = xt * 16;
	int zo = zt * 16;

	random.setSeed(level->getSeed());
	int xScale = random.nextInt() / 2 * 2 + 1;
	int zScale = random.nextInt() / 2 * 2 + 1;
	random.setSeed(((xt * xScale) + (zt * zScale)) ^ level->getSeed());

	// 1. Cascading Lavafalls (using dynamic flowing lava so it flows down!)
	for (int i = 0; i < 24; i++) {
		int x = xo + random.nextInt(16) + 8;
		int y = random.nextInt(85) + 30;
		int z = zo + random.nextInt(16) + 8;
		SpringFeature feature(Tile::lava->id);
		feature.place(level, &random, x, y, z);
		level->addToTickNextTick(x, y, z, Tile::lava->id, 10);
	}

	// 2. Nether Gold Veins (placed directly in Netherrack)
	for (int i = 0; i < 20; i++) {
		int x = xo + random.nextInt(16);
		int y = random.nextInt(108) + 10;
		int z = zo + random.nextInt(16);
		OreFeature goldFeature(Tile::goldOre->id, 10);
		goldFeature.place(level, &random, x, y, z);
	}

	// 3. Fire Patches on the ground
	int fireCount = random.nextInt(random.nextInt(10) + 1) + 1;
	for (int i = 0; i < fireCount; i++) {
		int x = xo + random.nextInt(16) + 8;
		int y = random.nextInt(120) + 4;
		int z = zo + random.nextInt(16) + 8;
		if (level->getTile(x, y - 1, z) == Tile::netherrack->id && level->getTile(x, y, z) == 0) {
			level->setTile(x, y, z, Tile::fire->id);
		}
	}

	// 4. Large Hanging Glowstone Clusters
	for (int i = 0; i < 15; i++) {
		int x = xo + random.nextInt(16) + 8;
		int y = random.nextInt(85) + 35;
		int z = zo + random.nextInt(16) + 8;
		placeGlowstoneCluster(level, &random, x, y, z);
	}

	// 5. Nether Gold & Quartz (Embedded inside Netherrack walls)
	for (int i = 0; i < 20; i++) {
		int x = xo + random.nextInt(16);
		int y = random.nextInt(108) + 10;
		int z = zo + random.nextInt(16);
		placeNetherOre(level, &random, x, y, z, Tile::goldOre->id, 10);

		int qx = xo + random.nextInt(16);
		int qy = random.nextInt(108) + 10;
		int qz = zo + random.nextInt(16);
		placeNetherOre(level, &random, qx, qy, qz, Tile::quartzBlock->id, 14);
	}

	// 6. Mushrooms
	if (random.nextInt(2) == 0) {
		int x = xo + random.nextInt(16) + 8;
		int y = random.nextInt(128);
		int z = zo + random.nextInt(16) + 8;
		FlowerFeature feature(Tile::mushroom1->id);
		feature.place(level, &random, x, y, z);
	}
	if (random.nextInt(2) == 0) {
		int x = xo + random.nextInt(16) + 8;
		int y = random.nextInt(128);
		int z = zo + random.nextInt(16) + 8;
		FlowerFeature feature(Tile::mushroom2->id);
		feature.place(level, &random, x, y, z);
	}

	HeavyTile::instaFall = false;
	level->isGeneratingTerrain = false;
}

LevelChunk* HellRandomLevelSource::create(int x, int z) { return getChunk(x, z); }

LevelChunk* HellRandomLevelSource::getChunk(int xOffs, int zOffs) {
	int hashedPos = ChunkPos::hashCode(xOffs, zOffs);

	ChunkMap::iterator it = chunkMap.find(hashedPos);
	if (it != chunkMap.end()) return it->second;

	random.setSeed((long)(xOffs * 341872712l + zOffs * 132899541l));

	unsigned char* blocks = new unsigned char[LevelChunk::ChunkBlockCount];
	LevelChunk* chunk = new LevelChunk(level, blocks, xOffs, zOffs);
	chunkMap.insert(std::make_pair(hashedPos, chunk));

	prepareHeights(xOffs, zOffs, blocks);
	buildSurfaces(xOffs, zOffs, blocks);

	chunk->recalcHeightmap();
	return chunk;
}

bool HellRandomLevelSource::hasChunk(int x, int z) { return true; }
bool HellRandomLevelSource::tick() { return false; }
bool HellRandomLevelSource::shouldSave() { return true; }
std::string HellRandomLevelSource::gatherStats() { return "HellRandomLevelSource"; }

Biome::MobList HellRandomLevelSource::getMobsAt(const MobCategory& mobCategory, int x, int y, int z) {
	if (Biome::hell != NULL) return Biome::hell->getMobs(mobCategory);
	return Biome::MobList();
}