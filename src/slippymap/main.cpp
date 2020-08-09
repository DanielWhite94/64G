#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "../engine/map/map.h"
#include "../engine/map/maptiled.h"

int main(int argc, char *argv[]) {
	// Grab arguments.
	if (argc!=2) {
		printf("Usage: %s mappath\n", argv[0]);
		return EXIT_FAILURE;
	}
	const char *mapPath=argv[1];

	// Load map
	printf("Loading map at '%s'.\n", mapPath);
	class Map *map=new class Map(mapPath);
	if (map==NULL || !map->initialized) {
		printf("Could not load map.\n");
		return EXIT_FAILURE;
	}

	// Calculate size of this map
	unsigned regionsWide, regionsHigh;
	if (!map->calculateRegionWidthHeight(&regionsWide, &regionsHigh)) {
		printf("Could not get map size\n");
		return EXIT_FAILURE;
	}

	// Work out the minimum zoom level which encompasses the entire map
	unsigned mapSize=std::max(regionsWide*MapRegion::tilesWide, regionsHigh*MapRegion::tilesHigh); // in tile units
	unsigned minZoom=MapTiled::maxZoom-1-std::ceil(std::log2(mapSize/MapTiled::imageSize));

	// Generate slippymap.js file to pass on map-speicifc parameters such as min/max zoom
	char slippymapJsPath[1024]; // TODO: better
	sprintf(slippymapJsPath, "%s/slippymap.js", map->getBaseDir());
	FILE *slippymapJs=fopen(slippymapJsPath, "w");
	if (slippymapJs==NULL) {
		printf("Could not create custom slippymap.js file at '%s'\n", slippymapJsPath);
		delete map;
		return EXIT_FAILURE;
	}

	fprintf(slippymapJs, "var map=L.map('mapid', {\n");
	fprintf(slippymapJs, "	zoomDelta: 0.25,\n");
	fprintf(slippymapJs, "	zoomSnap: 0,\n");
	fprintf(slippymapJs, "});\n");
	fprintf(slippymapJs, "\n");
	fprintf(slippymapJs, "L.tileLayer('maptiled/{z}/{x}/{y}.png', {\n");
	fprintf(slippymapJs, "	attribution: 'me',\n");
	fprintf(slippymapJs, "	minZoom: 0,\n");
	fprintf(slippymapJs, "	maxZoom: %u,\n", MapTiled::maxZoom-1-minZoom);
	fprintf(slippymapJs, "	zoomOffset: %u,\n", minZoom);
	fprintf(slippymapJs, "	noWrap: true,\n");
	fprintf(slippymapJs, "	errorTileUrl: 'maptiled/blank.png'\n");
	fprintf(slippymapJs, "}).addTo(map);\n");
	fprintf(slippymapJs, "\n");
	fprintf(slippymapJs, "map.fitWorld();\n");

	fclose(slippymapJs);

	// Generate all needed images
	if (!MapTiled::generateTileMap(map, minZoom, 0, 0, MapTiled::maxZoom, false, NULL)) {
		printf("Could not generate all images\n");
		return EXIT_FAILURE;
	}

	// Tidy up.
	delete map;

	return 0;
}
