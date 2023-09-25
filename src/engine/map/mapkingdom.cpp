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

#include "mapkingdom.h"

namespace Engine {
	namespace Map {
		MapKingdom::MapKingdom() {
			id=MapKingdomIdNone;
		}

		MapKingdom::MapKingdom(MapKingdomId id): id(id) {
		}

		MapKingdom::~MapKingdom() {
		}

		bool MapKingdom::load(FILE *file) {
			assert(file!=NULL);

			bool result=true;

			result&=(fread(&id, sizeof(id), 1, file)==1);

			return result;
		}

		bool MapKingdom::save(FILE *file) const {
			assert(file!=NULL);

			bool result=true;

			result&=(fwrite(&id, sizeof(id), 1, file)==1);

			return result;
		}

		MapKingdomId MapKingdom::getId(void) const {
			return id;
		}

		uint32_t MapKingdom::getArea(void) const {
			uint32_t area=0;
			for(auto *landmass: landmasses)
				area+=landmass->getArea();

			return area;
		}

		unsigned MapKingdom::getAverageX(void) const {
			unsigned x, y;
			getAverageXY(&x, &y);
			return x;
		}

		unsigned MapKingdom::getAverageY(void) const {
			unsigned x, y;
			getAverageXY(&x, &y);
			return y;
		}

		void MapKingdom::getAverageXY(unsigned *averageX, unsigned *averageY) const {
			// Weighted average of landmass average x/y values
			uint64_t totalX=0, totalY=0, totalArea=0;
			for(auto *landmass: landmasses) {
				totalX+=landmass->getTileAverageX()*landmass->getArea();
				totalY+=landmass->getTileAverageY()*landmass->getArea();
				totalArea+=landmass->getArea();
			}

			*averageX=totalX/totalArea;
			*averageY=totalY/totalArea;
		}

		bool MapKingdom::addLandmass(MapLandmass *landmass) {
			assert(landmass->getKingdomId()==getId());

			landmasses.push_back(landmass);

			return true;
		}
	};
};
