#include <cassert>
#include <cfloat>
#include <cmath>
#include <cstdbool>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <new>

#include "../engine/gen/common.h"
#include "../engine/gen/edgedetect.h"
#include "../engine/gen/floodfill.h"
#include "../engine/gen/modifytiles.h"
#include "../engine/gen/particleflow.h"
#include "../engine/gen/search.h"
#include "../engine/gen/stats.h"
#include "../engine/gen/town.h"
#include "../engine/map/map.h"
#include "../engine/map/mapobject.h"
#include "../engine/fbnnoise.h"
#include "../engine/util.h"

#include "common.h"

typedef struct {
	// Only these are computed initially.
	class Map *map;
	int width, height;

	FbnNoise *heightNoise;
	FbnNoise *temperatureNoise;
	FbnNoise *forestNoise;

	// These are not computed immediately, see main.
	double coldThreshold;
	double hotThreshold;
	double riverMoistureThreshold;

	// These are computed after ground water/land modify tiles stage.
	unsigned long long landCount, waterCount, arableCount, totalCount; // with arableCount<=landCount
	double landFraction;

	double landSqKm, arableSqKm, peoplePerSqKm, totalPopulation;
} DemogenMapData;

bool demogenAddTextures(class Map *map);
bool demogenAddItems(class Map *map);

MapObject *demogenAddObjectSheep(class Map *map, CoordAngle rotation, const CoordVec &pos);
MapObject *demogenAddObjectDog(class Map *map, CoordAngle rotation, const CoordVec &pos);
MapObject *demogenAddObjectOldBeardMan(class Map *map, CoordAngle rotation, const CoordVec &pos);

void demogenInitModifyTilesFunctor(unsigned threadId, class Map *map, unsigned x, unsigned y, void *userData);
void demogenGroundModifyTilesFunctor(unsigned threadId, class Map *map, unsigned x, unsigned y, void *userData);
void demogenGrassForestModifyTilesFunctor(unsigned threadId, class Map *map, unsigned x, unsigned y, void *userData);
void demogenSandForestModifyTilesFunctor(unsigned threadId, class Map *map, unsigned x, unsigned y, void *userData);
void demogenGrassSheepModifyTilesFunctor(unsigned threadId, class Map *map, unsigned x, unsigned y, void *userData);
void demogenTownFolkModifyTilesFunctor(unsigned threadId, class Map *map, unsigned x, unsigned y, void *userData);

bool demogenTownTileTestFunctor(class Map *map, int x, int y, int w, int h, void *userData);

void demogenFloodFillLandmassFillFunctor(class Map *map, unsigned x, unsigned y, unsigned groupId, void *userData);

