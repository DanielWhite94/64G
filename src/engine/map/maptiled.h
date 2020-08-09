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
			static const unsigned pixelsPerTileAtMaxZoom=1; // TODO: consider retiring this - we probably always want 1:1 ratio between pixels and tiles at the most zoomed level
			static const unsigned maxZoom=9; // zoom in range [0,maxZoom-1], equal to 1+log2((Map::regionsSize*MapRegion::tilesSize)/imageSize)

			MapTiled();
			~MapTiled();

			static bool createMetadata(const class Map *map);

			// If the image already exists, nothing is done.
			// Otherwise if all children exist the image is stitched together from them.
			// Otherwise we compare zoom and minZoomToGen.
			// If zoom<minZoomToGen then we recurse to generate children before stitching,
			// but if zoom>=minZoomToGen then we simply generate the desired image directly.
			// If genOnce is true then in the case of recursing to generate children,
			// we will stop after generating a single image with mappnglib, and return false.
			static bool generateTileMap(class Map *map, unsigned zoom, unsigned x, unsigned y, unsigned minZoomToGen, bool genOnce, bool *haveGen);

			static void getZoomPath(const class Map *map, unsigned zoom, char path[1024]); // TODO: improve hardcoded size
			static void getZoomXPath(const class Map *map, unsigned zoom, unsigned x, char path[1024]); // TODO: improve hardcoded size
			static void getZoomXYPath(const class Map *map, unsigned zoom, unsigned x, unsigned y, char path[1024]); // TODO: improve hardcoded size
			static void getBlankImagePath(const class Map *map, char path[1024]); // TODO: improve hardcoded size

		private:
		};
	};
};

#endif
