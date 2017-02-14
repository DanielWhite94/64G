#ifndef ENGINE_GRAPHICS_MAPTILE_H
#define ENGINE_GRAPHICS_MAPTILE_H

#include "mapobject.h"
#include "../physics/hitmask.h"

namespace Engine {
	namespace Map {
		struct MapTileLayer {
			unsigned textureId;
		};

		class MapTile {
		public:
			static const unsigned layersMax=16;
			static const unsigned objectsMax=16;

			MapTile();
			MapTile(unsigned temp); // TODO: Remove this.
			~MapTile();

			const MapTileLayer *getLayer(unsigned z) const;
			const MapObject *getObject(unsigned n) const;
			unsigned getObjectCount(void) const;

			Physics::HitMask getHitMask(const CoordVec &tilePos) const;

			void addObject(MapObject *object);
			void removeObject(MapObject *object);
		private:
			MapTileLayer layers[layersMax];

			MapObject *objects[objectsMax];
			unsigned objectsNext;
		};
	};
};

#endif
