#ifndef MAPPNG_MAPPNGLIB_H
#define MAPPNG_MAPPNGLIB_H

#include <cmath>
#include <cstdbool>
#include <cstdio>
#include <cstdlib>
#include <png.h>

#include "../engine/map/map.h"
#include "../engine/map/mapgen.h"
#include "../engine/map/mapobject.h"
#include "../engine/util.h"

using namespace Engine;
using namespace Engine::Map;

class MapPngLib {
public:
	static bool generatePng(class Map *map, const char *imagePath, int mapTileX, int mapTileY, int mapTileWidth, int mapTileHeight, int imageWidth, int imageHeight, bool quiet);
private:
	static void getColourForTile(const class Map *map, const MapTile *tile, uint8_t *r, uint8_t *g, uint8_t *b);
};

#endif
