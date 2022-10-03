#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <new>

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
	printf("Loading map at '%s'...\n", mapPath);

	class Map *map;
	try {
		map=new class Map(mapPath);
	} catch (std::exception& e) {
		std::cout << "Could not load map: " << e.what() << '\n';
		return EXIT_FAILURE;
	}

	// Work out the various zoom parameters
	unsigned mapWidth=map->getWidth();
	unsigned mapHeight=map->getHeight();
	unsigned mapSize=std::max(mapWidth, mapHeight);

	unsigned slippyZoomOffset=MapTiled::maxZoom-1-std::ceil(std::log2(mapSize/MapTiled::imageSize));
	unsigned slippyMaxNativeZoom=MapTiled::maxZoom-1-slippyZoomOffset;

	printf("Map is %u tiles wide and %u tiles high, giving size %u with zoom offset %u.\n", mapWidth, mapHeight, mapSize, slippyZoomOffset);

	// Generate slippymap.js file to pass on map-speicifc parameters such as min/max zoom
	char slippymapJsPath[1024]; // TODO: better
	sprintf(slippymapJsPath, "%s/slippymap.js", map->getBaseDir());

	printf("Creating slippymap.js file at '%s'...\n", slippymapJsPath);

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

	fprintf(slippymapJs, "var layerBase=L.tileLayer('maptiled/{z}/{x}/{y}-0.png', {\n");
	fprintf(slippymapJs, "	attribution: 'me',\n");
	fprintf(slippymapJs, "	minZoom: 2,\n");
	fprintf(slippymapJs, "	maxZoom: %u,\n", slippyMaxNativeZoom+4);
	fprintf(slippymapJs, "	minNativeZoom: 0,\n");
	fprintf(slippymapJs, "	maxNativeZoom: %u,\n", slippyMaxNativeZoom);
	fprintf(slippymapJs, "	zoomOffset: %u,\n", slippyZoomOffset);
	fprintf(slippymapJs, "	errorTileUrl: 'maptiled/blank.png'\n");
	fprintf(slippymapJs, "});\n");
	fprintf(slippymapJs, "layerBase.addTo(map);\n");
	fprintf(slippymapJs, "\n");

	fprintf(slippymapJs, "var layerTemperature=L.tileLayer('maptiled/{z}/{x}/{y}-1.png', {\n");
	fprintf(slippymapJs, "	attribution: 'me',\n");
	fprintf(slippymapJs, "	minZoom: 2,\n");
	fprintf(slippymapJs, "	maxZoom: %u,\n", slippyMaxNativeZoom+4);
	fprintf(slippymapJs, "	minNativeZoom: 0,\n");
	fprintf(slippymapJs, "	maxNativeZoom: %u,\n", slippyMaxNativeZoom);
	fprintf(slippymapJs, "	zoomOffset: %u,\n", slippyZoomOffset);
	fprintf(slippymapJs, "	errorTileUrl: 'maptiled/blank.png'\n");
	fprintf(slippymapJs, "});\n");
	fprintf(slippymapJs, "\n");

	fprintf(slippymapJs, "var layerTerrain=L.tileLayer('maptiled/{z}/{x}/{y}-2.png', {\n");
	fprintf(slippymapJs, "	attribution: 'me',\n");
	fprintf(slippymapJs, "	minZoom: 2,\n");
	fprintf(slippymapJs, "	maxZoom: %u,\n", slippyMaxNativeZoom+4);
	fprintf(slippymapJs, "	minNativeZoom: 0,\n");
	fprintf(slippymapJs, "	maxNativeZoom: %u,\n", slippyMaxNativeZoom);
	fprintf(slippymapJs, "	zoomOffset: %u,\n", slippyZoomOffset);
	fprintf(slippymapJs, "	errorTileUrl: 'maptiled/blank.png'\n");
	fprintf(slippymapJs, "});\n");
	fprintf(slippymapJs, "\n");

	fprintf(slippymapJs, "var layerHumidity=L.tileLayer('maptiled/{z}/{x}/{y}-3.png', {\n");
	fprintf(slippymapJs, "	attribution: 'me',\n");
	fprintf(slippymapJs, "	minZoom: 2,\n");
	fprintf(slippymapJs, "	maxZoom: %u,\n", slippyMaxNativeZoom+4);
	fprintf(slippymapJs, "	minNativeZoom: 0,\n");
	fprintf(slippymapJs, "	maxNativeZoom: %u,\n", slippyMaxNativeZoom);
	fprintf(slippymapJs, "	zoomOffset: %u,\n", slippyZoomOffset);
	fprintf(slippymapJs, "	errorTileUrl: 'maptiled/blank.png'\n");
	fprintf(slippymapJs, "});\n");
	fprintf(slippymapJs, "\n");

	fprintf(slippymapJs, "var layerContour=L.tileLayer('maptiled/{z}/{x}/{y}-4.png', {\n");
	fprintf(slippymapJs, "	attribution: 'me',\n");
	fprintf(slippymapJs, "	minZoom: 2,\n");
	fprintf(slippymapJs, "	maxZoom: %u,\n", slippyMaxNativeZoom+4);
	fprintf(slippymapJs, "	minNativeZoom: 0,\n");
	fprintf(slippymapJs, "	maxNativeZoom: %u,\n", slippyMaxNativeZoom);
	fprintf(slippymapJs, "	zoomOffset: %u,\n", slippyZoomOffset);
	fprintf(slippymapJs, "	errorTileUrl: 'maptiled/blank.png'\n");
	fprintf(slippymapJs, "});\n");
	fprintf(slippymapJs, "\n");

	fprintf(slippymapJs, "var layerPolitical=L.tileLayer('maptiled/{z}/{x}/{y}-5.png', {\n");
	fprintf(slippymapJs, "	attribution: 'me',\n");
	fprintf(slippymapJs, "	minZoom: 2,\n");
	fprintf(slippymapJs, "	maxZoom: %u,\n", slippyMaxNativeZoom+4);
	fprintf(slippymapJs, "	minNativeZoom: 0,\n");
	fprintf(slippymapJs, "	maxNativeZoom: %u,\n", slippyMaxNativeZoom);
	fprintf(slippymapJs, "	zoomOffset: %u,\n", slippyZoomOffset);
	fprintf(slippymapJs, "	errorTileUrl: 'maptiled/blank.png'\n");
	fprintf(slippymapJs, "});\n");
	fprintf(slippymapJs, "\n");

	fprintf(slippymapJs, "var layerRegionGrid=L.tileLayer('maptiled/{z}/{x}/{y}-6.png', {\n");
	fprintf(slippymapJs, "	attribution: 'me',\n");
	fprintf(slippymapJs, "	minZoom: 2,\n");
	fprintf(slippymapJs, "	maxZoom: %u,\n", slippyMaxNativeZoom+4);
	fprintf(slippymapJs, "	minNativeZoom: 0,\n");
	fprintf(slippymapJs, "	maxNativeZoom: %u,\n", slippyMaxNativeZoom);
	fprintf(slippymapJs, "	zoomOffset: %u,\n", slippyZoomOffset);
	fprintf(slippymapJs, "	errorTileUrl: 'maptiled/blank.png'\n");
	fprintf(slippymapJs, "});\n");
	fprintf(slippymapJs, "\n");

	fprintf(slippymapJs, "var baseLayers={\n");
	fprintf(slippymapJs, "	\"Base\": layerBase,\n");
	fprintf(slippymapJs, "	\"Temperature\": layerTemperature,\n");
	fprintf(slippymapJs, "	\"Terrain\": layerTerrain,\n");
	fprintf(slippymapJs, "	\"Humidity\": layerHumidity,\n");
	fprintf(slippymapJs, "	\"Political\": layerPolitical,\n");
	fprintf(slippymapJs, "};\n");
	fprintf(slippymapJs, "var overlays={\n");
	fprintf(slippymapJs, "	\"Contours\": layerContour,\n");
	fprintf(slippymapJs, "	\"Regions\": layerRegionGrid,\n");
	fprintf(slippymapJs, "};\n");
	fprintf(slippymapJs, "L.control.layers(baseLayers, overlays).addTo(map);\n");
	fprintf(slippymapJs, "\n");

	fprintf(slippymapJs, "map.fitWorld();\n");
	fprintf(slippymapJs, "\n");

	fclose(slippymapJs);

	// Generate all needed images
	MapTiled::ImageLayerSet imageLayerSet=MapTiled::ImageLayerSetAll;
	if (!MapTiled::generateImage(map, slippyZoomOffset, 0, 0, imageLayerSet, &mapTiledGenerateImageProgressString, (void *)"Generating slippymap images... ")) { // ..... improve string
		printf("\nCould not generate all images\n");
		delete map;
		return EXIT_FAILURE;
	}
	printf("\n");

	// Tidy up.
	delete map;

	return 0;
}
