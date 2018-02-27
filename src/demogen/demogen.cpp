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

	FbnNoise *heightNoise;
	FbnNoise *temperatureNoise;
	NoiseArray *moistureNoise;

	// These are computed after ground water/land modify tiles stage.
	unsigned long long landCount, waterCount, totalCount;
	double landFraction;

	double landSqKm, peoplePerSqKm, totalPopulation;
} DemogenMapData;

void demogenGroundModifyTilesFunctor(class Map *map, unsigned x, unsigned y, void *userData);
void demogenGrassForestModifyTilesFunctor(class Map *map, unsigned x, unsigned y, void *userData);
void demogenSandForestModifyTilesFunctor(class Map *map, unsigned x, unsigned y, void *userData);

bool demogenTownTileTestFunctor(class Map *map, int x, int y, int w, int h, void *userData);

void demogenGroundModifyTilesFunctor(class Map *map, unsigned x, unsigned y, void *userData) {
	assert(map!=NULL);
	assert(userData!=NULL);

	DemogenMapData *mapData=(DemogenMapData *)userData;

	// Grab tile.
	MapTile *tile=map->getTileAtOffset(x, y, Engine::Map::Map::GetTileFlag::Dirty);
	if (tile==NULL)
		return;

	// Choose parameters.
	const double seaLevel=0.05;
	const double alpineLevel=0.33;

	// Calculate constants.
	double height=mapData->heightNoise->eval(x, y);
	double temperatureRandomOffset=mapData->temperatureNoise->eval(x, y);
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
		idA=MapGen::TextureIdDeepWater;
		idB=MapGen::TextureIdWater;
		factor=((height+1.0)/(seaLevel+1.0));

		double skewThreshold=0.7; // >0.5 shifts towards idA
		factor=(factor>skewThreshold ? (factor-skewThreshold)/(2.0*(1.0-skewThreshold))+0.5 : factor/(2*skewThreshold));
	} else if (height<=alpineLevel) {
		// land
		const double temperatureThreshold=0.5;
		const double temperatureThreshold2=0.8;
		if (temperature<=temperatureThreshold) {
			// between snow and grass
			idA=MapGen::TextureIdSnow;
			idB=MapGen::TextureIdGrass0;
			factor=(temperature+1)/(temperatureThreshold+1);
		} else if (temperature<=temperatureThreshold2) {
			// between grass and sand
			idA=MapGen::TextureIdGrass0;
			idB=MapGen::TextureIdSand;
			factor=(temperature-temperatureThreshold)/(temperatureThreshold2-temperatureThreshold);
		} else {
			// between sand and hot sand
			idA=MapGen::TextureIdSand;
			idB=MapGen::TextureIdHotSand;
			factor=(temperature-temperatureThreshold2)/(1.0-temperatureThreshold2);

			double skewThreshold=0.7; // >0.5 shifts towards idA
			factor=(factor>skewThreshold ? (factor-skewThreshold)/(2.0*(1.0-skewThreshold))+0.5 : factor/(2*skewThreshold));
		}
	} else {
		// alpine
		idA=MapGen::TextureIdLowAlpine;
		idB=MapGen::TextureIdHighAlpine;
		factor=(height-alpineLevel)/(1.0-alpineLevel);

		double skewThreshold=0.2; // >0.5 shifts towards idA
		factor=(factor>skewThreshold ? (factor-skewThreshold)/(2.0*(1.0-skewThreshold))+0.5 : factor/(2*skewThreshold));
	}
	assert(factor>=0.0 && factor<=1.0);

	MapTexture::Id textureId=(factor<=0.5 ? idA : idB);
	assert(textureId!=MapGen::TextureIdNone);

	// Update tile layer.
	MapTile::Layer layer={.textureId=textureId};
	tile->setLayer(DemoGenTileLayerGround, layer);

	// Update map data.
	if (textureId==MapGen::TextureIdWater || textureId==MapGen::TextureIdDeepWater)
		++mapData->waterCount;
	else
		++mapData->landCount;
	++mapData->totalCount;
}

