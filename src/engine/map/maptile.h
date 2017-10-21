#ifndef ENGINE_GRAPHICS_MAPTILE_H
#define ENGINE_GRAPHICS_MAPTILE_H

#include "mapobject.h"
#include "../physics/hitmask.h"

namespace Engine {
	namespace Map {

		class MapTile {
		public:
			struct Layer {
				MapTexture::Id textureId;
			};

			static const unsigned layersMax=4;
			static const unsigned objectsMax=8;

			MapTile();
			MapTile(MapTexture::Id textureId);
			MapTile(MapTexture::Id textureId, unsigned layer);
			~MapTile();

			Layer *getLayer(unsigned z);
			const Layer *getLayer(unsigned z) const;
			const Layer *getLayers(void) const;
			const MapObject *getObject(unsigned n) const;
			unsigned getObjectCount(void) const;

			Physics::HitMask getHitMask(const CoordVec &tilePos) const;

			void setLayer(unsigned z, const Layer &layer);

			bool addObject(MapObject *object);
			void removeObject(MapObject *object);
			bool isObjectsFull(void) const;
		private:
			Layer layers[layersMax];

			MapObject *objects[objectsMax];
			unsigned objectsNext;
		};
	};
};

#endif