bool demogenAddTextures(class Map *map) {
	const char *texturePaths[TextureIdNB]={
		[TextureIdGrass0]="../src/demo/images/tiles/grass0.png",
		[TextureIdGrass1]="../src/demo/images/tiles/grass1.png",
		[TextureIdGrass2]="../src/demo/images/tiles/grass2.png",
		[TextureIdGrass3]="../src/demo/images/tiles/grass3.png",
		[TextureIdGrass4]="../src/demo/images/tiles/grass4.png",
		[TextureIdGrass5]="../src/demo/images/tiles/grass5.png",
		[TextureIdBrickPath]="../src/demo/images/tiles/tile.png",
		[TextureIdDirt]="../src/demo/images/tiles/dirt.png",
		[TextureIdDock]="../src/demo/images/tiles/dock.png",
		[TextureIdWater]="../src/demo/images/tiles/water.png",
		[TextureIdTree1]="../src/demo/images/objects/tree1.png",
		[TextureIdTree2]="../src/demo/images/objects/tree2.png",
		[TextureIdTree3]="../src/demo/images/objects/tree3.png",
		[TextureIdMan1]="../src/demo/images/objects/man1.png",
		[TextureIdOldManN]="../src/demo/images/npcs/oldbeardman/north.png",
		[TextureIdOldManE]="../src/demo/images/npcs/oldbeardman/east.png",
		[TextureIdOldManS]="../src/demo/images/npcs/oldbeardman/south.png",
		[TextureIdOldManW]="../src/demo/images/npcs/oldbeardman/west.png",
		[TextureIdHouseDoorBL]="../src/demo/images/tiles/house/doorbl.png",
		[TextureIdHouseDoorBR]="../src/demo/images/tiles/house/doorbr.png",
		[TextureIdHouseDoorTL]="../src/demo/images/tiles/house/doortl.png",
		[TextureIdHouseDoorTR]="../src/demo/images/tiles/house/doortr.png",
		[TextureIdHouseRoof]="../src/demo/images/tiles/house/roof.png",
		[TextureIdHouseRoofTop]="../src/demo/images/tiles/house/rooftop.png",
		[TextureIdHouseWall2]="../src/demo/images/tiles/house/wall2.png",
		[TextureIdHouseWall3]="../src/demo/images/tiles/house/wall3.png",
		[TextureIdHouseWall4]="../src/demo/images/tiles/house/wall4.png",
		[TextureIdHouseChimney]="../src/demo/images/tiles/house/chimney.png",
		[TextureIdHouseChimneyTop]="../src/demo/images/tiles/house/chimneytop.png",
		[TextureIdSand]="../src/demo/images/tiles/sand.png",
		[TextureIdHotSand]="../src/demo/images/tiles/hotsand.png",
		[TextureIdSnow]="../src/demo/images/tiles/snow.png",
		[TextureIdShopCobbler]="../src/demo/images/tiles/shops/cobbler.png",
		[TextureIdDeepWater]="../src/demo/images/tiles/deepwater.png",
		[TextureIdRiver]="../src/demo/images/tiles/water.png",
		[TextureIdHighAlpine]="../src/demo/images/tiles/highalpine.png",
		[TextureIdLowAlpine]="../src/demo/images/tiles/lowalpine.png",
		[TextureIdSheepN]="../src/demo/images/npcs/sheep/north.png",
		[TextureIdSheepE]="../src/demo/images/npcs/sheep/east.png",
		[TextureIdSheepS]="../src/demo/images/npcs/sheep/south.png",
		[TextureIdSheepW]="../src/demo/images/npcs/sheep/west.png",
		[TextureIdRoseBush]="../src/demo/images/objects/rosebush.png",
		[TextureIdCoins]="../src/demo/images/objects/coins.png",
		[TextureIdDog]="../src/demo/images/npcs/dog/east.png",
		[TextureIdChestClosed]="../src/demo/images/objects/chestclosed.png",
		[TextureIdChestOpen]="../src/demo/images/objects/chestopen.png",
	};
	int textureScales[TextureIdNB]={
		[TextureIdGrass0]=4,
		[TextureIdGrass1]=4,
		[TextureIdGrass2]=4,
		[TextureIdGrass3]=4,
		[TextureIdGrass4]=4,
		[TextureIdGrass5]=4,
		[TextureIdBrickPath]=4,
		[TextureIdDirt]=4,
		[TextureIdDock]=4,
		[TextureIdWater]=4,
		[TextureIdTree1]=4,
		[TextureIdTree2]=4,
		[TextureIdTree3]=4,
		[TextureIdMan1]=4,
		[TextureIdOldManN]=4,
		[TextureIdOldManE]=4,
		[TextureIdOldManS]=4,
		[TextureIdOldManW]=4,
		[TextureIdHouseDoorBL]=4,
		[TextureIdHouseDoorBR]=4,
		[TextureIdHouseDoorTL]=4,
		[TextureIdHouseDoorTR]=4,
		[TextureIdHouseRoof]=4,
		[TextureIdHouseRoofTop]=4,
		[TextureIdHouseWall2]=4,
		[TextureIdHouseWall3]=4,
		[TextureIdHouseWall4]=4,
		[TextureIdHouseChimney]=4,
		[TextureIdHouseChimneyTop]=4,
		[TextureIdSand]=4,
		[TextureIdHotSand]=4,
		[TextureIdShopCobbler]=4,
		[TextureIdDeepWater]=4,
		[TextureIdRiver]=4,
		[TextureIdSnow]=4,
		[TextureIdHighAlpine]=4,
		[TextureIdLowAlpine]=4,
		[TextureIdSheepN]=8,
		[TextureIdSheepE]=8,
		[TextureIdSheepS]=8,
		[TextureIdSheepW]=8,
		[TextureIdRoseBush]=8,
		[TextureIdCoins]=8,
		[TextureIdDog]=8,
		[TextureIdChestClosed]=4,
		[TextureIdChestOpen]=4,
	};
	uint8_t textureColours[TextureIdNB][3]={
		[TextureIdGrass0]={0,160,0},
		[TextureIdGrass1]={0,192,0},
		[TextureIdGrass2]={0,160,0},
		[TextureIdGrass3]={0,128,0},
		[TextureIdGrass4]={0,96,0},
		[TextureIdGrass5]={0,64,0},
		[TextureIdBrickPath]={102,51,0},
		[TextureIdDirt]={204,102,0},
		[TextureIdDock]={102,51,0},
		[TextureIdWater]={0,0,140},
		[TextureIdTree1]={0,100,0},
		[TextureIdTree2]={0,100,0},
		[TextureIdTree3]={255,50,0},
		[TextureIdMan1]={255,0,0},
		[TextureIdOldManN]={255,0,0},
		[TextureIdOldManE]={255,0,0},
		[TextureIdOldManS]={255,0,0},
		[TextureIdOldManW]={255,0,0},
		[TextureIdHouseDoorBL]={255,128,0},
		[TextureIdHouseDoorBR]={255,128,0},
		[TextureIdHouseDoorTL]={255,128,0},
		[TextureIdHouseDoorTR]={255,128,0},
		[TextureIdHouseRoof]={255,128,0},
		[TextureIdHouseRoofTop]={255,128,0},
		[TextureIdHouseWall2]={255,128,0},
		[TextureIdHouseWall3]={255,128,0},
		[TextureIdHouseWall4]={255,128,0},
		[TextureIdHouseChimney]={255,128,0},
		[TextureIdHouseChimneyTop]={255,128,0},
		[TextureIdSand]={255,204,50},
		[TextureIdHotSand]={255,102,50},
		[TextureIdShopCobbler]={255,128,0},
		[TextureIdDeepWater]={0,0,110},
		[TextureIdRiver]={0,0,225},
		[TextureIdSnow]={220,220,220},
		[TextureIdHighAlpine]={95,95,95},
		[TextureIdLowAlpine]={190,190,190},
		[TextureIdSheepN]={255,0,0},
		[TextureIdSheepE]={255,0,0},
		[TextureIdSheepS]={255,0,0},
		[TextureIdSheepW]={255,0,0},
		[TextureIdRoseBush]={0,100,0},
		[TextureIdCoins]={255,0,0},
		[TextureIdDog]={255,0,0},
		[TextureIdChestClosed]={255,0,0},
		[TextureIdChestOpen]={255,0,0},
	};

	bool success=true;
	for(unsigned textureId=0; textureId<TextureIdNB; ++textureId)
		success&=map->addTexture(new MapTexture(textureId, texturePaths[textureId], textureScales[textureId], textureColours[textureId][0], textureColours[textureId][1], textureColours[textureId][2]));

	return success;
};

