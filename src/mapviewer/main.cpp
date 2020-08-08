#include <cstdio>
#include <cstdlib>

#include "../engine/util.h"

#include "slippymap.h"

void slippyMapGenAllProgressFunctor(double progress, void *userData) {
	Util::clearConsoleLine();
	printf("Generating tile images... %.2f%%", progress*100.0);
	fflush(stdout);
}

int main(int argc, char *argv[]) {
	// Grab arguments.
	if (argc!=3) {
		printf("Usage: %s mappath mapsize\n", argv[0]);
		return EXIT_FAILURE;
	}
	const char *mapPath=argv[1];
	int mapSize=atoi(argv[2]);

	// Create slippy map instance.
	MapViewer::SlippyMap slippyMap(mapPath, mapSize, "./slippymapdata");

	// Generate slippy map tiles.
	slippyMap.genAll(&slippyMapGenAllProgressFunctor, NULL);
	printf("\n");

	return 0;
}
