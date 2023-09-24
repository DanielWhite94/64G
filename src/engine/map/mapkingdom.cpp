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

		bool MapKingdom::addLandmass(MapLandmass *landmass) {
			assert(landmass->getKingdomId()==getId());

			landmasses.push_back(landmass);

			return true;
		}
	};
};
