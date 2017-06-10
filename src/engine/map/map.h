#ifndef ENGINE_GRAPHICS_MAP_H
#define ENGINE_GRAPHICS_MAP_H

#include <vector>

#include "mapobject.h"
#include "mapregion.h"
#include "maptexture.h"
#include "maptile.h"
#include "../physics/coord.h"

using namespace std;

using namespace Engine;
using namespace Engine::Map;
using namespace Engine::Physics;

namespace Engine {
	namespace Map {
		class Map {
		public:
			static const unsigned regionsWide=256, regionsHigh=256;

			bool initialized;

			Map(const char *mapBaseDirPath);
			~Map();

			bool save(void); // Saves everything recursively.
			bool saveMetadata(void) const; // Creates directories.
			bool saveTextures(void) const; // Only saves list of textures (requires directory exists).
			bool saveRegions(void); // Only saves regions (requires directory exists).

			bool loadRegion(unsigned regionX, unsigned regionY, const char *regionPath);

			void tick(void);

			MapTile *getTileAtCoordVec(const CoordVec &vec);
			MapRegion *getRegionAtCoordVec(const CoordVec &vec);
			MapRegion *getRegionAtOffset(unsigned regionX, unsigned regionY);

			void setTileAtCoordVec(const CoordVec &vec, const MapTile &tile);

			bool addObject(MapObject *object);
			bool moveObject(MapObject *object, const CoordVec &newPos);

			bool addTexture(MapTexture *texture); // These functions will free texture later.
			void removeTexture(unsigned id);
			const MapTexture *getTexture(unsigned id) const;
		private:
			static const unsigned regionsLoadedMax=32; // TODO: Decide this better

			char *baseDir;
			char *texturesDir;
			char *regionsDir;

			struct RegionData {
				MapRegion *ptr; // Pointer to region itself.
				unsigned index; // Index into regionsByIndex array.
				unsigned offsetX, offsetY; // Indicies into regionsByOffset array.
			};

			unsigned regionsByIndexNext;
			RegionData *regionsByIndex[regionsLoadedMax]; // These are pointers into regionsByOffset array.
			RegionData regionsByOffset[regionsHigh][regionsWide];

			vector<MapObject *> objects;

			MapTexture *textures[MapTexture::IdMax];

			void initclean(void);

			const char *getBaseDir(void) const;
			const char *getRegionsDir(void) const;
			const char *getTexturesDir(void) const;

			MapRegion *getRegionAtIndex(unsigned index);
			const MapRegion *getRegionAtIndex(unsigned index) const;

			bool createBlankRegion(unsigned regionX, unsigned regionY);
			bool ensureSpaceForRegion(void); // If our regions array is full, evict something.

			static bool isDir(const char *path);
		};
	};
};

#endif
