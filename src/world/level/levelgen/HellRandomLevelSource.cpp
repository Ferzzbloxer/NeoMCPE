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
#include "../../entity/MobFactory.h"
#include "../../entity/Mob.h"

// ----------------------------------------------------------------------------
// 1.0 Hanging Glowstone Stalactite Cluster Generator
// ----------------------------------------------------------------------------
static void placeGlowstoneCluster(Level* level, Random* random, int startX, int startY, int startZ) {
	if (!level->isEmptyTile(startX, startY, startZ)) return;
	if (level->getTile(startX, startY + 1, startZ) != Tile::netherrack->id) return;

	level->setTile(startX, startY, startZ, Tile::lightGem->id);

	for (int i = 0; i < 1500; i++) {
		int x = startX + random->nextInt(8) - random->nextInt(8);
		int y = startY - random->nextInt(12);
		int z = startZ + random->nextInt(8) - random->nextInt(8);

		if (level->getTile(x, y, z) == 0) {
			int glowNeighbors = 0;
			if (level->getTile(x - 1, y, z) == Tile::lightGem->id) glowNeighbors++;
			if (level->getTile(x + 1, y, z) == Tile::lightGem->id) glowNeighbors++;
			if (level->getTile(x, y - 1, z) == Tile::lightGem->id) glowNeighbors++;
			if (level->getTile(x, y + 1, z) == Tile::lightGem->id) glowNeighbors++;
			if (level->getTile(x, y, z - 1) == Tile::lightGem->id) glowNeighbors++;
			if (level->getTile(x, y, z + 1) == Tile::lightGem->id) glowNeighbors++;

			if (glowNeighbors == 1) {
				level->setTile(x, y, z, Tile::lightGem->id);
			}
		}
	}
}

HellRandomLevelSource::HellRandomLevelSource(Level* level, long seed)
	: level(level),
	  random(seed),
	  lperlinNoise1(&random, 16),
	  lperlinNoise2(&random, 16),
	  perlinNoise1(&random, 8),
	  perlinNoise2(&random, 4),
	  perlinNoise3(&random, 4),
	  scaleNoise(&random, 10),
	  depthNoise(&random, 16),
	  pnr(NULL), ar(NULL), br(NULL), sr(NULL), dr(NULL) // Initialize cached arrays!
{
	buffer = new float[NOISE_SIZE_X * NOISE_SIZE_Y * NOISE_SIZE_Z];
}

HellRandomLevelSource::~HellRandomLevelSource() {
	delete[] buffer;
	delete[] pnr;
	delete[] ar;
	delete[] br;
	delete[] sr;
	delete[] dr;
}

