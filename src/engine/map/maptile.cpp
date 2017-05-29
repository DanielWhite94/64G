#include <cassert>
#include <cstdlib>
#include <cstdio>

#include "maptexture.h"
#include "maptile.h"

using namespace Engine;
using namespace Engine::Map;

namespace Engine {
	namespace Map {
		MapTile::MapTile() {
			for(unsigned i=0; i<layersMax; ++i)
				layers[i].textureId=0;

			objectsNext=0;
		}

		MapTile::MapTile(unsigned textureId) {
			assert(textureId<MapTexture::IdMax);

			for(unsigned i=0; i<layersMax; ++i)
				layers[i].textureId=0;
			layers[0].textureId=textureId;

			objectsNext=0;
		}

		MapTile::MapTile(unsigned textureId, unsigned layer) {
			assert(textureId<MapTexture::IdMax);
			assert(layer<layersMax);

			for(unsigned i=0; i<layersMax; ++i)
				layers[i].textureId=0;
			layers[layer].textureId=textureId;

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

		Physics::HitMask MapTile::getHitMask(const CoordVec &tilePos) const {
			HitMask hitMask;
			unsigned i, max=getObjectCount();
			for(i=0; i<max; ++i)
				hitMask|=getObject(i)->getHitMaskByCoord(tilePos);
			return hitMask;
		}

		void MapTile::setLayer(unsigned z, const MapTileLayer &layer) {
			assert(z<layersMax);

			layers[z]=layer;
		}

		void MapTile::addObject(MapObject *object) {
			assert(object!=NULL);
			assert(objectsNext<objectsMax);

			objects[objectsNext++]=object;
		}

		void MapTile::removeObject(MapObject *object) {
			assert(object!=NULL);

			// Search for the object.
			unsigned i;
			for(i=0; i<objectsNext; ++i)
				if (objects[i]==object) {
					// Remove this object by overwriting it with the one at the end of the array.
					objects[i]=objects[--objectsNext];
					return;
				}

			// Object not found.
			assert(false);
		}
	};
};
