#include <cassert>
#include <cmath>
#include <cstdbool>
#include <cstdio>
#include <cstdlib>

#include "../engine/noisearray.h"
#include "../engine/map/map.h"
#include "../engine/map/mapgen.h"
#include "../engine/map/mapobject.h"
#include "../engine/util.h"

using namespace Engine;
using namespace Engine::Map;

enum DemoGenTileLayer {
	DemoGenTileLayerGround,
	DemoGenTileLayerDecoration,
	DemoGenTileLayerHalf,
	DemoGenTileLayerFull,
};

struct DemogenFullForestModifyTilesData {
	NoiseArray *noiseArray;
};

/*
bool demogenForestTestFunctorGroundIsTexture(class Map *map, MapGen::BuiltinObject builtin, const CoordVec &position, void *userData) {
	assert(map!=NULL);

	MapTexture::Id textureId=(MapTexture::Id)(uintptr_t)userData;

	const MapTile *tile=map->getTileAtCoordVec(position);
	if (tile==NULL)
		return false;

	const MapTile::Layer *layer=tile->getLayer(DemoGenTileLayerGround);
	if (layer==NULL)
		return false;

	return (layer->textureId==textureId);
}
*/

/*
bool demogenTownTileTestFunctor(class Map *map, unsigned x, unsigned y, unsigned w, unsigned h, void *userData) {
	assert(map!=NULL);
	assert(map!=NULL);

	// Loop over tiles.
	unsigned tx, ty;
	for(ty=0; ty<h; ++ty)
		for(tx=0; tx<w; ++tx) {
			const MapTile *tile=map->getTileAtCoordVec(CoordVec((x+tx)*Physics::CoordsPerTile, (y+ty)*Physics::CoordsPerTile));

			// Look for grass ground layer.
			MapTexture::Id layerGround=tile->getLayer(DemoGenTileLayerGround)->textureId;
			if (layerGround<=MapGen::TextureIdGrass0 || layerGround>=MapGen::TextureIdGrass5)
				return false;

			// Look full obstacles.
			MapTexture::Id layerFull=tile->getLayer(DemoGenTileLayerFull)->textureId;
			if (layerFull!=MapGen::TextureIdNone)
				return false;
		}

	return true;
}
*/

/*
void addMixedForest(class Map *map, int x0, int y0, int x1, int y1) {
	assert(map!=NULL);
	assert(x0<=x1);
	assert(y0<=y1);

	MapGen::addBuiltinObjectForestWithTestFunctor(map, MapGen::BuiltinObject::Tree2, CoordVec(x0*Physics::CoordsPerTile, y0*Physics::CoordsPerTile), CoordVec((x1-x0)*Physics::CoordsPerTile, (y1-y0)*Physics::CoordsPerTile), CoordVec(6*Physics::CoordsPerTile, 6*Physics::CoordsPerTile), &demogenForestTestFunctorGroundIsTexture, (void *)(uintptr_t)MapGen::TextureIdGrass0);

	MapGen::addBuiltinObjectForestWithTestFunctor(map, MapGen::BuiltinObject::Tree1, CoordVec(x0*Physics::CoordsPerTile, y0*Physics::CoordsPerTile), CoordVec((x1-x0)*Physics::CoordsPerTile, (y1-y0)*Physics::CoordsPerTile), CoordVec(3*Physics::CoordsPerTile, 3*Physics::CoordsPerTile), &demogenForestTestFunctorGroundIsTexture, (void *)(uintptr_t)MapGen::TextureIdGrass0);

	MapGen::addBuiltinObjectForestWithTestFunctor(map, MapGen::BuiltinObject::Bush, CoordVec(x0*Physics::CoordsPerTile, y0*Physics::CoordsPerTile), CoordVec((x1-x0)*Physics::CoordsPerTile, (y1-y0)*Physics::CoordsPerTile), CoordVec(3*Physics::CoordsPerTile, 3*Physics::CoordsPerTile), &demogenForestTestFunctorGroundIsTexture, (void *)(uintptr_t)MapGen::TextureIdGrass0);
}
*/

MapGen::ModifyTilesManyEntry *demogenMakeModifyTilesManyEntryGroundWaterLand(int width, int height) {
	assert(width>0);
	assert(height>0);

	// Create noise.
	NoiseArray *noiseArray=new NoiseArray(width, height, 4096, 4096, 600.0, 16, &noiseArrayProgressFunctorString, (void *)"Water/land: generating height noise ");
	printf("\n");

	// Create user data.
	MapGen::GenerateBinaryNoiseModifyTilesData *groundModifyTilesData=(MapGen::GenerateBinaryNoiseModifyTilesData *)malloc(sizeof(MapGen::GenerateBinaryNoiseModifyTilesData));
	assert(groundModifyTilesData!=NULL); // TODO: better
	groundModifyTilesData->noiseArray=noiseArray;
	groundModifyTilesData->threshold=-0.12;
	groundModifyTilesData->lowTextureId=MapGen::TextureIdGrass0;
	groundModifyTilesData->highTextureId=MapGen::TextureIdWater;
	groundModifyTilesData->tileLayer=DemoGenTileLayerGround;

	// Create entry.
	MapGen::ModifyTilesManyEntry *entry=(MapGen::ModifyTilesManyEntry *)malloc(sizeof(MapGen::ModifyTilesManyEntry));
	entry->functor=&mapGenGenerateBinaryNoiseModifyTilesFunctor;
	entry->userData=groundModifyTilesData;

	return entry;
}

