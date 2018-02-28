#ifndef ENGINE_GRAPHICS_MAPTILE_H
#define ENGINE_GRAPHICS_MAPTILE_H

#include "mapobject.h"
#include "../physics/hitmask.h"

namespace Engine {
	namespace Map {

		class MapTile {
		public:
			static const unsigned layersMax=4;
			static const unsigned objectsMax=8;

			struct Layer {
				MapTexture::Id textureId;
			};

			struct FileData {
				Layer layers[layersMax];
				double height, moisture;
			};

			MapTile();
			~MapTile();

			void setFileData(FileData *fileData);

			Layer *getLayer(unsigned z);
			const Layer *getLayer(unsigned z) const;
			const Layer *getLayers(void) const;
			const MapObject *getObject(unsigned n) const;
			unsigned getObjectCount(void) const;
			double getHeight(void) const;
			double getMoisture(void) const;

			Physics::HitMask getHitMask(const CoordVec &tilePos) const;

			void setLayer(unsigned z, const Layer &layer);
			void setHeight(double height);
			void setMoisture(double moisture);

			bool addObject(MapObject *object);
			void removeObject(MapObject *object);
			bool isObjectsFull(void) const;
		private:
			FileData *fileData;

			MapObject *objects[objectsMax];
			unsigned objectsNext;
		};
	};
};

#endif
