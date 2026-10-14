#include "HellDimension.h"
#include "../Level.h"
#include "../levelgen/HellRandomLevelSource.h"
#include "../biome/Biome.h"

HellDimension::HellDimension()
	: Dimension()
{
	this->id = Dimension::NETHER;
	this->foggy = true;
	this->ultraWarm = true;
	this->hasCeiling = true;
}

HellDimension::~HellDimension() {
}

void HellDimension::init() {
	this->ultraWarm = true;
	this->hasCeiling = true;
	this->foggy = true;
	// BiomeSource will be bound to HellBiome in Batch 2
}

void HellDimension::updateLightRamp() {
	float ambientLight = 0.10f;
	for (int i = 0; i <= 15; i++) {
		float v = (1.0f - (float)i / 15.0f);
		brightnessRamp[i] = ((1.0f - v) / (v * 3.0f + 1.0f)) * (1.0f - ambientLight) + ambientLight;
	}
}

bool HellDimension::isNaturalDimension() {
	return false;
}

bool HellDimension::isValidSpawn(int x, int z) {
	return false;
}

float HellDimension::getTimeOfDay(long time, float a) {
	return 0.5f; // Constant twilight/noon in Nether
}

Vec3 HellDimension::getFogColor(float td, float a) {
	// Directly from 1.0: Color(0.2f, 0.03f, 0.03f)
	return Vec3(0.2f, 0.03f, 0.03f);
}

bool HellDimension::mayRespawn() {
	return false; // Explode bed on click
}

ChunkSource* HellDimension::createRandomLevelSource() {
	return new HellRandomLevelSource(level, level->getSeed());
}

std::string HellDimension::getDimension() {
	return "Nether";
}