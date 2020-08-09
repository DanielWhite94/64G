#ifndef ENGINE_GRAPHICS_MAPREGION_H
#define ENGINE_GRAPHICS_MAPREGION_H

#include <vector>

#include "maptile.h"
#include "../physics/coord.h"

using namespace Engine::Map;
using namespace Engine::Physics;

namespace Engine {
	namespace Map {
		class MapRegion {
		public:
			static const unsigned tilesSize=256; // numbers of tiles per side, with total number of tiles equal to tilesSize squared

			MapRegion(unsigned regionX, unsigned regionY);
			~MapRegion();

			bool load(const char *regionsDirPath);
			bool save(const char *regionsDirPath, unsigned regionX, unsigned regionY);

			static unsigned coordXToRegionXBase(CoordComponent x);
			static unsigned coordXToRegionXOffset(CoordComponent x);

			MapTile *getTileAtCoordVec(const CoordVec &vec);
			const MapTile *getTileAtCoordVec(const CoordVec &vec) const ;
			MapTile *getTileAtOffset(unsigned offsetX, unsigned offsetY);
			const MapTile *getTileAtOffset(unsigned offsetX, unsigned offsetY) const ;

			bool getIsDirty(void) const;

			void setDirty(void);

			bool addObject(MapObject *object);
			void ownObject(MapObject *object);
			void disownObject(MapObject *object);

			const unsigned regionX, regionY;

			std::vector<MapObject *> objects;
		private:
			bool isDirty;

			MapTile tileInstances[tilesSize][tilesSize]; // [y][x]
			MapTile::FileData tileFileData[tilesSize][tilesSize]; // [y][x]

			bool saveObjects(FILE *regionFile);
		};
	};
};

#endif
