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
			fileData=NULL;
			objectsNext=0;
		}

		MapTile::~MapTile() {
		}

		void MapTile::setFileData(FileData *gFileData) {
			assert(fileData==NULL);
			assert(gFileData!=NULL);

			fileData=gFileData;
		}

		const MapTile::Layer *MapTile::getLayer(unsigned z) const {
			assert(fileData!=NULL);
			assert(z<layersMax);

			return &fileData->layers[z];
		}

		MapTile::Layer *MapTile::getLayer(unsigned z) {
			assert(fileData!=NULL);
			assert(z<layersMax);

			return &fileData->layers[z];
		}

		const MapTile::Layer *MapTile::getLayers(void) const {
			assert(fileData!=NULL);

			return fileData->layers;
		}

		const MapObject *MapTile::getObject(unsigned n) const {
			assert(n<getObjectCount());
			return objects[n];
		}

		unsigned MapTile::getObjectCount(void) const {
			return objectsNext;
		}

		double MapTile::getHeight(void) const {
			return fileData->height;
		}

		double MapTile::getMoisture(void) const {
			return fileData->moisture;
		}

		double MapTile::getTemperature(void) const {
			return fileData->temperature;
		}

		uint64_t MapTile::getBitset(void) const {
			return fileData->bitset;
		}

		bool MapTile::getBitsetN(unsigned n) const {
			assert(n<64);
			return (fileData->bitset>>n)&1;
		}

		uint16_t MapTile::getLandmassId(void) const {
			return fileData->landmassId;
		}

		uint32_t MapTile::getScratchInt(void) const {
			return fileData->scratchInt;
		}

		float MapTile::getScratchFloat(void) const {
			return fileData->scratchFloat;
		}

		Physics::HitMask MapTile::getHitMask(const CoordVec &tilePos) const {
			// Tile hitmask.
			HitMask hitMask=fileData->hitmask;

			// Add object hitmasks.
			unsigned objectCount=getObjectCount();
			for(unsigned i=0; i<objectCount; ++i)
				hitMask|=getObject(i)->getHitMaskByCoord(tilePos);

			return hitMask;
		}

		void MapTile::setLayer(unsigned z, const Layer &layer) {
			assert(fileData!=NULL);
			assert(z<layersMax);

			fileData->layers[z]=layer;
		}

		void MapTile::setHeight(double height) {
			fileData->height=height;
		}

		void MapTile::setMoisture(double moisture) {
			fileData->moisture=moisture;
		}

		void MapTile::setTemperature(double temperature) {
			fileData->temperature=temperature;
		}

		void MapTile::setHitMask(Physics::HitMask hitmask) {
			fileData->hitmask=hitmask;
		}

		void MapTile::setBitset(uint64_t bitset) {
			fileData->bitset=bitset;
		}

		void MapTile::setBitsetN(unsigned n, bool value) {
			assert(n<64);

			if (value)
				fileData->bitset|=(((uint64_t)1)<<n);
			else
				fileData->bitset&=~(((uint64_t)1)<<n);
		}

		void MapTile::setLandmassId(uint16_t landmassId) {
			fileData->landmassId=landmassId;
		}

		void MapTile::setScratchInt(uint32_t gScratchInt) {
			fileData->scratchInt=gScratchInt;
		}

		void MapTile::setScratchFloat(float gScratchFloat) {
			fileData->scratchFloat=gScratchFloat;
		}

		bool MapTile::addObject(MapObject *object) {
			assert(object!=NULL);

			// Already full?
			if (isObjectsFull())
				return false;

			// Add object.
			objects[objectsNext++]=object;

			return true;
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

		bool MapTile::isObjectsFull(void) const {
			return (objectsNext>=objectsMax);
		}
	};
};
