#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/types.h>
#include <unistd.h>

#include "maplandmass.h"

namespace Engine {
	namespace Map {
		MapLandmass::MapLandmass() {
			id=IdNone;
			kingdomId=MapKingdomIdNone;
			area=0;
			isWater=true;

			tileExampleX=0;
			tileExampleY=0;
		}

		MapLandmass::MapLandmass(Id id, uint32_t area, bool isWater, uint16_t tileExampleX, uint16_t tileExampleY): id(id), area(area), isWater(isWater), tileExampleX(tileExampleX), tileExampleY(tileExampleY) {
			kingdomId=MapKingdomIdNone;
		}

		MapLandmass::~MapLandmass() {
		}

		bool MapLandmass::load(FILE *file) {
			assert(file!=NULL);

			bool result=true;

			result&=(fread(&id, sizeof(id), 1, file)==1);
			result&=(fread(&kingdomId, sizeof(kingdomId), 1, file)==1);
			result&=(fread(&area, sizeof(area), 1, file)==1);
			result&=(fread(&isWater, sizeof(isWater), 1, file)==1);
			result&=(fread(&tileExampleX, sizeof(tileExampleX),  1, file)==1);
			result&=(fread(&tileExampleY, sizeof(tileExampleY),  1, file)==1);

			return result;
		}

		bool MapLandmass::save(FILE *file) const {
			assert(file!=NULL);

			bool result=true;

			result&=(fwrite(&id, sizeof(id), 1, file)==1);
			result&=(fwrite(&kingdomId, sizeof(kingdomId), 1, file)==1);
			result&=(fwrite(&area, sizeof(area), 1, file)==1);
			result&=(fwrite(&isWater, sizeof(isWater), 1, file)==1);
			result&=(fwrite(&tileExampleX, sizeof(tileExampleX),  1, file)==1);
			result&=(fwrite(&tileExampleY, sizeof(tileExampleY),  1, file)==1);

			return result;
		}

		MapLandmass::Id MapLandmass::getId(void) const {
			return id;
		}

		MapKingdomId MapLandmass::getKingdomId(void) const {
			return kingdomId;
		}

		uint32_t MapLandmass::getArea(void) const {
			return area;
		}

		bool MapLandmass::getIsWater(void) const {
			return isWater;
		}

		uint16_t MapLandmass::getTileExampleX(void) const {
			return tileExampleX;
		}

		uint16_t MapLandmass::getTileExampleY(void) const {
			return tileExampleY;
		}

		void MapLandmass::setKingdomId(MapKingdomId id) {
			kingdomId=id;
		}
	};
};
