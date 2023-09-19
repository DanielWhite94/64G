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
			static void getColourForTile(class Map *map, int mapTileX, int mapTileY, const MapTile *tile, MapTiled::ImageLayer layer, uint8_t *r, uint8_t *g, uint8_t *b, uint8_t *a);
			static void getColourForTileHeight(class Map *map, int mapTileX, int mapTileY, const MapTile *tile, uint8_t *r, uint8_t *g, uint8_t *b, uint8_t *a);
			static void getColourForTileTemperature(class Map *map, int mapTileX, int mapTileY, const MapTile *tile, uint8_t *r, uint8_t *g, uint8_t *b, uint8_t *a);
			static void getColourForTileMoisture(class Map *map, int mapTileX, int mapTileY, const MapTile *tile, uint8_t *r, uint8_t *g, uint8_t *b, uint8_t *a);
			static void getColourForTileTexture(class Map *map, int mapTileX, int mapTileY, const MapTile *tile, uint8_t *r, uint8_t *g, uint8_t *b, uint8_t *a);
			static void getColourForTileHeightContour(class Map *map, int mapTileX, int mapTileY, const MapTile *tile, uint8_t *r, uint8_t *g, uint8_t *b, uint8_t *a);
			static void getColourForTilePath(class Map *map, int mapTileX, int mapTileY, const MapTile *tile, uint8_t *r, uint8_t *g, uint8_t *b, uint8_t *a);
			static void getColourForTilePolitical(class Map *map, int mapTileX, int mapTileY, const MapTile *tile, uint8_t *r, uint8_t *g, uint8_t *b, uint8_t *a);
			static void getColourForTileRegionGrid(class Map *map, int mapTileX, int mapTileY, const MapTile *tile, uint8_t *r, uint8_t *g, uint8_t *b, uint8_t *a);
		};
	};
};

#endif
