#include <cstdio>
#include <cstdlib>

#include "slippymap.h"

int main (int argc, char *argv[]) {
	// Grab arguments.
	if (argc!=3) {
		printf("Usage: %s mappath mapsize\n", argv[0]);
		return EXIT_FAILURE;
	}
	const char *mapPath=argv[1];
	int mapSize=atoi(argv[2]);

	// Load map.
	printf("Loading map at '%s'.\n", mapPath);
	class Map *map=new class Map(mapPath);
	printf("\n");
	if (map==NULL || !map->initialized) {
		printf("Could not load map.\n");
		return EXIT_FAILURE;
	}

	// Create slippy map instance.
	MapViewer::SlippyMap slippyMap(map, mapSize, "slippymapdata");

	// Create slippy map image for whole world (causing all others to also be generated).
	slippyMap.getImageByZoom(0, 0, 0);

	// Tidy up.
	delete map;

	return 0;
}
