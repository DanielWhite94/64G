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

struct DemogenGrassForestModifyTilesData {
	NoiseArray *moistureNoiseArray;
};

typedef struct {
	DemogenMapData *mapData;

	NoiseArray *heightNoiseArray;
	NoiseArray *temperatureNoiseArray;
} DemogenGroundModifyTilesData;

typedef struct {
	NoiseArray *moistureNoiseArray;
} DemogenSandForestModifyTilesData;

void demogenGroundModifyTilesFunctor(class Map *map, unsigned x, unsigned y, void *userData);
void demogenGrassForestModifyTilesFunctor(class Map *map, unsigned x, unsigned y, void *userData);
void demogenSandForestModifyTilesFunctor(class Map *map, unsigned x, unsigned y, void *userData);

MapGen::ModifyTilesManyEntry *demogenMakeModifyTilesManyEntryGround(DemogenMapData *mapData);
MapGen::ModifyTilesManyEntry *demogenMakeModifyTilesManyEntryGrassForest(DemogenMapData *mapData);
MapGen::ModifyTilesManyEntry *demogenMakeModifyTilesManyEntrySandForest(DemogenMapData *mapData);

bool demogenTownTileTestFunctor(class Map *map, int x, int y, int w, int h, void *userData);

void demogenGroundModifyTilesFunctor(class Map *map, unsigned x, unsigned y, void *userData) {
	assert(map!=NULL);
	assert(userData!=NULL);

	const DemogenGroundModifyTilesData *data=(const DemogenGroundModifyTilesData *)userData;
	DemogenMapData *mapData=data->mapData;

	// Grab tile.
	MapTile *tile=map->getTileAtOffset(x, y, false);
	if (tile==NULL)
		return;

	// Choose parameters.
	const double seaLevel=0.05;

	// Calculate constants.
	double height=data->heightNoiseArray->eval(x, y);
	double temperatureRandomOffset=data->temperatureNoiseArray->eval(x, y);
	double normalisedHeight=(height>seaLevel ? (height-seaLevel)/(1.0-seaLevel) : 0.0);
	double latitude=2.0*((double)y)/mapData->height-1.0;
	double poleDistance=1.0-fabs(latitude);

	double adjustedPoleDistance=2*poleDistance-1; // -1..1
	double adjustedHeight=2*normalisedHeight-1;
	double temperature=(4*adjustedPoleDistance+2*adjustedHeight+2*temperatureRandomOffset)/8;
	temperature=2*temperature+2.0/3; // HACK to get reasonable range.
	if (temperature<-1.0)
		temperature=-1.0;
	if (temperature>1.0)
		temperature=1.0;

	assert(height>=-1.0 & height<=1.0);
	assert(temperatureRandomOffset>=-1.0 & temperatureRandomOffset<=1.0);
	assert(normalisedHeight>=0.0 && normalisedHeight<=1.0);
	assert(latitude>=-1.0 && latitude<=1.0);
	assert(poleDistance>=0.0 && poleDistance<=1.0);
	assert(temperature>=-1.0 && temperature<=1.0);

	// Choose texture.
	MapTexture::Id idA=MapGen::TextureIdNone, idB=MapGen::TextureIdNone;
	double factor;
	if (height<=seaLevel) {
		// water
		idA=MapGen::TextureIdWater;
		idB=MapGen::TextureIdWater;
		factor=0.0;
	} else {
		// land
		const double temperatureThreshold=0.5;
		if (temperature<=temperatureThreshold) {
			// between snow and grass
			idA=MapGen::TextureIdSnow;
			idB=MapGen::TextureIdGrass0;
			factor=(temperature+1)/(temperatureThreshold+1);
		} else {
			// between grass and sand
			idA=MapGen::TextureIdGrass0;
			idB=MapGen::TextureIdSand;
			factor=(temperature-temperatureThreshold)/(1-temperatureThreshold);
		}
	}
	assert(factor>=0.0 && factor<=1.0);

	MapTexture::Id textureId=(factor<=0.5 ? idA : idB);
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

void demogenGrassForestModifyTilesFunctor(class Map *map, unsigned x, unsigned y, void *userData) {
	assert(map!=NULL);
	assert(userData!=NULL);

	const DemogenGrassForestModifyTilesData *data=(const DemogenGrassForestModifyTilesData *)userData;

	// Choose parameters.
	const double temperateThreshold=0.6;
	const double hotThreshold=0.7;

	assert(0.0<=temperateThreshold);
	assert(temperateThreshold<=hotThreshold);
	assert(hotThreshold<=1.0);

	// Compute constants..
	double moisture=(data->moistureNoiseArray->eval(x, y)+1.0)/2.0;

	assert(moisture>=0.0 & moisture<=1.0);

	// Not enough moisture to support anything?
	if (moisture<temperateThreshold)
		return;

	// Random chance of a 'tree'.
	double randomValue=(rand()/((double)RAND_MAX));
	if (randomValue<0.90)
		return;

	// Grab tile.
	MapTile *tile=map->getTileAtOffset(x, y, false);
	if (tile==NULL)
		return;

	// Check layers.
	if (tile->getLayer(DemoGenTileLayerGround)->textureId!=MapGen::TextureIdGrass0)
		return;
	if (tile->getLayer(DemoGenTileLayerDecoration)->textureId!=MapGen::TextureIdNone)
		return;
	if (tile->getLayer(DemoGenTileLayerHalf)->textureId!=MapGen::TextureIdNone)
		return;
	if (tile->getLayer(DemoGenTileLayerFull)->textureId!=MapGen::TextureIdNone)
		return;

	// Choose new texture.
	MapTexture::Id textureId=MapGen::TextureIdNone;

	if (moisture<hotThreshold) {
		const double probabilities[]={0.3,0.2,0.2,0.2,0.1};
		unsigned index=Util::chooseWithProb(probabilities, sizeof(probabilities)/sizeof(probabilities[0]));
		textureId=MapGen::TextureIdGrass1+index;
	} else
		textureId=MapGen::TextureIdTree1;

	assert(textureId!=MapGen::TextureIdNone);

	// Update tile layer.
	MapTile::Layer layer={.textureId=textureId};
	tile->setLayer(DemoGenTileLayerFull, layer);
}

void demogenSandForestModifyTilesFunctor(class Map *map, unsigned x, unsigned y, void *userData) {
	assert(map!=NULL);
	assert(userData!=NULL);

	const DemogenSandForestModifyTilesData *data=(const DemogenSandForestModifyTilesData *)userData;

	// Compute constants..
	double moisture=(data->moistureNoiseArray->eval(x, y)+1.0)/2.0;

	assert(moisture>=0.0 & moisture<=1.0);

	// Not enough moisture to support a forest?
	if (moisture<0.7)
		return;

	// Random chance of a tree.
	double randomValue=(rand()/((double)RAND_MAX));
	if (randomValue<0.99)
		return;

	// Grab tile.
	MapTile *tile=map->getTileAtOffset(x, y, false);
	if (tile==NULL)
		return;

	// Check layers.
	if (tile->getLayer(DemoGenTileLayerGround)->textureId!=MapGen::TextureIdSand)
		return;
	if (tile->getLayer(DemoGenTileLayerDecoration)->textureId!=MapGen::TextureIdNone)
		return;
	if (tile->getLayer(DemoGenTileLayerHalf)->textureId!=MapGen::TextureIdNone)
		return;
	if (tile->getLayer(DemoGenTileLayerFull)->textureId!=MapGen::TextureIdNone)
		return;

	// Update tile layer.
	MapTile::Layer layer={.textureId=MapGen::TextureIdTree3};
	tile->setLayer(DemoGenTileLayerFull, layer);
}

MapGen::ModifyTilesManyEntry *demogenMakeModifyTilesManyEntryGround(DemogenMapData *mapData) {
	assert(mapData!=NULL);

	// Create callback data.
	DemogenGroundModifyTilesData *callbackData=(DemogenGroundModifyTilesData *)malloc(sizeof(DemogenGroundModifyTilesData));
	assert(callbackData!=NULL); // TODO: Better
	callbackData->mapData=mapData;

	// Create noise.
	callbackData->heightNoiseArray=new NoiseArray(17, mapData->width, mapData->height, 2048, 2048, 600.0, 16, 8, &noiseArrayProgressFunctorString, (void *)"Generating ground height noise ");
	printf("\n");
	callbackData->temperatureNoiseArray=new NoiseArray(19, mapData->width, mapData->height, 1024, 1024, 100.0, 16, 8, &noiseArrayProgressFunctorString, (void *)"Generating temperature noise ");
	printf("\n");

	// Create entry.
	MapGen::ModifyTilesManyEntry *entry=(MapGen::ModifyTilesManyEntry *)malloc(sizeof(MapGen::ModifyTilesManyEntry));
	entry->functor=&demogenGroundModifyTilesFunctor;
	entry->userData=(void *)callbackData;

	return entry;
}

MapGen::ModifyTilesManyEntry *demogenMakeModifyTilesManyEntryGrassForest(DemogenMapData *mapData) {
	assert(mapData!=NULL);


	// Create user data.
	DemogenGrassForestModifyTilesData *callbackData=(DemogenGrassForestModifyTilesData *)malloc(sizeof(MapGen::GenerateBinaryNoiseModifyTilesData));
	assert(callbackData!=NULL); // TODO: better

	// Create noise.
	callbackData->moistureNoiseArray=new NoiseArray(23, mapData->width, mapData->height, 1024, 1024, 100.0, 16, 8, &noiseArrayProgressFunctorString, (void *)"Generating grass forest moisture noise ");
	printf("\n");

	// Create entry.
	MapGen::ModifyTilesManyEntry *entry=(MapGen::ModifyTilesManyEntry *)malloc(sizeof(MapGen::ModifyTilesManyEntry));
	entry->functor=&demogenGrassForestModifyTilesFunctor;
	entry->userData=callbackData;

	return entry;
}

MapGen::ModifyTilesManyEntry *demogenMakeModifyTilesManyEntrySandForest(DemogenMapData *mapData) {
	assert(mapData!=NULL);


	// Create user data.
	DemogenSandForestModifyTilesData *callbackData=(DemogenSandForestModifyTilesData *)malloc(sizeof(MapGen::GenerateBinaryNoiseModifyTilesData));
	assert(callbackData!=NULL); // TODO: better

	// Create noise.
	callbackData->moistureNoiseArray=new NoiseArray(23, mapData->width, mapData->height, 1024, 1024, 100.0, 16, 8, &noiseArrayProgressFunctorString, (void *)"Generating sand forest moisture noise ");
	printf("\n");

	// Create entry.
	MapGen::ModifyTilesManyEntry *entry=(MapGen::ModifyTilesManyEntry *)malloc(sizeof(MapGen::ModifyTilesManyEntry));
	entry->functor=&demogenSandForestModifyTilesFunctor;
	entry->userData=callbackData;

	return entry;
}

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
	size_t modifyTilesArrayCount=3;
	MapGen::ModifyTilesManyEntry *modifyTilesArray[modifyTilesArrayCount];
	modifyTilesArray[0]=demogenMakeModifyTilesManyEntryGround(&mapData);
	modifyTilesArray[1]=demogenMakeModifyTilesManyEntryGrassForest(&mapData);
	modifyTilesArray[2]=demogenMakeModifyTilesManyEntrySandForest(&mapData);

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