bool demogenAddItems(class Map *map) {
	const char *itemNames[ItemIdNB]={
		[ItemIdCoins]="Coins",
	};

	bool success=true;
	for(unsigned itemId=0; itemId<ItemIdNB; ++itemId)
		success&=map->addItem(new MapItem(itemId, itemNames[itemId]));

	return success;
};

MapObject *demogenAddObjectSheep(class Map *map, CoordAngle rotation, const CoordVec &pos) {
	// Create hitmask.
	const char hitmaskStr[64+1]=
		"________"
		"________"
		"_######_"
		"_######_"
		"_######_"
		"_######_"
		"_######_"
		"_######_";
	HitMask hitmask(hitmaskStr);

	// Create object.
	MapObject *object=new MapObject(rotation, pos, 1, 1);
	object->setHitMaskByTileOffset(0, 0, hitmask);
	object->setTextureIdForAngle(CoordAngle0, TextureIdSheepS);
	object->setTextureIdForAngle(CoordAngle90, TextureIdSheepW);
	object->setTextureIdForAngle(CoordAngle180, TextureIdSheepN);
	object->setTextureIdForAngle(CoordAngle270, TextureIdSheepE);

	// Add object to map.
	if (!map->addObject(object)) {
		delete object;
		return NULL;
	}

	return object;
}

MapObject *demogenAddObjectDog(class Map *map, CoordAngle rotation, const CoordVec &pos) {
	// Create hitmask.
	const char hitmaskStr[64+1]=
		"________"
		"________"
		"________"
		"_#####__"
		"_#####__"
		"_#####__"
		"___###__"
		"________";
	HitMask hitmask(hitmaskStr);

	// Create object.
	MapObject *object=new MapObject(rotation, pos, 1, 1);
	object->setHitMaskByTileOffset(0, 0, hitmask);
	object->setTextureIdForAngle(CoordAngle0, TextureIdDog);
	object->setTextureIdForAngle(CoordAngle90, TextureIdDog);
	object->setTextureIdForAngle(CoordAngle180, TextureIdDog);
	object->setTextureIdForAngle(CoordAngle270, TextureIdDog);

	// Add object to map.
	if (!map->addObject(object)) {
		delete object;
		return NULL;
	}

	return object;
}

MapObject *demogenAddObjectOldBeardMan(class Map *map, CoordAngle rotation, const CoordVec &pos) {
	// Create hitmask.
	const unsigned hitmaskW=4, hitmaskH=5;
	HitMask hitmask;
	unsigned x, y;
	for(y=(8-hitmaskH)/2; y<(8+hitmaskH)/2; ++y)
		for(x=(8-hitmaskW)/2; x<(8+hitmaskW)/2; ++x)
			hitmask.setXY(x, y, true);

	// Create object.
	MapObject *object=new MapObject(rotation, pos, 1, 1);
	object->setHitMaskByTileOffset(0, 0, hitmask);
	object->setTextureIdForAngle(CoordAngle0, TextureIdOldManS);
	object->setTextureIdForAngle(CoordAngle90, TextureIdOldManW);
	object->setTextureIdForAngle(CoordAngle180, TextureIdOldManN);
	object->setTextureIdForAngle(CoordAngle270, TextureIdOldManE);

	// Add object to map.
	if (!map->addObject(object)) {
		delete object;
		return NULL;
	}

	return object;
}

void demogenInitModifyTilesFunctor(unsigned threadId, class Map *map, unsigned x, unsigned y, void *userData) {
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

	const double temperatureRandomOffset=mapData->temperatureNoise->eval(x/((double)mapData->width), y/((double)mapData->height));

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

	for(unsigned i=0; i<MapTile::layersMax; ++i) {
		MapTile::Layer layer={.textureId=MapTexture::IdMax, .hitmask=HitMask()};
		tile->setLayer(i, layer);
	}
}

