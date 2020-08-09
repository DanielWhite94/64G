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

			// The conditions following are considered in the order listed.
			// If the image already exists, nothing is done.
			// If the zoom level is the max then we generate the image directly with MapPngLib.
			// If all four children exist, we scale and stitch them to create the desired image.
			// If zoom>=minZoomToGen then we generate the image directly with MapPngLib.
			// Finally we recurse to generate children, before stitching together as described above.
			// If genOnce is true then in the case of recursing to generate children,
			// we will stop after generating a single image with MapPngLib, and return false.
			static bool generateImage(class Map *map, unsigned zoom, unsigned x, unsigned y, unsigned minZoomToGen, bool genOnce, bool *haveGen);

			static void getZoomPath(const class Map *map, unsigned zoom, char path[1024]); // TODO: improve hardcoded size
			static void getZoomXPath(const class Map *map, unsigned zoom, unsigned x, char path[1024]); // TODO: improve hardcoded size
			static void getZoomXYPath(const class Map *map, unsigned zoom, unsigned x, unsigned y, char path[1024]); // TODO: improve hardcoded size
			static void getBlankImagePath(const class Map *map, char path[1024]); // TODO: improve hardcoded size

		private:
		};
	};
};

#endif
