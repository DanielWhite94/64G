#include <cassert>
#include <cfloat>
#include <cmath>
#include <cstdbool>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <new>

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

	// These are not computed immediately, see main.
	double coldThreshold;
	double hotThreshold;
	double riverMoistureThreshold;

	// These are computed after ground water/land modify tiles stage.
	unsigned long long landCount, waterCount, arableCount, totalCount; // with arableCount<=landCount
	double landFraction;

	double landSqKm, arableSqKm, peoplePerSqKm, totalPopulation;
} DemogenMapData;

void demogenInitModifyTilesFunctor(class Map *map, unsigned x, unsigned y, void *userData);
void demogenGroundModifyTilesFunctor(class Map *map, unsigned x, unsigned y, void *userData);
/*
void demogenGrassForestModifyTilesFunctor(class Map *map, unsigned x, unsigned y, void *userData);
void demogenSandForestModifyTilesFunctor(class Map *map, unsigned x, unsigned y, void *userData);
*/
void demogenGrassSheepModifyTilesFunctor(class Map *map, unsigned x, unsigned y, void *userData);
void demogenTownFolkModifyTilesFunctor(class Map *map, unsigned x, unsigned y, void *userData);

bool demogenTownTileTestFunctor(class Map *map, int x, int y, int w, int h, void *userData);

void demogenFloodFillLandmassFillFunctor(class Map *map, unsigned x, unsigned y, unsigned groupId, void *userData);

void demogenInitModifyTilesFunctor(class Map *map, unsigned x, unsigned y, void *userData) {
	assert(map!=NULL);
	assert(userData!=NULL);

	DemogenMapData *mapData=(DemogenMapData *)userData;

	// Grab tile.
	MapTile *tile=map->getTileAtOffset(x, y, Engine::Map::Map::GetTileFlag::CreateDirty);
	if (tile==NULL)
		return;

	// Calculate height and temperature.
	double normalisedHeight=mapData->heightNoise->eval(x/((double)mapData->width), y/((double)mapData->height)); // [-1,1.0]
	const double height=normalisedHeight*6000.0; // [-6000,6000]

	const double temperatureRandomOffset=mapData->temperatureNoise->eval(x, y);

	const double latitude=2.0*((double)y)/mapData->height-1.0;
	assert(latitude>=-1.0 && latitude<=1.0);

	const double poleDistance=1.0-fabs(latitude);
	assert(poleDistance>=0.0 && poleDistance<=1.0);

	double adjustedPoleDistance=2*poleDistance-1; // -1..1
	assert(adjustedPoleDistance>=-1.0 && adjustedPoleDistance<=1.0);

	double adjustedHeight=1.0-std::max(0.0, normalisedHeight); // [0,1], roughly assuming sea level will be about height=0

	double temperature=(3*adjustedPoleDistance+6*adjustedHeight+1*temperatureRandomOffset)/7;

	// Update tile.
	tile->setHeight(height);
	tile->setMoisture(0.0);
	tile->setTemperature(temperature);
	tile->setLandmassId(0); // default to 0 to imply part of a border - we will update non-border tiles in a later step
}

