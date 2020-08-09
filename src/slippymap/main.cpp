#include <cstdio>
#include <cstdlib>

#include "../engine/util.h"

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

	// TODO: this

	// Tidy up.
	delete map;


	return 0;
}