void HellRandomLevelSource::getHeights(float* noiseBuffer, int x, int y, int z) {
	float s = 684.412f;
	float hs = 684.412f * 3.0f;

	// Safely reuse internal arrays instead of allocating/freeing!
	sr = scaleNoise.getRegion(sr, x, z, NOISE_SIZE_X, NOISE_SIZE_Z, 1.0f, 1.0f, 0.5f);
	dr = depthNoise.getRegion(dr, x, z, NOISE_SIZE_X, NOISE_SIZE_Z, 100.0f, 100.0f, 0.5f);

	pnr = perlinNoise1.getRegion(pnr, (float)x, (float)y, (float)z, NOISE_SIZE_X, NOISE_SIZE_Y, NOISE_SIZE_Z, s / 80.0f, hs / 60.0f, s / 80.0f);
	ar = lperlinNoise1.getRegion(ar, (float)x, (float)y, (float)z, NOISE_SIZE_X, NOISE_SIZE_Y, NOISE_SIZE_Z, s, hs, s);
	br = lperlinNoise2.getRegion(br, (float)x, (float)y, (float)z, NOISE_SIZE_X, NOISE_SIZE_Y, NOISE_SIZE_Z, s, hs, s);

	float yoffs[NOISE_SIZE_Y];
	for (int yy = 0; yy < NOISE_SIZE_Y; yy++) {
		yoffs[yy] = Mth::cos(yy * Mth::PI * 6.0f / (float)NOISE_SIZE_Y) * 2.0f;
		float dd = (float)yy;
		if (yy > NOISE_SIZE_Y / 2) {
			dd = (float)(NOISE_SIZE_Y - 1) - yy;
		}
		if (dd < 4.0f) {
			dd = 4.0f - dd;
			yoffs[yy] -= dd * dd * dd * 10.0f;
		}
	}

	int p = 0;
	for (int xx = 0; xx < NOISE_SIZE_X; xx++) {
		for (int zz = 0; zz < NOISE_SIZE_Z; zz++) {
			for (int yy = 0; yy < NOISE_SIZE_Y; yy++) {
				float val = 0.0f;
				float yOffs = yoffs[yy];

				float bb = ar[p] / 512.0f;
				float cc = br[p] / 512.0f;

				float v = (pnr[p] / 10.0f + 1.0f) / 2.0f;
				if (v < 0.0f) val = bb;
				else if (v > 1.0f) val = cc;
				else val = bb + (cc - bb) * v;

				val -= yOffs;
				val -= 2.0f; // Open cavern halls

				if (yy > NOISE_SIZE_Y - 4) {
					float slide = (yy - (NOISE_SIZE_Y - 4)) / (4.0f - 1.0f);
					val = val * (1.0f - slide) + -10.0f * slide;
				}

				noiseBuffer[p] = val;
				p++;
			}
		}
	}
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
					float _s0 = s0;
					float _s1 = s1;
					float _s0a = (s2 - s0) * xStep;
					float _s1a = (s3 - s1) * xStep;

					for (int x = 0; x < CHUNK_WIDTH; x++) {
						float zStep = 1.0f / (float)CHUNK_WIDTH;
						float val = _s0;
						float vala = (_s1 - _s0) * zStep;

						int blockX = x + xc * CHUNK_WIDTH;
						int blockY = yc * CHUNK_HEIGHT + y;

						for (int z = 0; z < CHUNK_WIDTH; z++) {
							int blockZ = z + zc * CHUNK_WIDTH;

							// Native MCPE 0.6.1 Chunk array index: (X << 11) | (Z << 7) | Y
							int offs = (blockX << 11) | (blockZ << 7) | blockY;

							int tileId = 0;
							if (blockY < lavaHeight) {
								tileId = Tile::calmLava->id;
							}
							if (val > 0.0f) {
								tileId = Tile::netherrack->id;
							}

							blocks[offs] = (unsigned char)tileId;
							val += vala;
						}
						_s0 += _s0a;
						_s1 += _s1a;
					}
					s0 += s0a;
					s1 += s1a;
					s2 += s2a;
					s3 += s3a;
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
				// Native MCPE 0.6.1 Chunk array index: (x << 11) | (z << 7) | y
				int offs = (x << 11) | (z << 7) | y;

				// Bedrock Ceiling (123..127) & Bedrock Floor (0..4)
				if (y >= 127 - random.nextInt(5) || y <= random.nextInt(5)) {
					blocks[offs] = (unsigned char)Tile::unbreakable->id;
				} else {
					int old = blocks[offs];
					if (old == 0) {
						run = -1;
					} else if (old == Tile::netherrack->id) {
						if (run == -1) {
							if (runDepth <= 0) {
								top = 0;
								mat = Tile::netherrack->id;
							} else if (y >= 28 && y <= 33) {
								top = Tile::netherrack->id;
								mat = Tile::netherrack->id;
								if (gravel) {
									top = Tile::gravel->id;
									mat = Tile::netherrack->id;
								}
								if (sand) {
									top = Tile::gravel->id;
									mat = top;
								}
							}

							if (y < 32 && top == 0) {
								top = Tile::calmLava->id;
							}

							run = runDepth;
							if (y >= 31) {
								blocks[offs] = (unsigned char)top;
							} else {
								blocks[offs] = (unsigned char)mat;
							}
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

	// 1. Cascading Lavafalls flowing down cavern walls
	for (int i = 0; i < 24; i++) {
		int x = xo + random.nextInt(16) + 8;
		int y = random.nextInt(90) + 25;
		int z = zo + random.nextInt(16) + 8;
		SpringFeature feature(Tile::lava->id);
		feature.place(level, &random, x, y, z);
		level->addToTickNextTick(x, y, z, Tile::lava->id, 10);
	}

	// 2. Hidden Lava Pockets inside solid Netherrack
	for (int i = 0; i < 8; i++) {
		int x = xo + random.nextInt(16) + 8;
		int y = random.nextInt(100) + 15;
		int z = zo + random.nextInt(16) + 8;
		if (level->getTile(x, y, z) == Tile::netherrack->id) {
			level->setTile(x, y, z, Tile::calmLava->id);
		}
	}

	// 3. Fire Patches on the ground
	int fireCount = random.nextInt(random.nextInt(12) + 1) + 1;
	for (int i = 0; i < fireCount; i++) {
		int x = xo + random.nextInt(16) + 8;
		int y = random.nextInt(120) + 4;
		int z = zo + random.nextInt(16) + 8;
		if (level->getTile(x, y - 1, z) == Tile::netherrack->id && level->getTile(x, y, z) == 0) {
			level->setTile(x, y, z, Tile::fire->id);
		}
	}

	// 4. Large Hanging Glowstone Stalactites
	for (int i = 0; i < 15; i++) {
		int x = xo + random.nextInt(16) + 8;
		int y = random.nextInt(85) + 35;
		int z = zo + random.nextInt(16) + 8;
		placeGlowstoneCluster(level, &random, x, y, z);
	}

	// 5. Nether Gold Ore Veins (placed directly in Netherrack)
	for (int i = 0; i < 20; i++) {
		int x = xo + random.nextInt(16);
		int y = random.nextInt(108) + 10;
		int z = zo + random.nextInt(16);
		OreFeature goldFeature(Tile::goldOre->id, 10);
		goldFeature.place(level, &random, x, y, z);
	}

	// 6. Nether Quartz (16 veins per chunk)
	for (int i = 0; i < 16; i++) {
		int x = xo + random.nextInt(16);
		int y = random.nextInt(108) + 10;
		int z = zo + random.nextInt(16);
		OreFeature quartzFeature(Tile::quartzBlock->id, 14);
		quartzFeature.place(level, &random, x, y, z);
	}

	// 7. Mushrooms
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

	// 8. Spawn Zombie Pigman Hordes in the caverns!
	// 50% chance per chunk to spawn a pack of 1 to 4 Pigmen
	if (random.nextInt(2) == 0) {
		int pigmanCount = random.nextInt(4) + 1;
		for (int i = 0; i < pigmanCount; i++) {
			int px = xo + random.nextInt(16);
			int pz = zo + random.nextInt(16);
			int py = random.nextInt(60) + 30; // Start scan in the middle of the cavern

			// Scan downwards to find the floor
			while (py > 10 && level->getTile(px, py, pz) == 0) {
				py--;
			}

			// If we landed on Netherrack with enough air space above it, spawn the Pigman!
			if (level->getTile(px, py, pz) == Tile::netherrack->id && 
			    level->getTile(px, py + 1, pz) == 0 && 
			    level->getTile(px, py + 2, pz) == 0) {
				
				Mob* pigman = MobFactory::CreateMob(36, level); // 36 = PigZombie
				if (pigman != NULL) {
					pigman->setPos((float)px + 0.5f, (float)py + 1.0f, (float)pz + 0.5f);
					level->addEntity(pigman);
				}
			}
		}
	}

	HeavyTile::instaFall = false;
	level->isGeneratingTerrain = false;
}

LevelChunk* HellRandomLevelSource::create(int x, int z) {
	return getChunk(x, z);
}

LevelChunk* HellRandomLevelSource::getChunk(int xOffs, int zOffs) {
	int hashedPos = ChunkPos::hashCode(xOffs, zOffs);

	ChunkMap::iterator it = chunkMap.find(hashedPos);
	if (it != chunkMap.end())
		return it->second;

	random.setSeed((long)(xOffs * 341872712l + zOffs * 132899541l));

	unsigned char* blocks = new unsigned char[LevelChunk::ChunkBlockCount];
	LevelChunk* chunk = new LevelChunk(level, blocks, xOffs, zOffs);
	chunkMap.insert(std::make_pair(hashedPos, chunk));

	prepareHeights(xOffs, zOffs, blocks);
	buildSurfaces(xOffs, zOffs, blocks);

	chunk->recalcHeightmap();
	return chunk;
}

bool HellRandomLevelSource::hasChunk(int x, int z) {
	return true;
}

bool HellRandomLevelSource::tick() {
	return false;
}

bool HellRandomLevelSource::shouldSave() {
	return true;
}

std::string HellRandomLevelSource::gatherStats() {
	return "HellRandomLevelSource";
}

Biome::MobList HellRandomLevelSource::getMobsAt(const MobCategory& mobCategory, int x, int y, int z) {
	if (Biome::hell != NULL) {
		return Biome::hell->getMobs(mobCategory);
	}
	return Biome::MobList();
}