#include <cassert>
#include <cmath>
#include <cstdbool>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <png.h>

#include "../engine/map/map.h"
#include "../engine/map/mapgen.h"
#include "../engine/map/mapobject.h"
#include "../engine/map/mappnglib.h"
#include "../engine/util.h"

using namespace Engine;
using namespace Engine::Map;

int main(int argc, char **argv) {
	// Grab arguments.
	if (argc!=9 && argc!=10) {
		printf("Usage: %s [--quiet] mappath mapx mapy mapw maph imagewidth imageheight imagepath\n", argv[0]);
		return EXIT_FAILURE;
	}

	int arg=1;

	bool quiet=false;
	if (strcmp(argv[arg], "--quiet")==0) {
		quiet=true;
		++arg;
	}

	const char *mapPath=argv[arg++];
	int mapTileX=atoi(argv[arg++]);
	int mapTileY=atoi(argv[arg++]);
	int mapTileWidth=atoi(argv[arg++]);
	int mapTileHeight=atoi(argv[arg++]);
	int imageWidth=atoi(argv[arg++]);
	int imageHeight=atoi(argv[arg++]);
	const char *imagePath=argv[arg++];

	if (mapTileX<0 || mapTileX>=Engine::Map::Map::regionsWide*MapRegion::tilesWide) {
		if (!quiet)
			printf("Bad mapX '%i': must be at least zero and less than max map width (%i)\n", mapTileX, Engine::Map::Map::regionsWide*MapRegion::tilesWide);
		return EXIT_FAILURE;
	}

	if (mapTileY<0 || mapTileY>=Engine::Map::Map::regionsHigh*MapRegion::tilesHigh) {
		if (!quiet)
			printf("Bad mapY '%i': must be at least zero and less than max map height (%i)\n", mapTileY, Engine::Map::Map::regionsHigh*MapRegion::tilesHigh);
		return EXIT_FAILURE;
	}

	if (mapTileWidth<=0 || mapTileHeight<=0) {
		if (!quiet)
			printf("Bad map width or height '%i' and '%i': both must be greater than zero\n", mapTileWidth, mapTileHeight);
		return EXIT_FAILURE;
	}

	if (imageWidth<=0 || imageHeight<=0) {
		if (!quiet)
			printf("Bad image width or height '%i' and '%i': both must be greater than zero\n", imageWidth, imageHeight);
		return EXIT_FAILURE;
	}

	// Load map.
	if (!quiet)
		printf("Loading map at '%s'.\n", mapPath);
	class Map *map=new class Map(mapPath);
	if (map==NULL || !map->initialized) {
		if (!quiet)
			printf("Could not load map.\n");
		return EXIT_FAILURE;
	}

	// Use lib to do most of the work
	MapPngLib::generatePng(map, imagePath, mapTileX, mapTileY, mapTileWidth, mapTileHeight, imageWidth, imageHeight, quiet);

	// Tidy up.
	delete map;

	return EXIT_SUCCESS;
}