void demogenGroundModifyTilesFunctor(class Map *map, unsigned x, unsigned y, void *userData) {
	assert(map!=NULL);
	assert(userData!=NULL);

	DemogenMapData *mapData=(DemogenMapData *)userData;

	// Grab tile.
	MapTile *tile=map->getTileAtOffset(x, y, Engine::Map::Map::GetTileFlag::Dirty);
	if (tile==NULL)
		return;

	// Calculate constants.
	const double height=tile->getHeight();
	const double temperature=tile->getTemperature();

	// Choose texture.
	MapTexture::Id idA=MapGen::TextureIdNone, idB=MapGen::TextureIdNone;
	double factor;
	if (height<=mapData->map->seaLevel) {
		// ocean
		idA=MapGen::TextureIdDeepWater;
		idB=MapGen::TextureIdWater;
		factor=((height-map->minHeight)/(mapData->map->seaLevel-map->minHeight));

		double skewThreshold=0.7; // >0.5 shifts towards idA
		factor=(factor>skewThreshold ? (factor-skewThreshold)/(2.0*(1.0-skewThreshold))+0.5 : factor/(2*skewThreshold));
	} else if (height<=mapData->map->alpineLevel) {
		// River?
		if (tile->getMoisture()>mapData->riverMoistureThreshold) {
			idA=MapGen::TextureIdRiver;
			idB=MapGen::TextureIdRiver;
			factor=0.0;
		} else {
			// land
			if (temperature<=mapData->coldThreshold) {
				// between snow and grass
				idA=MapGen::TextureIdSnow;
				idB=MapGen::TextureIdGrass0;
				factor=(temperature-map->minTemperature)/(mapData->coldThreshold-map->minTemperature);
			} else if (temperature<=mapData->hotThreshold) {
				// grass
				idA=MapGen::TextureIdGrass0;
				idB=MapGen::TextureIdGrass0;
				factor=(temperature-mapData->coldThreshold)/(mapData->hotThreshold-mapData->coldThreshold);
			} else {
				// between sand and hot sand
				idA=MapGen::TextureIdSand;
				idB=MapGen::TextureIdHotSand;
				factor=(temperature-mapData->hotThreshold)/(map->maxTemperature-mapData->hotThreshold);
				double skewThreshold=0.5; // >0.5 shifts towards idA
				factor=(factor>skewThreshold ? (factor-skewThreshold)/(2.0*(1.0-skewThreshold))+0.5 : factor/(2*skewThreshold));
			}
		}
	} else {
		// alpine
		idA=MapGen::TextureIdLowAlpine;
		idB=MapGen::TextureIdHighAlpine;
		factor=(height-map->alpineLevel)/(map->maxHeight-map->alpineLevel);

		double skewThreshold=0.75; // >0.5 shifts towards idA
		factor=(factor>skewThreshold ? (factor-skewThreshold)/(2.0*(1.0-skewThreshold))+0.5 : factor/(2*skewThreshold));
	}
	assert(factor>=0.0 && factor<=1.0);

	MapTexture::Id textureId=(factor<=0.5 ? idA : idB);
	assert(textureId!=MapGen::TextureIdNone);

	// Update tile layer.
	MapTile::Layer layer={.textureId=textureId, .hitmask=HitMask()};
	tile->setLayer(DemoGenTileLayerGround, layer);

	// Update map data.
	if (textureId==MapGen::TextureIdWater || textureId==MapGen::TextureIdDeepWater || textureId==MapGen::TextureIdRiver)
		++mapData->waterCount;
	else
		++mapData->landCount;
	if (textureId>=MapGen::TextureIdGrass0 && textureId<=MapGen::TextureIdGrass5)
		++mapData->arableCount;
	++mapData->totalCount;
}

void demogenGrassForestModifyTilesFunctor(class Map *map, unsigned x, unsigned y, void *userData) {
	/*
	assert(map!=NULL);
	assert(userData!=NULL);

	const DemogenMapData *mapData=(const DemogenMapData *)userData;

	// Choose parameters.
	const double temperateThreshold=0.6;
	const double hotThreshold=0.7;

	assert(0.0<=temperateThreshold);
	assert(temperateThreshold<=hotThreshold);
	assert(hotThreshold<=1.0);

	// Grab tile and temperature.
	MapTile *tile=map->getTileAtOffset(x, y, Engine::Map::Map::GetTileFlag::None);
	if (tile==NULL)
		return;

	const double temperature=tile->getTemperature();

	// Not enough temperature to support anything?
	if (temperature<temperateThreshold)
		return;

	// Random chance of a 'tree'.
	double randomValue=Util::randFloatInInterval(0.0, 1.0);
	if (randomValue<0.90)
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

	if (temperature<hotThreshold) {
		const double probabilities[]={0.3,0.2,0.2,0.2,0.1};
		unsigned index=Util::chooseWithProb(probabilities, sizeof(probabilities)/sizeof(probabilities[0]));
		textureId=MapGen::TextureIdGrass1+index;
	} else
		textureId=MapGen::TextureIdTree1;

	assert(textureId!=MapGen::TextureIdNone);

	// Update tile layer.
	MapTile::Layer layer={.textureId=textureId};
	tile->setLayer(DemoGenTileLayerFull, layer);

	// We have made a change - mark region dirty.
	map->markRegionDirtyAtTileOffset(x, y, false);
	*/
}

void demogenSandForestModifyTilesFunctor(class Map *map, unsigned x, unsigned y, void *userData) {
	/*
	assert(map!=NULL);
	assert(userData!=NULL);

	const DemogenMapData *mapData=(const DemogenMapData *)userData;

	// Grab tile and temperature.
	MapTile *tile=map->getTileAtOffset(x, y, Engine::Map::Map::GetTileFlag::None);
	if (tile==NULL)
		return;

	double temperature=tile->getTemperature();

	// Not enough temperature to support a forest?
	if (temperature<0.7)
		return;

	// Random chance of a tree.
	double randomValue=Util::randFloatInInterval(0.0, 1.0);
	if (randomValue<0.99)
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

	// We have made a change - mark region dirty.
	map->markRegionDirtyAtTileOffset(x, y, false);
	*/
}

