#include <cassert>
#include <cmath>
#include <cstdbool>
#include <cstdio>
#include <cstdlib>

#include "../engine/map/map.h"
#include "../engine/map/mapgen.h"
#include "../engine/map/mapobject.h"
#include "../engine/util.h"

using namespace Engine;
using namespace Engine::Map;

int main(int argc, char **argv) {
	// Grab arguments.
	if (argc!=2) {
		printf("Usage: %s mappath\n", argv[0]);
		return EXIT_FAILURE;
	}

	const char *mapPath=argv[1];

	// Load map.
	printf("Loading map at '%s'.\n", mapPath);
	class Map *map=new class Map(mapPath);
	if (map==NULL || !map->initialized) {
		printf("Could not load map.\n");
		return EXIT_FAILURE;
	}
	printf("\n");

	// Tidy up.
	delete map;

	return EXIT_SUCCESS;
}
