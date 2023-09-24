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

			tileMinX=UINT16_MAX;
			tileMinY=UINT16_MAX;
			tileMaxX=0;
			tileMaxY=0;
			tileAverageX=0;
			tileAverageY=0;
			tileExampleX=0;
			tileExampleY=0;
		}

		MapLandmass::MapLandmass(Id id, uint32_t area, uint16_t tileMinX, uint16_t tileMinY, uint16_t tileMaxX, uint16_t tileMaxY, uint16_t tileAverageX, uint16_t tileAverageY, uint16_t tileExampleX, uint16_t tileExampleY): id(id), area(area), tileMinX(tileMinX), tileMinY(tileMinY), tileMaxX(tileMaxX), tileMaxY(tileMaxY), tileAverageX(tileAverageX), tileAverageY(tileAverageY), tileExampleX(tileExampleX) {
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
			result&=(fread(&tileMinX, sizeof(tileMinX),  1, file)==1);
			result&=(fread(&tileMinY, sizeof(tileMinY),  1, file)==1);
			result&=(fread(&tileMaxX, sizeof(tileMaxX),  1, file)==1);
			result&=(fread(&tileMaxY, sizeof(tileMaxY),  1, file)==1);
			result&=(fread(&tileAverageX, sizeof(tileAverageX),  1, file)==1);
			result&=(fread(&tileAverageY, sizeof(tileAverageY),  1, file)==1);
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
			result&=(fwrite(&tileMinX, sizeof(tileMinX),  1, file)==1);
			result&=(fwrite(&tileMinY, sizeof(tileMinY),  1, file)==1);
			result&=(fwrite(&tileMaxX, sizeof(tileMaxX),  1, file)==1);
			result&=(fwrite(&tileMaxY, sizeof(tileMaxY),  1, file)==1);
			result&=(fwrite(&tileAverageX, sizeof(tileAverageX),  1, file)==1);
			result&=(fwrite(&tileAverageY, sizeof(tileAverageY),  1, file)==1);
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

		uint16_t MapLandmass::getTileMinX(void) const {
			return tileMinX;
		}

		uint16_t MapLandmass::getTileMinY(void) const {
			return tileMinY;
		}

		uint16_t MapLandmass::getTileMaxX(void) const {
			return tileMaxX;
		}

		uint16_t MapLandmass::getTileMaxY(void) const {
			return tileMaxY;
		}

		uint16_t MapLandmass::getTileAverageX(void) const {
			return tileAverageX;
		}

		uint16_t MapLandmass::getTileAverageY(void) const {
			return tileAverageY;
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
