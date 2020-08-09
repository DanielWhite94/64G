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
		delete map;
		return EXIT_FAILURE;
	}

	// Work out the various zoom parameters
	unsigned mapSize=std::max(regionsWide*MapRegion::tilesSize, regionsHigh*MapRegion::tilesSize); // in tile units

	unsigned slippyZoomOffset=MapTiled::maxZoom-1-std::ceil(std::log2(mapSize/MapTiled::imageSize));
	unsigned slippyMaxNativeZoom=MapTiled::maxZoom-1-slippyZoomOffset;

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
	fprintf(slippymapJs, "	minZoom: 1,\n"); // z=0 is too zoomed out - the entire map is less than half the screen
	fprintf(slippymapJs, "	maxZoom: %u,\n", slippyMaxNativeZoom+4);
	fprintf(slippymapJs, "	minNativeZoom: 0,\n");
	fprintf(slippymapJs, "	maxNativeZoom: %u,\n", slippyMaxNativeZoom);
	fprintf(slippymapJs, "	zoomOffset: %u,\n", slippyZoomOffset);
	fprintf(slippymapJs, "	noWrap: true,\n");
	fprintf(slippymapJs, "	errorTileUrl: 'maptiled/blank.png'\n");
	fprintf(slippymapJs, "}).addTo(map);\n");
	fprintf(slippymapJs, "\n");
	fprintf(slippymapJs, "map.fitWorld();\n");

	fclose(slippymapJs);

	// Generate all needed images
	if (!MapTiled::generateImage(map, slippyZoomOffset, 0, 0, MapTiled::maxZoom-1, false, NULL)) {
		printf("Could not generate all images\n");
		return EXIT_FAILURE;
	}

	// Tidy up.
	delete map;

	return 0;
}
