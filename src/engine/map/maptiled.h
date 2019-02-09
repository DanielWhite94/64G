#ifndef ENGINE_MAP_MAPTILED_H
#define ENGINE_MAP_MAPTILED_H

#include "map.h"
#include "maptexture.h"
#include "maptile.h"
#include "../util.h"

namespace Engine {
	namespace Map {
		class MapTiled {
		public:
			static const unsigned imageSize=256;
			static const unsigned pixelsPerTileAtMaxZoom=8;
			static const unsigned maxZoom=12; // zoom in range [0,maxZoom-1], equal to 1+log2((pixelsPerTileAtMaxZoom*Map::regionsWide*MapRegion::tilesWide)/imageSize)

			MapTiled();
			~MapTiled();

			static bool createDirs(const class Map *map);
			static void getZoomPath(const class Map *map, unsigned zoom, char path[1024]); // TODO: improve hardcoded size
			static void getZoomXPath(const class Map *map, unsigned zoom, unsigned x, char path[1024]); // TODO: improve hardcoded size
			static void getZoomXYPath(const class Map *map, unsigned zoom, unsigned x, unsigned y, char path[1024]); // TODO: improve hardcoded size

		private:
		};
	};
};

#endif
