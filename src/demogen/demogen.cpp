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

typedef struct {
	// Only these are computed initially.
	class Map *map;
	int width, height;

	// These are computed after ground water/land modify tiles stage.
	unsigned long long landCount, waterCount, totalCount;
	double landFraction;

	double landSqKm, peoplePerSqKm, totalPopulation;
} DemogenMapData;

struct DemogenFullForestModifyTilesData {
	NoiseArray *noiseArray;
};

typedef struct {
	DemogenMapData *mapData;

	NoiseArray *noiseArray;
} DemogenGroundWaterLandModifyTilesData;

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

bool demogenTownTileTestFunctor(class Map *map, int x, int y, int w, int h, void *userData) {
	assert(map!=NULL);

	// Loop over tiles.
	int tx, ty;
	for(ty=0; ty<h; ++ty)
		for(tx=0; tx<w; ++tx) {
			const MapTile *tile=map->getTileAtCoordVec(CoordVec((x+tx)*Physics::CoordsPerTile, (y+ty)*Physics::CoordsPerTile), false);
			if (tile==NULL)
				return false;

			// Look for grass ground layer.
			MapTexture::Id layerGround=tile->getLayer(DemoGenTileLayerGround)->textureId;
			if (layerGround<MapGen::TextureIdGrass0 || layerGround>MapGen::TextureIdGrass5)
				return false;

			// Look for road decoration.
			MapTexture::Id layerDecoration=tile->getLayer(DemoGenTileLayerDecoration)->textureId;
			if (layerDecoration==MapGen::TextureIdBrickPath || layerDecoration==MapGen::TextureIdDirt)
				return false;

			// Look for full obstacles.
			MapTexture::Id layerFull=tile->getLayer(DemoGenTileLayerFull)->textureId;
			if (layerFull!=MapGen::TextureIdNone)
				return false;
		}

	return true;
}

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

void demogenGroundWaterLandModifyTilesFunctor(class Map *map, unsigned x, unsigned y, void *userData) {
	assert(map!=NULL);
	assert(userData!=NULL);

	const DemogenGroundWaterLandModifyTilesData *data=(const DemogenGroundWaterLandModifyTilesData *)userData;
	DemogenMapData *mapData=data->mapData;
	const NoiseArray *noiseArray=data->noiseArray;

	// Calculate height.
	double height=noiseArray->eval(x, y);

	// Grab tile.
	MapTile *tile=map->getTileAtOffset(x, y, false);
	if (tile==NULL)
		return;

	MapTexture::Id textureId=MapGen::TextureIdNone;
	if (height>-0.12)
		textureId=MapGen::TextureIdWater;
	else if (height>-0.13)
		textureId=MapGen::TextureIdSand;
	else {
		double r=((double)rand())/RAND_MAX;
		if (r<0.80)
			textureId=MapGen::TextureIdGrass0;
		else if (r<0.85)
			textureId=MapGen::TextureIdGrass1;
		else if (r<0.90)
			textureId=MapGen::TextureIdGrass2;
		else if (r<0.95)
			textureId=MapGen::TextureIdGrass3;
		else
			textureId=MapGen::TextureIdGrass4;
	}
	assert(textureId!=MapGen::TextureIdNone);

	// Update tile layer.
	MapTile::Layer layer={.textureId=textureId};
	tile->setLayer(DemoGenTileLayerGround, layer);

	// Update map data.
	if (textureId==MapGen::TextureIdWater)
		++mapData->waterCount;
	else
		++mapData->landCount;
	++mapData->totalCount;
}

