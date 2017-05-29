#include <cassert>
#include <cstdbool>
#include <cstdlib>
#include <cstdio>

#include "../engine/map/map.h"
#include "../engine/map/mapgen.h"
#include "../engine/map/mapobject.h"

using namespace Engine;
using namespace Engine::Map;

int main(int argc, char **argv) {
	// Grab arguments.
	if (argc!=4) {
		printf("Usage: %s width height outputpath", argv[0]);
		return EXIT_FAILURE;
	}

	int width=atoi(argv[1]);
	int height=atoi(argv[2]);
	const char *outputPath=argv[3];

	if (width<=0 || height<=0) {
		printf("Usage: Bad width or height (%i and %i)", width, height);
		return EXIT_FAILURE;
	}

	// Generate a map.
	MapGen::MapGen gen(width, height);
	class Map *map=gen.generate();

	if (map==NULL || !map->initialized) {
		printf("Could not generate a map.\n");
		return EXIT_FAILURE;
	}
	printf("Map generated.\n");

	// Save map.
	if (!map->save(outputPath)) {
		printf("Could not save map to '%s'.\n", outputPath);
		return EXIT_FAILURE;
	}
	printf("Saved map to '%s'.\n", outputPath);

	return EXIT_SUCCESS;
}
