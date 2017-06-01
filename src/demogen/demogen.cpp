#include <cassert>
#include <cstdbool>
#include <cstdlib>
#include <cstdio>

#include "../engine/map/map.h"
#include "../engine/map/mapgen.h"
#include "../engine/map/mapobject.h"

using namespace Engine;
using namespace Engine::Map;

bool demogenForestTestFunctorIsLand(class Map *map, MapGen::BuiltinObject builtin, const CoordVec &position, void *userData) {
	assert(map!=NULL);
	assert(userData==NULL);

	const MapTile *tile=map->getTileAtCoordVec(position);
	if (tile==NULL)
		return false;

	const MapTileLayer *layer=tile->getLayer(0);
	if (layer==NULL)
		return false;

	return (layer->textureId!=MapGen::TextureIdWater);
}

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
		printf("Bad width or height (%i and %i)", width, height);
		return EXIT_FAILURE;
	}

	// Create Map.
	printf("Creating map...\n");
	class Map *map=new class Map();
	if (map==NULL || !map->initialized) {
		printf("Could not create a map.\n");
		return EXIT_FAILURE;
	}

	// Add textures.
	printf("Creating textures...\n");
	if (!MapGen::addBaseTextures(map)) {
		printf("Could not add base textures.\n");
		return EXIT_FAILURE;
	}

	// Create base tile layer - water/grass.
	printf("Creating land/water...\n");
	if (!MapGen::generateWaterLand(map, 0, 0, width, height, MapGen::TextureIdWater, MapGen::TextureIdGrass0, 0)) {
		printf("Could not generate land/water.\n");
		return EXIT_FAILURE;
	}

	// Add a test NPC.
	printf("Adding an NPC...\n");
	MapObject *npc1=MapGen::addBuiltinObject(map, MapGen::BuiltinObject::OldBeardMan, CoordAngle0, CoordVec(200*Physics::CoordsPerTile, 523*Physics::CoordsPerTile));
	if (npc1!=NULL)
		npc1->setMovementModeConstantVelocity(CoordVec(2,1)); // east south east

	// Add a test forest.
	/*
	printf("Adding a few forests...\n");

	MapGen::addBuiltinObjectForestWithTestFunctor(map, MapGen::BuiltinObject::Bush, CoordVec(0*Physics::CoordsPerTile, 0*Physics::CoordsPerTile), CoordVec(width*Physics::CoordsPerTile, height*Physics::CoordsPerTile), CoordVec(3*Physics::CoordsPerTile, 3*Physics::CoordsPerTile), &demogenForestTestFunctorIsLand, NULL);

	MapGen::addBuiltinObjectForestWithTestFunctor(map, MapGen::BuiltinObject::Tree2, CoordVec(0*Physics::CoordsPerTile, 0*Physics::CoordsPerTile), CoordVec(width*Physics::CoordsPerTile, height*Physics::CoordsPerTile), CoordVec(6*Physics::CoordsPerTile, 6*Physics::CoordsPerTile), &demogenForestTestFunctorIsLand, NULL);

	MapGen::addBuiltinObjectForestWithTestFunctor(map, MapGen::BuiltinObject::Tree1, CoordVec(0*Physics::CoordsPerTile, 0*Physics::CoordsPerTile), CoordVec(width*Physics::CoordsPerTile, height*Physics::CoordsPerTile), CoordVec(3*Physics::CoordsPerTile, 3*Physics::CoordsPerTile), &demogenForestTestFunctorIsLand, NULL);
	*/

	// Save map.
	if (!map->save(outputPath)) {
		printf("Could not save map to '%s'.\n", outputPath);
		return EXIT_FAILURE;
	}
	printf("Saved map to '%s'.\n", outputPath);

	return EXIT_SUCCESS;
}