void demogenGrassSheepModifyTilesFunctor(class Map *map, unsigned x, unsigned y, void *userData) {
	assert(map!=NULL);
	assert(userData!=NULL);

	const DemogenMapData *mapData=(const DemogenMapData *)userData;

	// Grab tile.
	const MapTile *tile=map->getTileAtOffset(x, y, Engine::Map::Map::GetTileFlag::None);
	if (tile==NULL)
		return;

	// Check layers.
	if (tile->getLayer(DemoGenTileLayerGround)->textureId<MapGen::TextureIdGrass0 || tile->getLayer(DemoGenTileLayerGround)->textureId>MapGen::TextureIdGrass5)
		return;

	// Decide whether to place a sheep here.
	if (rand()%(32*32)!=0)
		return;

	// Add object.
	CoordVec pos(x*CoordsPerTile, y*CoordsPerTile);
	MapObject *sheep=MapGen::addBuiltinObject(map, MapGen::BuiltinObject::Sheep, CoordAngle0, pos);
	if (sheep==NULL)
		return;
	sheep->setMovementModeRandomRadius(pos, 10*CoordsPerTile);


	// Mark region dirty.
	MapRegion *region=map->getRegionAtCoordVec(pos, false);
	if (region!=NULL)
		region->setDirty();
}

void demogenTownFolkModifyTilesFunctor(class Map *map, unsigned x, unsigned y, void *userData) {
	assert(map!=NULL);
	assert(userData!=NULL);

	const DemogenMapData *mapData=(const DemogenMapData *)userData;

	// Grab tile.
	const MapTile *tile=map->getTileAtOffset(x, y, Engine::Map::Map::GetTileFlag::None);
	if (tile==NULL)
		return;

	// Check layers.
	if (tile->getLayer(DemoGenTileLayerGround)->textureId!=MapGen::TextureIdBrickPath && tile->getLayer(DemoGenTileLayerGround)->textureId!=MapGen::TextureIdDirt)
		return;

	// Decide whether to place someone here.
	if (rand()%64!=0)
		return;

	// Choose object.
	MapGen::BuiltinObject builtinObject=(rand()%3==0 ? MapGen::BuiltinObject::Dog : MapGen::BuiltinObject::OldBeardMan);

	// Add object.
	CoordVec pos(x*CoordsPerTile, y*CoordsPerTile);
	MapObject *object=MapGen::addBuiltinObject(map, builtinObject, CoordAngle0, pos);
	if (object==NULL)
		return;
	object->setMovementModeRandomRadius(pos, 10*CoordsPerTile);

	// Mark region dirty.
	MapRegion *region=map->getRegionAtCoordVec(pos, false);
	if (region!=NULL)
		region->setDirty();
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

			// Look for half obstacles.
			MapTexture::Id layerHalf=tile->getLayer(DemoGenTileLayerHalf)->textureId;
			if (layerHalf!=MapGen::TextureIdNone)
				return false;

			// Look for full obstacles.
			MapTexture::Id layerFull=tile->getLayer(DemoGenTileLayerFull)->textureId;
			if (layerFull!=MapGen::TextureIdNone)
				return false;
		}

	return true;
}

void demogenFloodFillLandmassFillFunctor(class Map *map, unsigned x, unsigned y, unsigned groupId, void *userData) {
	assert(map!=NULL);
	assert(userData==NULL);

	// Grab tile.
	MapTile *tile=map->getTileAtOffset(x, y, Engine::Map::Map::GetTileFlag::CreateDirty);
	if (tile==NULL)
		return;

	// Set tile's landmass id
	// Note: we do +1 so that id 0 is reserved for 'border' tiles
	tile->setLandmassId(groupId+1);
}