void demogenFullForestFullForestModifyTilesFunctor(class Map *map, unsigned x, unsigned y, void *userData) {
	assert(map!=NULL);
	assert(userData!=NULL);

	const DemogenFullForestModifyTilesData *data=(const DemogenFullForestModifyTilesData *)userData;

	// Compute factor.
	double randomValue=(rand()/((double)RAND_MAX));

	// Calculate height.
	double height=data->noiseArray->eval(x, y);

	// Cutoff based on height and random factor.
	const double thresholdLimit=1/15.0;
	double power=pow(2.0,128.0);
	double offset=0.1-log(1/thresholdLimit)/log(power);
	double exponent=height-offset;
	double threshold=pow(power, exponent);
	if (threshold>thresholdLimit)
		threshold=thresholdLimit;

	if (randomValue>threshold)
		return;

	// Grab tile.
	MapTile *tile=map->getTileAtOffset(x, y);
	if (tile==NULL)
		return;

	// Check ground layer.
	const MapTile::Layer *groundLayer=tile->getLayer(DemoGenTileLayerGround);
	MapTexture::Id groundId=groundLayer->textureId;
	if (groundId!=MapGen::TextureIdGrass0)
		return;

	// Update tile layer.
	MapTile::Layer layer={.textureId=MapGen::TextureIdTree1};
	tile->setLayer(DemoGenTileLayerFull, layer);
}

MapGen::ModifyTilesManyEntry *demogenMakeModifyTilesManyEntryFullForest(int width, int height) {
	assert(width>0);
	assert(height>0);

	// Create noise.
	NoiseArray *noiseArray=new NoiseArray(width, height, 1024, 1024, 200.0, 16, &noiseArrayProgressFunctorString, (void *)"Forest: generating noise ");
	printf("\n");

	// Create user data.
	DemogenFullForestModifyTilesData *fullModifyTilesData=(DemogenFullForestModifyTilesData *)malloc(sizeof(MapGen::GenerateBinaryNoiseModifyTilesData));
	assert(fullModifyTilesData!=NULL); // TODO: better
	fullModifyTilesData->noiseArray=noiseArray;

	// Create entry.
	MapGen::ModifyTilesManyEntry *entry=(MapGen::ModifyTilesManyEntry *)malloc(sizeof(MapGen::ModifyTilesManyEntry));
	entry->functor=&demogenFullForestFullForestModifyTilesFunctor;
	entry->userData=fullModifyTilesData;

	return entry;
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

	// Run modify tiles.
	size_t modifyTilesArrayCount=2;
	MapGen::ModifyTilesManyEntry *modifyTilesArray[modifyTilesArrayCount];
	modifyTilesArray[0]=demogenMakeModifyTilesManyEntryGroundWaterLand(width, height);
	modifyTilesArray[1]=demogenMakeModifyTilesManyEntryFullForest(width, height);

	const char *progressString="	generating tiles (ground+forests) ";
	MapGen::modifyTilesMany(map, 0, 0, width, height, modifyTilesArrayCount, modifyTilesArray, 128, &mapGenModifyTilesProgressString, (void *)progressString);
	printf("\n");

	size_t i;
	for(i=0; i<modifyTilesArrayCount; ++i)
		free(modifyTilesArray[i]);

	// Add a test NPC.
	/*
	printf("Adding an NPC...\n");
	MapObject *npc1=MapGen::addBuiltinObject(map, MapGen::BuiltinObject::OldBeardMan, CoordAngle0, CoordVec(200*Physics::CoordsPerTile, 523*Physics::CoordsPerTile));
	if (npc1!=NULL)
		npc1->setMovementModeConstantVelocity(CoordVec(2,1)); // east south east
	*/

	// Add forests.
	/*
	printf("Adding forests...\n");
	addMixedForest(map, 2*0, 2*686, 2*411, 2*1023);
	addMixedForest(map, 2*605, 2*0, 2*1023, 2*293);
	*/

	// Add houses.
	/*
	printf("Adding houses...\n");
	MapGen::addHouse(map, 950, 730, 10, 8, DemoGenTileLayerFull);
	MapGen::addHouse(map, 963, 725, 6, 11, DemoGenTileLayerFull);
	MapGen::addHouse(map, 965, 737, 8, 7, DemoGenTileLayerFull);
	MapGen::addHouse(map, 951, 740, 5, 5, DemoGenTileLayerFull);
	*/

	// Add towns.
	/*
	printf("Adding towns...\n");
	MapGen::addTown(map, 780, 560, 980, 560, DemoGenTileLayerFull, &demogenTownTileTestFunctor, NULL);
	MapGen::addTown(map, 2*259, 2*42, 2*259, 2*117, DemoGenTileLayerFull, &demogenTownTileTestFunctor, NULL);
	MapGen::addTown(map, 2*808, 2*683, 2*1005, 2*683, DemoGenTileLayerFull, &demogenTownTileTestFunctor, NULL);
	MapGen::addTown(map, 2*279, 2*837, 2*279, 2*900, DemoGenTileLayerFull, &demogenTownTileTestFunctor, NULL);
	*/

	// Save map.
	if (!map->save()) {
		printf("Could not save map to '%s'.\n", outputPath);
		return EXIT_FAILURE;
	}
	printf("Saved map to '%s'.\n", outputPath);

	return EXIT_SUCCESS;
}
