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

bool demogenAddHouseTestFunctor(class Map *map, unsigned x, unsigned y, unsigned w, unsigned h, void *userData) {
	assert(map!=NULL);
	assert(map!=NULL);

	// Loop over tiles ensuring all have a suitable base layer.
	unsigned tx, ty;
	for(ty=0; ty<h; ++ty)
		for(tx=0; tx<w; ++tx) {
			const MapTile *tile=map->getTileAtCoordVec(CoordVec((x+tx)*Physics::CoordsPerTile, (y+ty)*Physics::CoordsPerTile));

			switch(tile->getLayer(0)->textureId) {
				case MapGen::TextureIdGrass0:
				case MapGen::TextureIdGrass1:
				case MapGen::TextureIdGrass2:
				case MapGen::TextureIdGrass3:
				case MapGen::TextureIdGrass4:
				case MapGen::TextureIdGrass5:
					// OK
				break;
				case MapGen::TextureIdDirt:
				case MapGen::TextureIdBrickPath:
				case MapGen::TextureIdDock:
				case MapGen::TextureIdWater:
				case MapGen::TextureIdTree1:
				case MapGen::TextureIdTree2:
				case MapGen::TextureIdHouseDoorBL:
				case MapGen::TextureIdHouseDoorBR:
				case MapGen::TextureIdHouseDoorTL:
				case MapGen::TextureIdHouseDoorTR:
				case MapGen::TextureIdHouseRoof:
				case MapGen::TextureIdHouseRoofTop:
				case MapGen::TextureIdHouseWall2:
				case MapGen::TextureIdHouseWall3:
				case MapGen::TextureIdHouseWall4:
				case MapGen::TextureIdHouseChimney:
				case MapGen::TextureIdHouseChimneyTop:
					// Bad
					return false;
				break;
				case MapGen::TextureIdNone:
				case MapGen::TextureIdMan1:
				case MapGen::TextureIdOldManN:
				case MapGen::TextureIdOldManE:
				case MapGen::TextureIdOldManS:
				case MapGen::TextureIdOldManW:
					// These shouldn't be in the base layer?
					return false;
				break;
				case MapGen::TextureIdNB:
					assert(false);
					return false;
				break;
				default:
					// TODO: Better
				break;
			}
		}

	return true;
}

int main(int argc, char **argv) {
	// Grab arguments.
	if (argc!=4) {
		printf("Usage: %s width height outputpath\n", argv[0]);
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
	class Map *map=new class Map(outputPath);
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
	if (!MapGen::generateWaterLand(map, 0, 0, width, height, MapGen::TextureIdWater, MapGen::TextureIdGrass0, 0, 0.3)) { // 30% land, 70% water
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

	/*
	// Add a test house.
	printf("Adding houses...\n");
	MapGen::addHouse(map, 950, 730, 10, 8, 4);
	MapGen::addHouse(map, 963, 725, 6, 11, 4);
	MapGen::addHouse(map, 965, 737, 8, 7, 4);
	MapGen::addHouse(map, 951, 740, 5, 5, 4);
	*/

	// Add a some test towns.
	printf("Adding towns...\n");
	MapGen::addTown(map, 780, 560, 980, 560, 4, &demogenAddHouseTestFunctor, NULL);
	MapGen::addTown(map, 2*259, 2*42, 2*259, 2*117, 4, &demogenAddHouseTestFunctor, NULL);
	MapGen::addTown(map, 2*808, 2*683, 2*1005, 2*683, 4, &demogenAddHouseTestFunctor, NULL);
	MapGen::addTown(map, 2*279, 2*837, 2*279, 2*900, 4, &demogenAddHouseTestFunctor, NULL);

	// Save map.
	if (!map->save()) {
		printf("Could not save map to '%s'.\n", outputPath);
		return EXIT_FAILURE;
	}
	printf("Saved map to '%s'.\n", outputPath);

	return EXIT_SUCCESS;
}