int main(int argc, char **argv) {
	const double desiredLandFraction=0.4;
	const double desiredAlpineFraction=0.01;

	DemogenMapData mapData={
	    .map=NULL,
	    .width=0,
	    .height=0,
	    .landCount=0,
	    .waterCount=0,
	    .arableCount=0,
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

	// Note start time
	Util::TimeMs startTime=Util::getTimeMs();

	// Create Map.
	printf("Creating map...\n");

	try {
		mapData.map=new class Map(outputPath, mapData.width, mapData.height);
	} catch (std::exception& e) {
		std::cout << "Could not create map: " << e.what() << '\n';
		return EXIT_FAILURE;
	}

	// Add textures.
	printf("Creating textures...\n");
	if (!MapGen::addBaseTextures(mapData.map)) {
		printf("Could not add base textures.\n");
		if (mapData.map!=NULL)
			delete mapData.map;
		return EXIT_FAILURE;
	}

	// Create noise.
	mapData.heightNoise=new FbnNoise(17, 8, 4.0);
	mapData.temperatureNoise=new FbnNoise(19, 8, 1.0);

	// Run init modify tiles function.
	const char *progressStringInit="Initializing tile parameters ";
	MapGen::modifyTiles(mapData.map, 0, 0, mapData.width, mapData.height, &demogenInitModifyTilesFunctor, &mapData, &mapGenModifyTilesProgressString, (void *)progressStringInit);
	printf("\n");

	// Tidy up noise.
	delete mapData.heightNoise;
	mapData.heightNoise=NULL;
	delete mapData.temperatureNoise;
	mapData.temperatureNoise=NULL;

	// Recalculate stats such as min/max height required for future calls.
	const char *progressStringGlobalStats1="Collecting global statistics (1/3) ";
	MapGen::recalculateStats(mapData.map, 0, 0, mapData.width, mapData.height, &mapGenModifyTilesProgressString, (void *)progressStringGlobalStats1);
	printf("\n");

	printf("	Min height %f, max height %f\n", mapData.map->minHeight, mapData.map->maxHeight);
	printf("	Min temperature %f, max temperature %f\n", mapData.map->minTemperature, mapData.map->maxTemperature);
	printf("	Min moisture %f, max moisture %f\n", mapData.map->minMoisture, mapData.map->maxMoisture);

	// Calculate sea level.
	printf("Searching for sea level (with desired land coverage %.2f%%) (1/3)...\n", desiredLandFraction*100.0);
	mapData.map->seaLevel=MapGen::narySearch(mapData.map, 0, 0, mapData.width, mapData.height, 63, desiredLandFraction, 0.45, mapData.map->minHeight, mapData.map->maxHeight, &mapGenNarySearchGetFunctorHeight, NULL);
	printf("	Sea level %f\n", mapData.map->seaLevel);

	// Run glacier calculation.
	/*

	Note: this is disabled for now as it is quite slow, although it does produce good effects

	const char *progressStringGlaciers="Applying glacial effects ";
	MapGen::ParticleFlow glacierGen(mapData.map, 7, false);
	glacierGen.dropParticles(0, 0, mapData.width, mapData.height, 1.0/64.0, &mapGenModifyTilesProgressString, (void *)progressStringGlaciers);
	printf("\n");

	// Recalculate stats such as min/max height required for future calls.
	const char *progressStringGlobalStats2="Collecting global statistics (2/3) ";
	MapGen::recalculateStats(mapData.map, 0, 0, mapData.width, mapData.height, &mapGenModifyTilesProgressString, (void *)progressStringGlobalStats2);
	printf("\n");

	printf("	Min height %f, max height %f\n", mapData.map->minHeight, mapData.map->maxHeight);
	printf("	Min temperature %f, max temperature %f\n", mapData.map->minTemperature, mapData.map->maxTemperature);
	printf("	Min moisture %f, max moisture %f\n", mapData.map->minMoisture, mapData.map->maxMoisture);

	// Calculate sea level.
	printf("Searching for sea level (with desired land coverage %.2f%%) (2/3)...\n", desiredLandFraction*100.0);
	mapData.map->seaLevel=MapGen::narySearch(mapData.map, 0, 0, mapData.width, mapData.height, 63, desiredLandFraction, 0.45, mapData.map->minHeight, mapData.map->maxHeight, &mapGenNarySearchGetFunctorHeight, NULL);
	printf("	Sea level %f\n", mapData.map->seaLevel);
	*/

	// Run moisture/river calculation.
	const char *progressStringRivers="Generating moisture/river data ";
	MapGen::ParticleFlow riverGen(mapData.map, 2, true);
	riverGen.dropParticles(0, 0, mapData.width, mapData.height, 1.0/16.0, &mapGenModifyTilesProgressString, (void *)progressStringRivers);
	printf("\n");

	// Recalculate stats such as min/max height required for future calls.
	const char *progressStringGlobalStats3="Collecting global statistics (3/3) ";
	MapGen::recalculateStats(mapData.map, 0, 0, mapData.width, mapData.height, &mapGenModifyTilesProgressString, (void *)progressStringGlobalStats3);
	printf("\n");

	printf("	Min height %f, max height %f\n", mapData.map->minHeight, mapData.map->maxHeight);
	printf("	Min temperature %f, max temperature %f\n", mapData.map->minTemperature, mapData.map->maxTemperature);
	printf("	Min moisture %f, max moisture %f\n", mapData.map->minMoisture, mapData.map->maxMoisture);

	// Calculate sea level.
	printf("Searching for sea level (with desired land coverage %.2f%%) (3/3)...\n", desiredLandFraction*100.0);
	mapData.map->seaLevel=MapGen::narySearch(mapData.map, 0, 0, mapData.width, mapData.height, 63, desiredLandFraction, 0.45, mapData.map->minHeight, mapData.map->maxHeight, &mapGenNarySearchGetFunctorHeight, NULL);
	printf("	Sea level %f\n", mapData.map->seaLevel);

	// Calculate alpine level.
	printf("Searching for alpine level (with desired coverage %.2f%%)...\n", desiredAlpineFraction*100.0);
	mapData.map->alpineLevel=MapGen::narySearch(mapData.map, 0, 0, mapData.width, mapData.height, 63, desiredAlpineFraction, 0.45, mapData.map->minHeight, mapData.map->maxHeight, &mapGenNarySearchGetFunctorHeight, NULL);
	printf("	Alpine level %f\n", mapData.map->alpineLevel);

	// Calculate cold threshold.
	double desiredColdCoverage=0.4;
	printf("Searching for cold threshold (with desired coverage %.2f%%)...\n", desiredColdCoverage*100.0);
	mapData.coldThreshold=MapGen::narySearch(mapData.map, 0, 0, mapData.width, mapData.height, 63, desiredColdCoverage, 0.45, mapData.map->minTemperature, mapData.map->maxTemperature, &mapGenNarySearchGetFunctorTemperature, NULL);
	printf("	Cold temperature %f\n", mapData.coldThreshold);

	// Calculate hot threshold level.
	double desiredHotCoverage=0.2;
	printf("Searching for hot threshold level (with desired coverage %.2f%%)...\n", desiredHotCoverage*100.0);
	mapData.hotThreshold=MapGen::narySearch(mapData.map, 0, 0, mapData.width, mapData.height, 63, desiredHotCoverage, 0.45, mapData.map->minTemperature, mapData.map->maxTemperature, &mapGenNarySearchGetFunctorTemperature, NULL);
	printf("	Hot temperature %f\n", mapData.hotThreshold);

	// Calculate river moisture threshold.
	double desiredRiverCoverage=0.005;
	printf("Searching for river moisture threshold (with desired coverage %.2f%%)...\n", desiredRiverCoverage*100.0);
	mapData.riverMoistureThreshold=MapGen::narySearch(mapData.map, 0, 0, mapData.width, mapData.height, 63, desiredRiverCoverage, 0.45, mapData.map->minMoisture, mapData.map->maxMoisture, &mapGenNarySearchGetFunctorMoisture, NULL);
	printf("	River moisture threshold %f\n", mapData.riverMoistureThreshold);

	// Run modify tiles for bimomes.
	size_t biomesModifyTilesArrayCount=2;
	MapGen::ModifyTilesManyEntry biomesModifyTilesArray[biomesModifyTilesArrayCount];
	biomesModifyTilesArray[0].functor=&demogenGroundModifyTilesFunctor;
	biomesModifyTilesArray[0].userData=&mapData;
	biomesModifyTilesArray[1].functor=&demogenGrassForestModifyTilesFunctor;
	biomesModifyTilesArray[1].userData=&mapData;
	biomesModifyTilesArray[2].functor=&demogenSandForestModifyTilesFunctor;
	biomesModifyTilesArray[2].userData=&mapData;

	const char *progressStringBiomes="Assigning tile textures for biomes ";
	MapGen::modifyTilesMany(mapData.map, 0, 0, mapData.width, mapData.height, biomesModifyTilesArrayCount, biomesModifyTilesArray, &mapGenModifyTilesProgressString, (void *)progressStringBiomes);
	printf("\n");

	// Compute more map data.
	if (mapData.totalCount>0)
		mapData.landFraction=((double)mapData.landCount)/mapData.totalCount;
	else
		mapData.landFraction=0.0;

	mapData.landSqKm=mapData.landCount/(1000.0*1000.0);
	mapData.arableSqKm=mapData.arableCount/(1000.0*1000.0);
	mapData.peoplePerSqKm=150.0;
	mapData.totalPopulation=mapData.arableSqKm*mapData.peoplePerSqKm;

	setlocale(LC_NUMERIC, "");
	printf("	Land %'.1fkm^2 (of which %'.1fkm^2 is arable), water %'.1fkm^2, land fraction %.2f%%\n", mapData.landSqKm, mapData.arableSqKm, mapData.waterCount/(1000.0*1000.0), mapData.landFraction*100.0);
	printf("	People per km^2 %.0f, total pop %.0f\n", mapData.peoplePerSqKm, mapData.totalPopulation);

	// Add towns.
	printf("Adding towns...\n");
	MapGen::addTowns(mapData.map, 0, 0, mapData.width, mapData.height, DemoGenTileLayerGround, DemoGenTileLayerFull, DemoGenTileLayerDecoration, mapData.totalPopulation, &demogenTownTileTestFunctor, NULL);
	printf("\n");

	// Run modify tiles npcs/animals.
	size_t npcModifyTilesArrayCount=2;
	MapGen::ModifyTilesManyEntry npcModifyTilesArray[npcModifyTilesArrayCount];
	npcModifyTilesArray[0].functor=&demogenGrassSheepModifyTilesFunctor;
	npcModifyTilesArray[0].userData=&mapData;
	npcModifyTilesArray[1].functor=&demogenTownFolkModifyTilesFunctor;
	npcModifyTilesArray[1].userData=&mapData;

	const char *progressStringNpcsAnimals="Adding npcs and animals ";
	MapGen::modifyTilesMany(mapData.map, 0, 0, mapData.width, mapData.height, npcModifyTilesArrayCount, npcModifyTilesArray, &mapGenModifyTilesProgressString, (void *)progressStringNpcsAnimals);
	printf("\n");

	// Landmass (contintent/island) identification
	unsigned landmassCacheBits[MapGen::EdgeDetect::DirectionNB];
	landmassCacheBits[MapGen::EdgeDetect::DirectionEast]=60;
	landmassCacheBits[MapGen::EdgeDetect::DirectionNorth]=61;
	landmassCacheBits[MapGen::EdgeDetect::DirectionWest]=62;
	landmassCacheBits[MapGen::EdgeDetect::DirectionSouth]=63;

	MapGen::EdgeDetect landmassEdgeDetect(mapData.map, landmassCacheBits);
	landmassEdgeDetect.trace(&mapGenEdgeDetectLandSampleFunctor, NULL, &mapGenEdgeDetectBitsetNEdgeFunctor, (void *)(uintptr_t)MapGen::TileBitsetIndexLandmassBorder, &mapGenEdgeDetectStringProgressFunctor, (void *)"Identifying landmass boundaries via edge detection ");
	printf("\n");

	MapGen::FloodFill landmassFloodFill(mapData.map, 63);
	landmassFloodFill.fill(&mapGenFloodFillBitsetNBoundaryFunctor, (void *)(uintptr_t)MapGen::TileBitsetIndexLandmassBorder, &demogenFloodFillLandmassFillFunctor, NULL, &mapGenFloodFillStringProgressFunctor, (void *)"Identifying individual landmasses via flood-fill ");
	printf("\n");

	// Run contour line detection logic
	unsigned contourCacheBits[MapGen::EdgeDetect::DirectionNB];
	contourCacheBits[MapGen::EdgeDetect::DirectionEast]=60;
	contourCacheBits[MapGen::EdgeDetect::DirectionNorth]=61;
	contourCacheBits[MapGen::EdgeDetect::DirectionWest]=62;
	contourCacheBits[MapGen::EdgeDetect::DirectionSouth]=63;

	MapGen::EdgeDetect heightContourEdgeDetect(mapData.map, contourCacheBits);
	heightContourEdgeDetect.traceHeightContours(19, &mapGenEdgeDetectStringProgressFunctor, (void *)"Height contour edge detection ");
	printf("\n");

	// Save map.
	if (!mapData.map->save()) {
		printf("Could not save map to '%s'.\n", outputPath);
		delete mapData.map;
		return EXIT_FAILURE;
	}
	printf("Saved map to '%s'.\n", outputPath);

	// Print elapsed total time
	Util::TimeMs totalTime=Util::getTimeMs()-startTime;
	printf("Total generation time: ");
	mapGenPrintTime(totalTime);
	printf("\n");

	// Tidy up
	delete mapData.map;
	return EXIT_SUCCESS;
}