void demogenGrassForestModifyTilesFunctor(class Map *map, unsigned x, unsigned y, void *userData) {
	assert(map!=NULL);
	assert(userData!=NULL);

	const DemogenMapData *mapData=(const DemogenMapData *)userData;

	// Choose parameters.
	const double temperateThreshold=0.6;
	const double hotThreshold=0.7;

	assert(0.0<=temperateThreshold);
	assert(temperateThreshold<=hotThreshold);
	assert(hotThreshold<=1.0);

	// Compute constants..
	double moisture=(mapData->moistureNoise->eval(x, y)+1.0)/2.0;

	assert(moisture>=0.0 & moisture<=1.0);

	// Not enough moisture to support anything?
	if (moisture<temperateThreshold)
		return;

	// Random chance of a 'tree'.
	double randomValue=Util::randFloatInInterval(0.0, 1.0);
	if (randomValue<0.90)
		return;

	// Grab tile.
	MapTile *tile=map->getTileAtOffset(x, y, Engine::Map::Map::GetTileFlag::None); // TODO: Mark region dirty (but wait until we are sure we are going to make a change).
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

	const DemogenMapData *mapData=(const DemogenMapData *)userData;

	// Compute constants..
	double moisture=(mapData->moistureNoise->eval(x, y)+1.0)/2.0;

	assert(moisture>=0.0 & moisture<=1.0);

	// Not enough moisture to support a forest?
	if (moisture<0.7)
		return;

	// Random chance of a tree.
	double randomValue=Util::randFloatInInterval(0.0, 1.0);
	if (randomValue<0.99)
		return;

	// Grab tile.
	MapTile *tile=map->getTileAtOffset(x, y, Engine::Map::Map::GetTileFlag::None); // TODO: Mark region dirty (but wait until we are sure we are going to make a change).
	if (tile==NULL)
		return;

	// Check layers.
	if (tile->getLayer(DemoGenTileLayerGround)->textureId!=MapGen::TextureIdSand && tile->getLayer(DemoGenTileLayerGround)->textureId!=MapGen::TextureIdHotSand)
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

bool demogenTownTileTestFunctor(class Map *map, int x, int y, int w, int h, void *userData) {
	assert(map!=NULL);

	// Loop over tiles.
	int tx, ty;
	for(ty=0; ty<h; ++ty)
		for(tx=0; tx<w; ++tx) {
			const MapTile *tile=map->getTileAtCoordVec(CoordVec((x+tx)*Physics::CoordsPerTile, (y+ty)*Physics::CoordsPerTile), Engine::Map::Map::GetTileFlag::None);
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

	// Create noise.
	mapData.heightNoise=new FbnNoise(17, 8, 1.0/(8.0*1024.0), 1.0, 2.0, 0.5);
	mapData.temperatureNoise=new FbnNoise(19, 8, 1.0/1024.0, 1.0, 2.0, 0.5);
	mapData.moistureNoise=new NoiseArray(23, mapData.width, mapData.height, 4*1024, 4*1024, 1024.0, 16, 8, &noiseArrayProgressFunctorString, (void *)"Generating moisture noise ");
	printf("\n");

	// Run modify tiles.
	size_t modifyTilesArrayCount=3;
	MapGen::ModifyTilesManyEntry modifyTilesArray[modifyTilesArrayCount];
	modifyTilesArray[0].functor=&demogenGroundModifyTilesFunctor;
	modifyTilesArray[0].userData=&mapData;
	modifyTilesArray[1].functor=&demogenGrassForestModifyTilesFunctor;
	modifyTilesArray[1].userData=&mapData;
	modifyTilesArray[2].functor=&demogenSandForestModifyTilesFunctor;
	modifyTilesArray[2].userData=&mapData;

	const char *progressString="Generating tiles (ground+forests) ";
	MapGen::modifyTilesMany(mapData.map, 0, 0, mapData.width, mapData.height, modifyTilesArrayCount, modifyTilesArray, &mapGenModifyTilesProgressString, (void *)progressString);
	printf("\n");

	// Tidy up noise.
	delete mapData.heightNoise;
	mapData.heightNoise=NULL;
	delete mapData.temperatureNoise;
	mapData.temperatureNoise=NULL;
	delete mapData.moistureNoise;
	mapData.moistureNoise=NULL;

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
