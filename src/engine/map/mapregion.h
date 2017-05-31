#ifndef ENGINE_GRAPHICS_MAPREGION_H
#define ENGINE_GRAPHICS_MAPREGION_H

#include "maptile.h"
#include "../physics/coord.h"

using namespace Engine::Map;
using namespace Engine::Physics;

namespace Engine {
	namespace Map {
		class MapRegion {
		public:
			static const unsigned tilesWide=256, tilesHigh=256;

			MapRegion();
			~MapRegion();

			bool save(const char *regionsDirPath, unsigned regionX, unsigned regionY);

			static unsigned coordXToRegionXBase(CoordComponent x);
			static unsigned coordXToRegionXOffset(CoordComponent x);

			MapTile *getTileAtCoordVec(const CoordVec &vec);
			const MapTile *getTileAtCoordVec(const CoordVec &vec) const ;
			MapTile *getTileAtOffset(unsigned offsetX, unsigned offsetY);
			const MapTile *getTileAtOffset(unsigned offsetX, unsigned offsetY) const ;

			bool getIsDirty(void) const;

			void setTileAtCoordVec(const CoordVec &vec, const MapTile &tile);
		private:
			bool isDirty;

			MapTile tiles[tilesHigh][tilesWide];
		};
	};
};

#endif
