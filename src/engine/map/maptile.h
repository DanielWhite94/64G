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
			static const unsigned landmassIdMax=65536;

			struct __attribute__((packed)) Layer  {
				MapTexture::Id textureId;
			};

			struct __attribute__((packed)) FileData {
				Layer layers[layersMax];
				Physics::HitMask hitmask;
				uint64_t bitset;
				float height, moisture, temperature;
				union {
					uint32_t scratchInt;
					float scratchFloat;
				};
				uint16_t landmassId;
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
			double getTemperature(void) const;
			uint64_t getBitset(void) const;
			bool getBitsetN(unsigned n) const;
			uint16_t getLandmassId(void) const;
			uint32_t getScratchInt(void) const;
			float getScratchFloat(void) const;
			Physics::HitMask getHitMask(const CoordVec &tilePos) const;

			void setLayer(unsigned z, const Layer &layer);
			void setHeight(double height);
			void setMoisture(double moisture);
			void setTemperature(double temperature);
			void setHitMask(Physics::HitMask hitmask);
			void setBitset(uint64_t bitset);
			void setBitsetN(unsigned n, bool value);
			void setLandmassId(uint16_t landmassId);
			void setScratchInt(uint32_t scratchInt);
			void setScratchFloat(float scratchFloat);

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
