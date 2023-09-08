#ifndef ENGINE_GRAPHICS_MAP_H
#define ENGINE_GRAPHICS_MAP_H

#include <mutex>
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

			static const unsigned regionsSize=256; // numbers of regions per side, with total number of regions equal to regionsSize squared

			Map(const char *mapBaseDirPath, unsigned mapWidth, unsigned mapHeight); // creates a new map, must not exist already. width and height are rounded up to a non-zero multiple of MapRegion::tilesSize, and are capped at Map::regionsSize*MapRegion::tilesSize.
			Map(const char *mapBaseDirPath, bool ignoreLock); // loads an existing map
			~Map();

			bool save(void); // Saves everything recursively.
			bool saveMetadata(void) const; // Creates directories.
			bool saveTextures(void) const; // Only saves list of textures (requires directory exists).
			bool saveItems(void) const; // Only saves list of item 'definitions' (requires directory exists).
			bool saveRegions(void); // Only saves regions (requires directory exists).

			bool loadRegion(unsigned regionX, unsigned regionY, const char *regionPath);
			bool markRegionDirtyAtTileOffset(unsigned offsetX, unsigned offsetY, bool create);

			void tick(void);

			unsigned getWidth(void) const;
			unsigned getHeight(void) const;

			MapTile *getTileAtCoordVec(const CoordVec &vec, GetTileFlag flags);
			MapTile *getTileAtOffset(unsigned offsetX, unsigned offsetY, GetTileFlag flags);
			MapRegion *getRegionAtCoordVec(const CoordVec &vec, bool create);
			MapRegion *getRegionAtOffset(unsigned regionX, unsigned regionY, bool create);

			bool addObject(MapObject *object);
			bool moveObject(MapObject *object, const CoordVec &newPos);

			bool addTexture(MapTexture *texture); // These functions will free texture later.
			void removeTexture(unsigned id);
			const MapTexture *getTexture(unsigned id) const;

			bool addItem(MapItem *item); // These functions will free item later.
			void removeItem(unsigned id);
			const MapItem *getItem(unsigned id) const;

			const char *getBaseDir(void) const;
			const char *getMapTiledDir(void) const;

			unsigned wrappingDistX(unsigned x1, unsigned x2); // distance between x1 and x2 (considers wrapping around the edge as an option)
			unsigned wrappingDistY(unsigned y1, unsigned y2);
			unsigned wrappingDist(unsigned x1, unsigned y1, unsigned x2, unsigned y2); // sum of x and y distances (e.g. manhattan/taxicab distance)

			unsigned addTileOffsetX(unsigned offsetX, int dx); // 0<=offsetX<map width, -map width<dx<map width
			unsigned addTileOffsetY(unsigned offsetY, int dy); // 0<=offsetY<map height, -map height<dy<map height

			// These need to be recalculated manually (e.g. by calling MapGen::recalculateStats).
			double minHeight, maxHeight;
			double minTemperature, maxTemperature;
			double minMoisture, maxMoisture;

			// These are similar but are not covered by the above function.
			double seaLevel, alpineLevel, forestLevel;
		private:
			static const unsigned regionsLoadedMax=32; // TODO: Decide this better

			int lockFd;

			unsigned mapWidth, mapHeight; // these should both be multiples of MapRegion::tilesSize

			char *baseDir;
			char *texturesDir;
			char *itemsDir;
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
			RegionData regionsByOffset[regionsSize][regionsSize]; // [y][x]
			std::mutex regionsLock;

			MapTexture *textures[MapTexture::IdMax];

			MapItem *items[MapItem::IdMax];

			const char *getRegionsDir(void) const;
			const char *getTexturesDir(void) const;
			const char *getItemsDir(void) const;

			MapRegion *getRegionAtIndex(unsigned index);
			const MapRegion *getRegionAtIndex(unsigned index) const;

			void updateRegionAge(const MapRegion *region);

			void regionUnload(unsigned index);
		};
	};
};

#endif
