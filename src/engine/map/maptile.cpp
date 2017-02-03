#include <cassert>
#include <cstdlib>
#include <cstdio>

#include "maptile.h"

using namespace Engine;
using namespace Engine::Map;

namespace Engine {
	namespace Map {
		MapTile::MapTile() {
			unsigned i;
			for(i=0; i<layersMax; ++i)
				layers[i].textureId=0;

			objectsNext=0;
		}

		MapTile::MapTile(unsigned temp) {
			layers[0].textureId=temp;
			unsigned i;
			for(i=1; i<layersMax; ++i)
				layers[i].textureId=0;

			objectsNext=0;
		}

		MapTile::~MapTile() {

		}

		const MapTileLayer *MapTile::getLayer(unsigned z) const {
			assert(z<layersMax);
			return &layers[z];
		}

		const MapObject *MapTile::getObject(unsigned n) const {
			assert(n<getObjectCount());
			return objects[n];
		}

		unsigned MapTile::getObjectCount(void) const {
			return objectsNext;
		}

		void MapTile::addObject(MapObject *object) {
			assert(object!=NULL);
			assert(objectsNext<objectsMax);

			objects[objectsNext++]=object;
		}
	};
};
