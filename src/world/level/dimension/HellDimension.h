#ifndef NET_MINECRAFT_WORLD_LEVEL_DIMENSION__HellDimension_H__
#define NET_MINECRAFT_WORLD_LEVEL_DIMENSION__HellDimension_H__

#include "Dimension.h"

class HellDimension : public Dimension {
public:
	HellDimension();
	virtual ~HellDimension();

	virtual void init() override;
	virtual void updateLightRamp() override;

	virtual bool isNaturalDimension() override;
	virtual bool isValidSpawn(int x, int z) override;
	virtual float getTimeOfDay(long time, float a) override;
	virtual Vec3 getFogColor(float td, float a) override;
	virtual bool mayRespawn() override;

	virtual ChunkSource* createRandomLevelSource() override;
	virtual std::string getDimension();
};

#endif /* NET_MINECRAFT_WORLD_LEVEL_DIMENSION__HellDimension_H__ */