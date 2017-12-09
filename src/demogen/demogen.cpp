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

	NoiseArray *heightNoiseArray;
	NoiseArray *temperatureNoiseArray;
	NoiseArray *moistureNoiseArray;
} DemogenGroundModifyTilesData;

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
	double moisture=(data->moistureNoiseArray->eval(x, y)+1.0)/2.0;
	double normalisedHeight=(height>seaLevel ? (height-seaLevel)/(1.0-seaLevel) : 0.0);
	double latitude=2.0*((double)y)/mapData->height-1.0;
	double poleDistance=1.0-fabs(latitude);
	double temperature=(5*(2*poleDistance-1)+3*temperatureRandomOffset+3-3*((height+1)/2))/11;
	temperature*=2;
	if (temperature<-1.0)
		temperature=-1.0;
	if (temperature>1.0)
		temperature=1.0;

	assert(height>=-1.0 & height<=1.0);
	assert(temperatureRandomOffset>=-1.0 & temperatureRandomOffset<=1.0);
	assert(moisture>=0.0 & moisture<=1.0);
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

MapGen::ModifyTilesManyEntry *demogenMakeModifyTilesManyEntryGround(DemogenMapData *mapData) {
	assert(mapData!=NULL);

	// Create callback data.
	DemogenGroundModifyTilesData *callbackData=(DemogenGroundModifyTilesData *)malloc(sizeof(DemogenGroundModifyTilesData));
	assert(callbackData!=NULL); // TODO: Better
	callbackData->mapData=mapData;

	// Create noise.
	callbackData->heightNoiseArray=new NoiseArray(17, mapData->width, mapData->height, 2048, 2048, 600.0, 16, &noiseArrayProgressFunctorString, (void *)"Generating ground height noise ");
	printf("\n");
	callbackData->temperatureNoiseArray=new NoiseArray(19, mapData->width, mapData->height, 1024, 1024, 100.0, 16, &noiseArrayProgressFunctorString, (void *)"Generating temperature noise ");
	printf("\n");
	callbackData->moistureNoiseArray=new NoiseArray(23, mapData->width, mapData->height, 1024, 1024, 100.0, 16, &noiseArrayProgressFunctorString, (void *)"Generating moisture noise ");
	printf("\n");

	// Create entry.
	MapGen::ModifyTilesManyEntry *entry=(MapGen::ModifyTilesManyEntry *)malloc(sizeof(MapGen::ModifyTilesManyEntry));
	entry->functor=&demogenGroundModifyTilesFunctor;
	entry->userData=(void *)callbackData;

	return entry;
}

/*
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
	NoiseArray *noiseArray=new NoiseArray(13, width, height, 1024, 1024, 200.0, 16, &noiseArrayProgressFunctorString, (void *)"Generating forest noise ");
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
*/

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
	size_t modifyTilesArrayCount=1;
	MapGen::ModifyTilesManyEntry *modifyTilesArray[modifyTilesArrayCount];
	modifyTilesArray[0]=demogenMakeModifyTilesManyEntryGround(&mapData);

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