void demogenGroundModifyTilesFunctor(unsigned threadId, class Map *map, unsigned x, unsigned y, void *userData) {
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
	MapTexture::Id idA=MapTexture::IdMax, idB=MapTexture::IdMax;
	double factor;
	if (height<=mapData->map->seaLevel) {
		// ocean
		idA=TextureIdDeepWater;
		idB=TextureIdWater;
		factor=((height-map->minHeight)/(mapData->map->seaLevel-map->minHeight));

		double skewThreshold=0.7; // >0.5 shifts towards idA
		factor=(factor>skewThreshold ? (factor-skewThreshold)/(2.0*(1.0-skewThreshold))+0.5 : factor/(2*skewThreshold));
	} else if (height<=mapData->map->alpineLevel) {
		// River?
		if (tile->getMoisture()>mapData->riverMoistureThreshold) {
			idA=TextureIdRiver;
			idB=TextureIdRiver;
			factor=0.0;
		} else {
			// land
			if (temperature<=mapData->coldThreshold) {
				// between snow and grass
				idA=TextureIdSnow;
				idB=TextureIdGrass0;
				factor=(temperature-map->minTemperature)/(mapData->coldThreshold-map->minTemperature);
			} else if (temperature<=mapData->hotThreshold) {
				// grass
				idA=TextureIdGrass0;
				idB=TextureIdGrass0;
				factor=(temperature-mapData->coldThreshold)/(mapData->hotThreshold-mapData->coldThreshold);
			} else {
				// between sand and hot sand
				idA=TextureIdSand;
				idB=TextureIdHotSand;
				factor=(temperature-mapData->hotThreshold)/(map->maxTemperature-mapData->hotThreshold);
				double skewThreshold=0.5; // >0.5 shifts towards idA
				factor=(factor>skewThreshold ? (factor-skewThreshold)/(2.0*(1.0-skewThreshold))+0.5 : factor/(2*skewThreshold));
			}
		}
	} else {
		// alpine
		idA=TextureIdLowAlpine;
		idB=TextureIdHighAlpine;
		factor=(height-map->alpineLevel)/(map->maxHeight-map->alpineLevel);

		double skewThreshold=0.75; // >0.5 shifts towards idA
		factor=(factor>skewThreshold ? (factor-skewThreshold)/(2.0*(1.0-skewThreshold))+0.5 : factor/(2*skewThreshold));
	}
	assert(factor>=0.0 && factor<=1.0);

	MapTexture::Id textureId=(factor<=0.5 ? idA : idB);
	assert(textureId!=MapTexture::IdMax);

	// Update tile layer.
	MapTile::Layer layer={.textureId=textureId, .hitmask=HitMask()};
	tile->setLayer(DemoGenTileLayerGround, layer);

	// Update map data.
	if (textureId==TextureIdWater || textureId==TextureIdDeepWater || textureId==TextureIdRiver)
		++mapData->waterCount;
	else
		++mapData->landCount;
	if (textureId>=TextureIdGrass0 && textureId<=TextureIdGrass5)
		++mapData->arableCount;
	++mapData->totalCount;
}

void demogenGrassForestModifyTilesFunctor(unsigned threadId, class Map *map, unsigned x, unsigned y, void *userData) {
	assert(map!=NULL);
	assert(userData!=NULL);

	const DemogenMapData *mapData=(const DemogenMapData *)userData;

	// Choose parameters.
	const double temperatureThreshold=0.6;
	const double hotThreshold=0.7;

	assert(0.0<=temperatureThreshold);
	assert(temperatureThreshold<=hotThreshold);
	assert(hotThreshold<=1.0);

	// Grab tile and temperature.
	MapTile *tile=map->getTileAtOffset(x, y, Engine::Map::Map::GetTileFlag::None);
	if (tile==NULL)
		return;

	const double temperature=tile->getTemperature();

	// Not enough temperature to support anything?
	if (temperature<temperatureThreshold)
		return;

	// Random chance of a 'tree'.
	double randomValue=Util::randFloatInInterval(0.0, 1.0);
	if (randomValue<0.90)
		return;

	// Check if broad noise indicates a forest should be here
	if (mapData->forestNoise->eval(x/((double)map->getWidth()), y/((double)map->getHeight()))<map->forestLevel)
		return;

	// Check layers.
	if (tile->getLayer(DemoGenTileLayerGround)->textureId!=TextureIdGrass0)
		return;
	if (tile->getLayer(DemoGenTileLayerDecoration)->textureId!=MapTexture::IdMax)
		return;
	if (tile->getLayer(DemoGenTileLayerHalf)->textureId!=MapTexture::IdMax)
		return;
	if (tile->getLayer(DemoGenTileLayerFull)->textureId!=MapTexture::IdMax)
		return;

	// Choose new texture.
	MapTexture::Id textureId=MapTexture::IdMax;

	if (temperature<hotThreshold) {
		const double probabilities[]={0.3,0.2,0.2,0.2,0.1};
		unsigned index=Util::chooseWithProb(probabilities, sizeof(probabilities)/sizeof(probabilities[0]));
		textureId=TextureIdGrass1+index;
		assert(textureId>=TextureIdGrass1 && textureId<=TextureIdGrass5);
	} else
		textureId=TextureIdTree1;

	assert(textureId!=MapTexture::IdMax);

	// Update tile layer.
	MapTile::Layer layer={.textureId=textureId};
	tile->setLayer(DemoGenTileLayerFull, layer);

	// We have made a change - mark region dirty.
	map->markRegionDirtyAtTileOffset(x, y, false);
}

