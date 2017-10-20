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
			static const unsigned layersMax=4;
			static const unsigned objectsMax=8;

			MapTile();
			MapTile(unsigned textureId);
			MapTile(unsigned textureId, unsigned layer);
			~MapTile();

			const MapTileLayer *getLayer(unsigned z) const;
			const MapObject *getObject(unsigned n) const;
			unsigned getObjectCount(void) const;

			Physics::HitMask getHitMask(const CoordVec &tilePos) const;

			void setLayer(unsigned z, const MapTileLayer &layer);

			bool addObject(MapObject *object);
			void removeObject(MapObject *object);
			bool isObjectsFull(void) const;
		private:
			MapTileLayer layers[layersMax];

			MapObject *objects[objectsMax];
			unsigned objectsNext;
		};
	};
};

#endif
