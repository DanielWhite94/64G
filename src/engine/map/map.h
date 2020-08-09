#ifndef ENGINE_GRAPHICS_MAP_H
#define ENGINE_GRAPHICS_MAP_H

#include <vector>

#include "mapobject.h"
#include "mapregion.h"
#include "maptexture.h"
#include "maptile.h"
#include "../physics/coord.h"
#include "../util.h"

using namespace std;

using namespace Engine;
using namespace Engine::Map;
using namespace Engine::Physics;

namespace Engine {
	namespace Map {
		class Map {
		public:
			enum GetTileFlag {
				None=0,
				Create=1, // If the given coordinates point to a non-existant region, attempt to create it first.
				Dirty=2, // Mark the region containing this tile as dirty.
				CreateDirty=3,
			};

			static const unsigned regionsWide=256, regionsHigh=256;

			bool initialized;

			Map(const char *mapBaseDirPath);
			~Map();

			bool save(void); // Saves everything recursively.
			bool saveMetadata(void) const; // Creates directories.
			bool saveTextures(void) const; // Only saves list of textures (requires directory exists).
			bool saveRegions(void); // Only saves regions (requires directory exists).

			bool loadRegion(unsigned regionX, unsigned regionY, const char *regionPath);
			bool markRegionDirtyAtTileOffset(unsigned offsetX, unsigned offsetY, bool create);

			void tick(void);

			MapTile *getTileAtCoordVec(const CoordVec &vec, GetTileFlag flags);
			MapTile *getTileAtOffset(unsigned offsetX, unsigned offsetY, GetTileFlag flags);
			MapRegion *getRegionAtCoordVec(const CoordVec &vec, bool create);
			MapRegion *getRegionAtOffset(unsigned regionX, unsigned regionY, bool create);

			bool addObject(MapObject *object);
			bool moveObject(MapObject *object, const CoordVec &newPos);

			bool addTexture(MapTexture *texture); // These functions will free texture later.
			void removeTexture(unsigned id);
			const MapTexture *getTexture(unsigned id) const;

			const char *getBaseDir(void) const;
			const char *getMapTiledDir(void) const;

			bool calculateRegionWidthHeight(unsigned *regionsWide, unsigned *regionsHigh); // inspects which regions have been touched and finds the greatest x/y values

			// These need to be recalculated manually (e.g. by calling MapGen::recalculateStats).
			double minHeight, maxHeight;
			double minTemperature, maxTemperature;
			double minMoisture, maxMoisture;

			// These are similar but are not covered by the above function.
			double seaLevel, alpineLevel;
		private:
			static const unsigned regionsLoadedMax=32; // TODO: Decide this better

			int lockFd;

			char *baseDir;
			char *texturesDir;
			char *regionsDir;
			char *mapTiledDir;

			struct RegionData {
				MapRegion *ptr; // Pointer to region itself.
				unsigned index; // Index into regionsByIndex array.
				unsigned offsetX, offsetY; // Indicies into regionsByOffset array.
			};

			unsigned regionsCount;
			RegionData *regionsByIndex[regionsLoadedMax]; // These are pointers into regionsByOffset array.
			RegionData *regionsByAge[regionsLoadedMax]; // These are pointers into regionsByOffset array.
			RegionData regionsByOffset[regionsHigh][regionsWide];

			MapTexture *textures[MapTexture::IdMax];

			void initclean(void);

			const char *getRegionsDir(void) const;
			const char *getTexturesDir(void) const;

			MapRegion *getRegionAtIndex(unsigned index);
			const MapRegion *getRegionAtIndex(unsigned index) const;

			bool createBlankRegion(unsigned regionX, unsigned regionY);
			bool ensureSpaceForRegion(void); // If our regions array is full, evict something.
			void updateRegionAge(const MapRegion *region);

			void regionUnload(unsigned index);
		};
	};
};

#endif