void demogenSandForestModifyTilesFunctor(unsigned threadId, class Map *map, unsigned x, unsigned y, void *userData) {
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

	// Check if broad noise indicates a forest should be here
	if (mapData->forestNoise->eval(x/((double)map->getWidth()), y/((double)map->getHeight()))<map->forestLevel)
		return;

	// Check layers.
	if (tile->getLayer(DemoGenTileLayerGround)->textureId!=TextureIdSand && tile->getLayer(DemoGenTileLayerGround)->textureId!=TextureIdHotSand)
		return;
	if (tile->getLayer(DemoGenTileLayerDecoration)->textureId!=MapTexture::IdMax)
		return;
	if (tile->getLayer(DemoGenTileLayerHalf)->textureId!=MapTexture::IdMax)
		return;
	if (tile->getLayer(DemoGenTileLayerFull)->textureId!=MapTexture::IdMax)
		return;

	// Update tile layer.
	MapTile::Layer layer={.textureId=TextureIdTree3};
	tile->setLayer(DemoGenTileLayerFull, layer);

	// We have made a change - mark region dirty.
	map->markRegionDirtyAtTileOffset(x, y, false);
}

void demogenGrassSheepModifyTilesFunctor(unsigned threadId, class Map *map, unsigned x, unsigned y, void *userData) {
	assert(map!=NULL);
	assert(userData!=NULL);

	const DemogenMapData *mapData=(const DemogenMapData *)userData;
	(void)mapData; // HACK to stop compiler unused variable warning

	// Grab tile.
	const MapTile *tile=map->getTileAtOffset(x, y, Engine::Map::Map::GetTileFlag::None);
	if (tile==NULL)
		return;

	// Check layers.
	if (tile->getLayer(DemoGenTileLayerGround)->textureId<TextureIdGrass0 || tile->getLayer(DemoGenTileLayerGround)->textureId>TextureIdGrass5)
		return;

	// Decide whether to place a sheep here.
	if (rand()%(32*32)!=0)
		return;

	// Add object.
	CoordVec pos(x*CoordsPerTile, y*CoordsPerTile);
	MapObject *sheep=demogenAddObjectSheep(map, CoordAngle0, pos);
	if (sheep==NULL)
		return;
	sheep->setMovementModeRandomRadius(pos, 10*CoordsPerTile);
}

