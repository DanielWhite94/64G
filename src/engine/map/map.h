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

			Map();
			Map(const char *mapBaseDirPath);
			~Map();

			bool save(const char *mapBaseDirPath); // Saves everything recursively.
			bool saveMetadata(const char *mapBaseDirPath) const; // Creates directories.
			bool saveTextures(const char *mapBaseDirPath) const; // Only saves list of textures (requires directory exists).
			bool saveRegions(const char *mapBaseDirPath); // Only saves regions (requires directory exists).

			bool loadRegion(const char *regionPath);

			void tick(void);

			MapTile *getTileAtCoordVec(const CoordVec &vec);
			const MapTile *getTileAtCoordVec(const CoordVec &vec) const;
			MapRegion *getRegionAtCoordVec(const CoordVec &vec);
			const MapRegion *getRegionAtCoordVec(const CoordVec &vec) const;
			MapRegion *getRegionAtOffset(unsigned regionX, unsigned regionY);
			const MapRegion *getRegionAtOffset(unsigned regionX, unsigned regionY) const;

			void setTileAtCoordVec(const CoordVec &vec, const MapTile &tile);

			bool addObject(MapObject *object);
			bool moveObject(MapObject *object, const CoordVec &newPos);

			bool addTexture(MapTexture *texture); // These functions will free texture later.
			void removeTexture(unsigned id);
			const MapTexture *getTexture(unsigned id) const;
		private:
			static const unsigned regionsLoadedMax=9;

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

			static char *saveBaseDirToRegionsDir(const char *mapBaseDirPath);
			static char *saveBaseDirToTexturesDir(const char *mapBaseDirPath);
		};
	};
};

#endif
