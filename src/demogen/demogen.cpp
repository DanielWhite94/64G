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
		printf("Could not generate a base map.\n");
		return EXIT_FAILURE;
	}
	printf("Base map generated.\n");

	// Add a test NPC.
	printf("Adding an NPC...\n");
	MapObject *npc1=MapGen::addBuiltinObject(map, MapGen::BuiltinObject::OldBeardMan, CoordAngle0, CoordVec(200*Physics::CoordsPerTile, 523*Physics::CoordsPerTile));
	if (npc1!=NULL)
		npc1->setMovementModeConstantVelocity(CoordVec(2,1)); // east south east

	// Add a test forest.
	printf("Adding a forest..\n");

	MapGen::addBuiltinObjectForest(map, MapGen::BuiltinObject::Bush, CoordVec(200*Physics::CoordsPerTile, 535*Physics::CoordsPerTile), CoordVec(80*Physics::CoordsPerTile, 23*Physics::CoordsPerTile), CoordVec(3*Physics::CoordsPerTile, 3*Physics::CoordsPerTile));

	MapGen::addBuiltinObjectForest(map, MapGen::BuiltinObject::Tree2, CoordVec(220*Physics::CoordsPerTile, 547*Physics::CoordsPerTile), CoordVec(40*Physics::CoordsPerTile, 12*Physics::CoordsPerTile), CoordVec(6*Physics::CoordsPerTile, 6*Physics::CoordsPerTile));

	MapGen::addBuiltinObjectForest(map, MapGen::BuiltinObject::Tree1, CoordVec(210*Physics::CoordsPerTile, 537*Physics::CoordsPerTile), CoordVec(60*Physics::CoordsPerTile, 18*Physics::CoordsPerTile), CoordVec(3*Physics::CoordsPerTile, 3*Physics::CoordsPerTile));

	// Save map.
	if (!map->save(outputPath)) {
		printf("Could not save map to '%s'.\n", outputPath);
		return EXIT_FAILURE;
	}
	printf("Saved map to '%s'.\n", outputPath);

	return EXIT_SUCCESS;
}