void demogenTownFolkModifyTilesFunctor(unsigned threadId, class Map *map, unsigned x, unsigned y, void *userData) {
	assert(map!=NULL);
	assert(userData!=NULL);

	const DemogenMapData *mapData=(const DemogenMapData *)userData;
	(void)mapData; // HACK to stop compiler unused variable warning

	// Grab tile.
	const MapTile *tile=map->getTileAtOffset(x, y, Engine::Map::Map::GetTileFlag::None);
	if (tile==NULL)
		return;

	// Check layers.
	if (tile->getLayer(DemoGenTileLayerGround)->textureId!=TextureIdBrickPath && tile->getLayer(DemoGenTileLayerGround)->textureId!=TextureIdDirt)
		return;

	// Decide whether to place someone here.
	if (rand()%64!=0)
		return;

	// Choose and add object.
	CoordVec pos(x*CoordsPerTile, y*CoordsPerTile);
	MapObject *object=(rand()%3==0 ? demogenAddObjectDog(map, CoordAngle0, pos) : demogenAddObjectOldBeardMan(map, CoordAngle0, pos));
	if (object==NULL)
		return;
	object->setMovementModeRandomRadius(pos, 10*CoordsPerTile);
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
			if (layerGround<TextureIdGrass0 || layerGround>TextureIdGrass5)
				return false;

			// Look for half obstacles.
			MapTexture::Id layerHalf=tile->getLayer(DemoGenTileLayerHalf)->textureId;
			if (layerHalf!=MapTexture::IdMax)
				return false;

			// Look for full obstacles.
			MapTexture::Id layerFull=tile->getLayer(DemoGenTileLayerFull)->textureId;
			if (layerFull!=MapTexture::IdMax)
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
	const double desiredForestFraction=0.4;

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
	if (argc<4 || argc>6) {
		printf("Usage: %s width height outputpath [seed=1 [threadcount=1]]\n", argv[0]);
		return EXIT_FAILURE;
	}

	mapData.width=atoi(argv[1]);
	mapData.height=atoi(argv[2]);
	const char *outputPath=argv[3];
	uint64_t seed=(argc>4 ? atoll(argv[4]) : 1);
	unsigned threadCount=(argc>5 ? atoi(argv[5]) : 1);

	if (mapData.width<=0 || mapData.height<=0) {
		printf("Bad width or height (%i and %i)", mapData.width, mapData.height);
		return EXIT_FAILURE;
	}

	// Init
	Util::TimeMs startTime=Util::getTimeMs();

	setlocale(LC_NUMERIC, "");

	// Create Map.
	printf("Creating map (seed %lu, thread count %u)...\n", seed, threadCount);

	try {
		mapData.map=new class Map(outputPath, mapData.width, mapData.height);
	} catch (std::exception& e) {
		std::cout << "Could not create map: " << e.what() << '\n';
		return EXIT_FAILURE;
	}

	// Add textures.
	printf("Creating textures...\n");
	if (!demogenAddTextures(mapData.map)) {
		printf("Could not add textures.\n");
		if (mapData.map!=NULL)
			delete mapData.map;
		return EXIT_FAILURE;
	}

	// Add items.
	printf("Creating items...\n");
	if (!demogenAddItems(mapData.map)) {
		printf("Could not add items.\n");
		if (mapData.map!=NULL)
			delete mapData.map;
		return EXIT_FAILURE;
	}

	// Run init modify tiles function.
	mapData.heightNoise=new FbnNoise(seed+17, 8, 8.0);
	mapData.temperatureNoise=new FbnNoise(seed+19, 8, 1.0);

	const char *progressStringInit="Initializing tile parameters ";
	Gen::modifyTiles(mapData.map, 0, 0, mapData.width, mapData.height, threadCount, &demogenInitModifyTilesFunctor, &mapData, &utilProgressFunctorString, (void *)progressStringInit);
	printf("\n");

	delete mapData.heightNoise;
	mapData.heightNoise=NULL;
	delete mapData.temperatureNoise;
	mapData.temperatureNoise=NULL;

	// Recalculate stats such as min/max height required for future calls.
	const char *progressStringGlobalStats1="Collecting global statistics (1/3) ";
	Gen::recalculateStats(mapData.map, threadCount, &utilProgressFunctorString, (void *)progressStringGlobalStats1);
	printf("\n");

	printf("	Min height %f, max height %f\n", mapData.map->minHeight, mapData.map->maxHeight);
	printf("	Min temperature %f, max temperature %f\n", mapData.map->minTemperature, mapData.map->maxTemperature);
	printf("	Min moisture %f, max moisture %f\n", mapData.map->minMoisture, mapData.map->maxMoisture);

	/*
	// Calculate sea level.
	printf("Searching for sea level (with desired land coverage %.2f%%) (1/3)...\n", desiredLandFraction*100.0);
	mapData.map->seaLevel=Gen::search(mapData.map, 0, 0, mapData.width, mapData.height, threadCount, true, 63, desiredLandFraction, 0.45, mapData.map->minHeight, mapData.map->maxHeight, &Gen::searchGetFunctorHeight, NULL);
	printf("	Sea level %f\n", mapData.map->seaLevel);

	// Run glacier calculation.

	Note: this is disabled for now as it is quite slow, although it does produce good effects

	const char *progressStringGlaciers="Applying glacial effects ";
	Gen::ParticleFlow glacierGen(mapData.map, 7, false);
	glacierGen.dropParticles(0, 0, mapData.width, mapData.height, 1.0/64.0, threadCount, &utilProgressFunctorString, (void *)progressStringGlaciers);
	printf("\n");

	// Recalculate stats such as min/max height required for future calls.
	const char *progressStringGlobalStats2="Collecting global statistics (2/3) ";
	Gen::recalculateStats(mapData.map, &utilProgressFunctorString, (void *)progressStringGlobalStats2);
	printf("\n");

	printf("	Min height %f, max height %f\n", mapData.map->minHeight, mapData.map->maxHeight);
	printf("	Min temperature %f, max temperature %f\n", mapData.map->minTemperature, mapData.map->maxTemperature);
	printf("	Min moisture %f, max moisture %f\n", mapData.map->minMoisture, mapData.map->maxMoisture);
	*/

	// Calculate sea level.
	printf("Searching for sea level (with desired land coverage %.2f%%) (2/3)...\n", desiredLandFraction*100.0);
	mapData.map->seaLevel=Gen::search(mapData.map, 0, 0, mapData.width, mapData.height, threadCount, true, 63, desiredLandFraction, 0.45, mapData.map->minHeight, mapData.map->maxHeight, &Gen::searchGetFunctorHeight, NULL);
	printf("	Sea level %f\n", mapData.map->seaLevel);

	// Run moisture/river calculation.
	const char *progressStringRivers="Generating moisture/river data ";
	Gen::ParticleFlow riverGen(mapData.map, 2, true);
	riverGen.dropParticles(0, 0, mapData.width, mapData.height, 1.0/16.0, 1/*.....threadCount*/, &utilProgressFunctorString, (void *)progressStringRivers);
	printf("\n");

	// Recalculate stats such as min/max height required for future calls.
	const char *progressStringGlobalStats3="Collecting global statistics (3/3) ";
	Gen::recalculateStats(mapData.map, threadCount, &utilProgressFunctorString, (void *)progressStringGlobalStats3);
	printf("\n");

	printf("	Min height %f, max height %f\n", mapData.map->minHeight, mapData.map->maxHeight);
	printf("	Min temperature %f, max temperature %f\n", mapData.map->minTemperature, mapData.map->maxTemperature);
	printf("	Min moisture %f, max moisture %f\n", mapData.map->minMoisture, mapData.map->maxMoisture);

	// Calculate variable levels/values/thresholds
	double desiredColdCoverage=0.4;
	double desiredHotCoverage=0.2;
	double desiredRiverCoverage=0.005;
	mapData.forestNoise=new FbnNoise(seed+23, 8, 8.0);

	printf("Searching for: sea level (with desired coverage %.2f%%), alpine level (%.2f%%), cold threshold (%.2f%%), hot threshold level (%.2f%%), river moisture threshold (%.2f%%), forest level (%.2f%%)...\n", desiredLandFraction*100.0, desiredAlpineFraction*100.0, desiredColdCoverage*100.0, desiredHotCoverage*100.0, desiredRiverCoverage*100.0, desiredForestFraction*100.0);

	Gen::SearchManyEntry searchManyArray[]={
		{.threshold=desiredLandFraction, .epsilon=0.45, .sampleMin=mapData.map->minHeight, .sampleMax=mapData.map->maxHeight, .sampleCount=63, .getFunctor=&Gen::searchGetFunctorHeight, .getUserData=NULL},
		{.threshold=desiredAlpineFraction, .epsilon=0.45, .sampleMin=mapData.map->minHeight, .sampleMax=mapData.map->maxHeight, .sampleCount=63, .getFunctor=&Gen::searchGetFunctorHeight, .getUserData=NULL},
		{.threshold=desiredColdCoverage, .epsilon=0.45, .sampleMin=mapData.map->minTemperature, .sampleMax=mapData.map->maxTemperature, .sampleCount=63, .getFunctor=&Gen::searchGetFunctorTemperature, .getUserData=NULL},
		{.threshold=desiredHotCoverage, .epsilon=0.45, .sampleMin=mapData.map->minTemperature, .sampleMax=mapData.map->maxTemperature, .sampleCount=63, .getFunctor=&Gen::searchGetFunctorTemperature, .getUserData=NULL},
		{.threshold=desiredRiverCoverage, .epsilon=0.45, .sampleMin=mapData.map->minMoisture, .sampleMax=mapData.map->maxMoisture, .sampleCount=63, .getFunctor=&Gen::searchGetFunctorMoisture, .getUserData=NULL},
		{.threshold=desiredForestFraction, .epsilon=0.005, .sampleMin=-1.0, .sampleMax=1.0, .sampleCount=63, .getFunctor=&Gen::searchGetFunctorNoise, .getUserData=mapData.forestNoise},
	};

	Gen::searchMany(mapData.map, 0, 0, mapData.width, mapData.height, threadCount, true, sizeof(searchManyArray)/sizeof(searchManyArray[0]), searchManyArray);
	mapData.map->seaLevel=searchManyArray[0].result;
	mapData.map->alpineLevel=searchManyArray[1].result;
	mapData.coldThreshold=searchManyArray[2].result;
	mapData.hotThreshold=searchManyArray[3].result;
	mapData.riverMoistureThreshold=searchManyArray[4].result;
	mapData.map->forestLevel=searchManyArray[5].result;

	printf("	Sea level %f, alpine level %f, cold temperature %f, hot temperature %f, river moisture threshold %f, forest level %f\n", mapData.map->seaLevel, mapData.map->alpineLevel, mapData.coldThreshold, mapData.hotThreshold, mapData.riverMoistureThreshold, mapData.map->forestLevel);

	// Run modify tiles for forests.
	size_t biomesModifyTilesArrayCount=3;
	Gen::ModifyTilesManyEntry biomesModifyTilesArray[biomesModifyTilesArrayCount];
	biomesModifyTilesArray[0].functor=&demogenGroundModifyTilesFunctor;
	biomesModifyTilesArray[0].userData=&mapData;
	biomesModifyTilesArray[1].functor=&demogenGrassForestModifyTilesFunctor;
	biomesModifyTilesArray[1].userData=&mapData;
	biomesModifyTilesArray[2].functor=&demogenSandForestModifyTilesFunctor;
	biomesModifyTilesArray[2].userData=&mapData;

	const char *progressStringBiomes="Assigning tile textures for biomes ";
	Gen::modifyTilesMany(mapData.map, 0, 0, mapData.width, mapData.height, threadCount, biomesModifyTilesArrayCount, biomesModifyTilesArray, &utilProgressFunctorString, (void *)progressStringBiomes);
	printf("\n");

	delete mapData.forestNoise;
	mapData.forestNoise=NULL;

	// Compute more map data.
	if (mapData.totalCount>0)
		mapData.landFraction=((double)mapData.landCount)/mapData.totalCount;
	else
		mapData.landFraction=0.0;

	mapData.landSqKm=mapData.landCount/(1000.0*1000.0);
	mapData.arableSqKm=mapData.arableCount/(1000.0*1000.0);
	mapData.peoplePerSqKm=150.0;
	mapData.totalPopulation=mapData.arableSqKm*mapData.peoplePerSqKm;

	printf("	Land %'.1fkm^2 (of which %'.1fkm^2 is arable), water %'.1fkm^2, land fraction %.2f%%\n", mapData.landSqKm, mapData.arableSqKm, mapData.waterCount/(1000.0*1000.0), mapData.landFraction*100.0);
	printf("	People per km^2 %.0f, total pop %.0f\n", mapData.peoplePerSqKm, mapData.totalPopulation);

	// Add towns.
	printf("Adding towns...\n");
	Gen::AddTownParameters townParams={
		.roadTileLayer=DemoGenTileLayerGround,
		.houseDecorationLayer=DemoGenTileLayerDecoration,
		.testFunctor=&demogenTownTileTestFunctor,
		.testFunctorUserData=NULL,
		.textureIdMajorPath=TextureIdBrickPath,
		.textureIdMinorPath=TextureIdDirt,
		.textureIdShopSignNone=TextureIdHouseWall2,
		.textureIdShopSignCobbler=TextureIdShopCobbler,
	};
	Gen::AddHouseParameters houseParams={
		.flags=(Gen::AddHouseFlags)(Gen::AddHouseFlags::ShowChimney|Gen::AddHouseFlags::AddDecoration),
		.tileLayer=DemoGenTileLayerFull,
		.decorationLayer=DemoGenTileLayerDecoration,
		.testFunctor=NULL,
		.testFunctorUserData=NULL,
		.textureIdWall0=TextureIdHouseWall3,
		.textureIdWall1=TextureIdHouseWall2,
		.textureIdWall2=TextureIdHouseWall4,
		.textureIdHouseDoorBL=TextureIdHouseDoorBL,
		.textureIdHouseDoorBR=TextureIdHouseDoorBR,
		.textureIdHouseDoorTL=TextureIdHouseDoorTL,
		.textureIdHouseDoorTR=TextureIdHouseDoorTR,
		.textureIdHouseRoof=TextureIdHouseRoof,
		.textureIdHouseRoofTop=TextureIdHouseRoofTop,
		.textureIdHouseChimneyTop=TextureIdHouseChimneyTop,
		.textureIdHouseChimney=TextureIdHouseChimney,
		.textureIdBrickPath=TextureIdBrickPath,
		.textureIdRoseBush=TextureIdRoseBush,
	};
	Gen::addTowns(mapData.map, 0, 0, mapData.width, mapData.height, &townParams, &houseParams, mapData.totalPopulation);
	printf("\n");

	// Run modify tiles npcs/animals.
	size_t npcModifyTilesArrayCount=2;
	Gen::ModifyTilesManyEntry npcModifyTilesArray[npcModifyTilesArrayCount];
	npcModifyTilesArray[0].functor=&demogenGrassSheepModifyTilesFunctor;
	npcModifyTilesArray[0].userData=&mapData;
	npcModifyTilesArray[1].functor=&demogenTownFolkModifyTilesFunctor;
	npcModifyTilesArray[1].userData=&mapData;

	const char *progressStringNpcsAnimals="Adding npcs and animals ";
	Gen::modifyTilesMany(mapData.map, 0, 0, mapData.width, mapData.height, 1, npcModifyTilesArrayCount, npcModifyTilesArray, &utilProgressFunctorString, (void *)progressStringNpcsAnimals);
	printf("\n");

	// Landmass (contintent/island) identification
	unsigned landmassCacheBits[Gen::EdgeDetect::DirectionNB];
	landmassCacheBits[Gen::EdgeDetect::DirectionEast]=60;
	landmassCacheBits[Gen::EdgeDetect::DirectionNorth]=61;
	landmassCacheBits[Gen::EdgeDetect::DirectionWest]=62;
	landmassCacheBits[Gen::EdgeDetect::DirectionSouth]=63;

	Gen::EdgeDetect landmassEdgeDetect(mapData.map);
	landmassEdgeDetect.traceFast(threadCount, &Gen::edgeDetectLandSampleFunctor, NULL, &Gen::edgeDetectBitsetNEdgeFunctor, (void *)(uintptr_t)Gen::TileBitsetIndexLandmassBorder, &utilProgressFunctorString, (void *)"Identifying landmass boundaries via edge detection ");
	printf("\n");

	Gen::FloodFill landmassFloodFill(mapData.map, 63);
	landmassFloodFill.fill(&Gen::floodFillBitsetNBoundaryFunctor, (void *)(uintptr_t)Gen::TileBitsetIndexLandmassBorder, &demogenFloodFillLandmassFillFunctor, NULL, &utilProgressFunctorString, (void *)"Identifying individual landmasses via flood-fill ");
	printf("\n");

	// Run contour line detection logic
	Gen::EdgeDetect heightContourEdgeDetect(mapData.map);
	heightContourEdgeDetect.traceFastHeightContours(threadCount, 9, &utilProgressFunctorString, (void *)"Height contour edge detection ");
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
	Util::printTime(totalTime);
	printf("\n");

	// Tidy up
	delete mapData.map;
	return EXIT_SUCCESS;
}
