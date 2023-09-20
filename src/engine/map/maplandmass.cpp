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
		MapLandmass::MapLandmass(Id gId, uint32_t gArea) {
			assert(gId<IdMax);

			id=gId;
			area=gArea;
		}

		MapLandmass::~MapLandmass() {
		}

		bool MapLandmass::load(FILE *file) {
			assert(file!=NULL);

			bool result=true;

			result&=(fread(&id, sizeof(id), 1, file)==1);
			result&=(fread(&area, sizeof(area), 1, file)==1);

			return result;
		}

		bool MapLandmass::save(FILE *file) const {
			assert(file!=NULL);

			bool result=true;

			result&=(fwrite(&id, sizeof(id), 1, file)==1);
			result&=(fwrite(&area, sizeof(area), 1, file)==1);

			return result;
		}

		MapLandmass::Id MapLandmass::getId(void) const {
			return id;
		}

		uint32_t MapLandmass::getArea(void) const {
			return area;
		}

	};
};