MapGen::ModifyTilesManyEntry *demogenMakeModifyTilesManyEntryGroundWaterLand(DemogenMapData *mapData) {
	assert(mapData!=NULL);

	// Create callback data.
	DemogenGroundWaterLandModifyTilesData *callbackData=(DemogenGroundWaterLandModifyTilesData *)malloc(sizeof(DemogenGroundWaterLandModifyTilesData));
	assert(callbackData!=NULL); // TODO: Better
	callbackData->mapData=mapData;

	// Create noise.
	callbackData->noiseArray=new NoiseArray(mapData->width, mapData->height, 2048, 2048, 600.0, 16, &noiseArrayProgressFunctorString, (void *)"Water/land: generating height noise ");
	printf("\n");

	// Create entry.
	MapGen::ModifyTilesManyEntry *entry=(MapGen::ModifyTilesManyEntry *)malloc(sizeof(MapGen::ModifyTilesManyEntry));
	entry->functor=&demogenGroundWaterLandModifyTilesFunctor;
	entry->userData=(void *)callbackData;

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
	MapTile *tile=map->getTileAtOffset(x, y, false);
	if (tile==NULL)
		return;

	// Check layers.
	if (tile->getLayer(DemoGenTileLayerGround)->textureId<MapGen::TextureIdGrass0 || tile->getLayer(DemoGenTileLayerGround)->textureId>MapGen::TextureIdGrass5)
		return;
	if (tile->getLayer(DemoGenTileLayerDecoration)->textureId!=MapGen::TextureIdNone)
		return;
	if (tile->getLayer(DemoGenTileLayerHalf)->textureId!=MapGen::TextureIdNone)
		return;
	if (tile->getLayer(DemoGenTileLayerFull)->textureId!=MapGen::TextureIdNone)
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
	DemogenMapData mapData={
	    .map=NULL,
	    .width=0,
	    .height=0,
	    .landCount=0,
	    .waterCount=0,
	    .totalCount=0,
	    .landFraction=0.0,
		.landSqKm=0.0,
		.peoplePerSqKm=0.0,
		.totalPopulation=0.0,
	};

	// Grab arguments.
	if (argc!=4) {
		printf("Usage: %s width height outputpath\n", argv[0]);
		return EXIT_FAILURE;
	}

	mapData.width=atoi(argv[1]);
	mapData.height=atoi(argv[2]);
	const char *outputPath=argv[3];

	if (mapData.width<=0 || mapData.height<=0) {
		printf("Bad width or height (%i and %i)", mapData.width, mapData.height);
		return EXIT_FAILURE;
	}

	// Create Map.
	printf("Creating map...\n");
	mapData.map=new class Map(outputPath);
	if (mapData.map==NULL || !mapData.map->initialized) {
		printf("Could not create a map.\n");
		return EXIT_FAILURE;
	}

	// Add textures.
	printf("Creating textures...\n");
	if (!MapGen::addBaseTextures(mapData.map)) {
		printf("Could not add base textures.\n");
		return EXIT_FAILURE;
	}

	// Run modify tiles.
	size_t modifyTilesArrayCount=2;
	MapGen::ModifyTilesManyEntry *modifyTilesArray[modifyTilesArrayCount];
	modifyTilesArray[0]=demogenMakeModifyTilesManyEntryGroundWaterLand(&mapData);
	modifyTilesArray[1]=demogenMakeModifyTilesManyEntryFullForest(mapData.width, mapData.height);

	const char *progressString="Generating tiles (ground+forests) ";
	MapGen::modifyTilesMany(mapData.map, 0, 0, mapData.width, mapData.height, modifyTilesArrayCount, modifyTilesArray, &mapGenModifyTilesProgressString, (void *)progressString);
	printf("\n");

	size_t i;
	for(i=0; i<modifyTilesArrayCount; ++i)
		free(modifyTilesArray[i]);

	// Compute more map data.
	if (mapData.totalCount>0)
		mapData.landFraction=((double)mapData.landCount)/mapData.totalCount;
	else
		mapData.landFraction=0.0;

	mapData.landSqKm=mapData.landCount/(1000.0*1000.0);
	mapData.peoplePerSqKm=150.0;
	mapData.totalPopulation=mapData.landSqKm*mapData.peoplePerSqKm;

	setlocale(LC_NUMERIC, "");
	printf("Land %'.1fkm^2, water %'.1fkm^2, land fraction %.2f%%\n", mapData.landSqKm, mapData.waterCount/(1000.0*1000.0), mapData.landFraction*100.0);
	printf("People per km^2 %.0f, total pop %.0f\n", mapData.peoplePerSqKm, mapData.totalPopulation);

	// Add a test NPC.
	/*
	printf("Adding an NPC...\n");
	MapObject *npc1=MapGen::addBuiltinObject(mapData.map, MapGen::BuiltinObject::OldBeardMan, CoordAngle0, CoordVec(200*Physics::CoordsPerTile, 523*Physics::CoordsPerTile));
	if (npc1!=NULL)
		npc1->setMovementModeConstantVelocity(CoordVec(2,1)); // east south east
	*/

	// Add forests.
	/*
	printf("Adding forests...\n");
	addMixedForest(mapData.map, 2*0, 2*686, 2*411, 2*1023);
	addMixedForest(mapData.map, 2*605, 2*0, 2*1023, 2*293);
	*/

	// Add houses.
	/*
	printf("Adding houses...\n");
	MapGen::addHouse(mapData.map, 950, 730, 10, 8, DemoGenTileLayerFull);
	MapGen::addHouse(mapData.map, 963, 725, 6, 11, DemoGenTileLayerFull);
	MapGen::addHouse(mapData.map, 965, 737, 8, 7, DemoGenTileLayerFull);
	MapGen::addHouse(mapData.map, 951, 740, 5, 5, DemoGenTileLayerFull);
	*/

	// Add towns.
	printf("Adding towns...\n");
	MapGen::addTowns(mapData.map, 0, 0, mapData.width, mapData.height, DemoGenTileLayerDecoration, DemoGenTileLayerFull, mapData.totalPopulation, &demogenTownTileTestFunctor, NULL);
	printf("\n");

	// Save map.
	if (!mapData.map->save()) {
		printf("Could not save map to '%s'.\n", outputPath);
		return EXIT_FAILURE;
	}
	printf("Saved map to '%s'.\n", outputPath);

	return EXIT_SUCCESS;
}
