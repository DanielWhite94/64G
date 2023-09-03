#ifndef ENGINE_GRAPHICS_MAPPNGLIB_H
#define ENGINE_GRAPHICS_MAPPNGLIB_H

#include <cmath>
#include <cstdbool>
#include <cstdio>
#include <cstdlib>
#include <png.h>

#include "map.h"
#include "mapobject.h"
#include "maptiled.h"
#include "../util.h"

using namespace Engine;
using namespace Engine::Map;

namespace Engine {
	namespace Map {
		class MapPngLib {
		public:
			static bool generatePng(class Map *map, const char *imagePath, int mapTileX, int mapTileY, int mapTileWidth, int mapTileHeight, int imageWidth, int imageHeight, MapTiled::ImageLayer layer, bool quiet);
		private:
			static void getColourForTile(const class Map *map, int mapTileX, int mapTileY, const MapTile *tile, MapTiled::ImageLayer layer, uint8_t *r, uint8_t *g, uint8_t *b, uint8_t *a);
			static void getColourForTileHeight(const class Map *map, int mapTileX, int mapTileY, const MapTile *tile, uint8_t *r, uint8_t *g, uint8_t *b, uint8_t *a);
			static void getColourForTileTemperature(const class Map *map, int mapTileX, int mapTileY, const MapTile *tile, uint8_t *r, uint8_t *g, uint8_t *b, uint8_t *a);
			static void getColourForTileMoisture(const class Map *map, int mapTileX, int mapTileY, const MapTile *tile, uint8_t *r, uint8_t *g, uint8_t *b, uint8_t *a);
			static void getColourForTileTexture(const class Map *map, int mapTileX, int mapTileY, const MapTile *tile, uint8_t *r, uint8_t *g, uint8_t *b, uint8_t *a);
			static void getColourForTileHeightContour(const class Map *map, int mapTileX, int mapTileY, const MapTile *tile, uint8_t *r, uint8_t *g, uint8_t *b, uint8_t *a);
			static void getColourForTilePath(const class Map *map, int mapTileX, int mapTileY, const MapTile *tile, uint8_t *r, uint8_t *g, uint8_t *b, uint8_t *a);
			static void getColourForTilePolitical(const class Map *map, int mapTileX, int mapTileY, const MapTile *tile, uint8_t *r, uint8_t *g, uint8_t *b, uint8_t *a);
			static void getColourForTileRegionGrid(const class Map *map, int mapTileX, int mapTileY, const MapTile *tile, uint8_t *r, uint8_t *g, uint8_t *b, uint8_t *a);
		};
	};
};

#endif
