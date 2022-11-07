#include <cassert>
#include <cfloat>
#include <cmath>
#include <cstdbool>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <new>

#include "../engine/map/map.h"
#include "../engine/map/mapgen.h"
#include "../engine/map/mapobject.h"
#include "../engine/fbnnoise.h"
#include "../engine/util.h"

using namespace Engine;
using namespace Engine::Map;

static const MapTexture::Id TextureIdGrass0=0;
static const MapTexture::Id TextureIdGrass1=1;
static const MapTexture::Id TextureIdGrass2=2;
static const MapTexture::Id TextureIdGrass3=3;
static const MapTexture::Id TextureIdGrass4=4;
static const MapTexture::Id TextureIdGrass5=5;
static const MapTexture::Id TextureIdBrickPath=6;
static const MapTexture::Id TextureIdDirt=7;
static const MapTexture::Id TextureIdDock=8;
static const MapTexture::Id TextureIdWater=9;
static const MapTexture::Id TextureIdTree1=10;
static const MapTexture::Id TextureIdTree2=11;
static const MapTexture::Id TextureIdTree3=12;
static const MapTexture::Id TextureIdMan1=13;
static const MapTexture::Id TextureIdOldManN=14;
static const MapTexture::Id TextureIdOldManE=15;
static const MapTexture::Id TextureIdOldManS=16;
static const MapTexture::Id TextureIdOldManW=17;
static const MapTexture::Id TextureIdHouseDoorBL=18;
static const MapTexture::Id TextureIdHouseDoorBR=19;
static const MapTexture::Id TextureIdHouseDoorTL=20;
static const MapTexture::Id TextureIdHouseDoorTR=21;
static const MapTexture::Id TextureIdHouseRoof=22;
static const MapTexture::Id TextureIdHouseRoofTop=23;
static const MapTexture::Id TextureIdHouseWall2=24;
static const MapTexture::Id TextureIdHouseWall3=25;
static const MapTexture::Id TextureIdHouseWall4=26;
static const MapTexture::Id TextureIdHouseChimney=27;
static const MapTexture::Id TextureIdHouseChimneyTop=28;
static const MapTexture::Id TextureIdSand=29;
static const MapTexture::Id TextureIdHotSand=30;
static const MapTexture::Id TextureIdShopCobbler=31;
static const MapTexture::Id TextureIdSnow=32;
static const MapTexture::Id TextureIdDeepWater=33;
static const MapTexture::Id TextureIdRiver=34;
static const MapTexture::Id TextureIdHighAlpine=35;
static const MapTexture::Id TextureIdLowAlpine=36;
static const MapTexture::Id TextureIdSheepN=37;
static const MapTexture::Id TextureIdSheepE=38;
static const MapTexture::Id TextureIdSheepS=39;
static const MapTexture::Id TextureIdSheepW=40;
static const MapTexture::Id TextureIdDog=41;
static const MapTexture::Id TextureIdRoseBush=42;
static const MapTexture::Id TextureIdCoins=43;
static const MapTexture::Id TextureIdChestClosed=44;
static const MapTexture::Id TextureIdChestOpen=45;
static const MapTexture::Id TextureIdNB=46;

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

bool demogenAddBaseTextures(class Map *map);

void demogenInitModifyTilesFunctor(unsigned threadId, class Map *map, unsigned x, unsigned y, void *userData);
void demogenGroundModifyTilesFunctor(unsigned threadId, class Map *map, unsigned x, unsigned y, void *userData);
void demogenGrassForestModifyTilesFunctor(unsigned threadId, class Map *map, unsigned x, unsigned y, void *userData);
void demogenSandForestModifyTilesFunctor(unsigned threadId, class Map *map, unsigned x, unsigned y, void *userData);
void demogenGrassSheepModifyTilesFunctor(unsigned threadId, class Map *map, unsigned x, unsigned y, void *userData);
void demogenTownFolkModifyTilesFunctor(unsigned threadId, class Map *map, unsigned x, unsigned y, void *userData);

bool demogenTownTileTestFunctor(class Map *map, int x, int y, int w, int h, void *userData);

void demogenFloodFillLandmassFillFunctor(class Map *map, unsigned x, unsigned y, unsigned groupId, void *userData);

bool demogenAddBaseTextures(class Map *map) {
	const char *texturePaths[TextureIdNB]={
		[TextureIdGrass0]="../images/tiles/grass0.png",
		[TextureIdGrass1]="../images/tiles/grass1.png",
		[TextureIdGrass2]="../images/tiles/grass2.png",
		[TextureIdGrass3]="../images/tiles/grass3.png",
		[TextureIdGrass4]="../images/tiles/grass4.png",
		[TextureIdGrass5]="../images/tiles/grass5.png",
		[TextureIdBrickPath]="../images/tiles/tile.png",
		[TextureIdDirt]="../images/tiles/dirt.png",
		[TextureIdDock]="../images/tiles/dock.png",
		[TextureIdWater]="../images/tiles/water.png",
		[TextureIdTree1]="../images/objects/tree1.png",
		[TextureIdTree2]="../images/objects/tree2.png",
		[TextureIdTree3]="../images/objects/tree3.png",
		[TextureIdMan1]="../images/objects/man1.png",
		[TextureIdOldManN]="../images/npcs/oldbeardman/north.png",
		[TextureIdOldManE]="../images/npcs/oldbeardman/east.png",
		[TextureIdOldManS]="../images/npcs/oldbeardman/south.png",
		[TextureIdOldManW]="../images/npcs/oldbeardman/west.png",
		[TextureIdHouseDoorBL]="../images/tiles/house/doorbl.png",
		[TextureIdHouseDoorBR]="../images/tiles/house/doorbr.png",
		[TextureIdHouseDoorTL]="../images/tiles/house/doortl.png",
		[TextureIdHouseDoorTR]="../images/tiles/house/doortr.png",
		[TextureIdHouseRoof]="../images/tiles/house/roof.png",
		[TextureIdHouseRoofTop]="../images/tiles/house/rooftop.png",
		[TextureIdHouseWall2]="../images/tiles/house/wall2.png",
		[TextureIdHouseWall3]="../images/tiles/house/wall3.png",
		[TextureIdHouseWall4]="../images/tiles/house/wall4.png",
		[TextureIdHouseChimney]="../images/tiles/house/chimney.png",
		[TextureIdHouseChimneyTop]="../images/tiles/house/chimneytop.png",
		[TextureIdSand]="../images/tiles/sand.png",
		[TextureIdHotSand]="../images/tiles/hotsand.png",
		[TextureIdSnow]="../images/tiles/snow.png",
		[TextureIdShopCobbler]="../images/tiles/shops/cobbler.png",
		[TextureIdDeepWater]="../images/tiles/deepwater.png",
		[TextureIdRiver]="../images/tiles/water.png",
		[TextureIdHighAlpine]="../images/tiles/highalpine.png",
		[TextureIdLowAlpine]="../images/tiles/lowalpine.png",
		[TextureIdSheepN]="../images/npcs/sheep/north.png",
		[TextureIdSheepE]="../images/npcs/sheep/east.png",
		[TextureIdSheepS]="../images/npcs/sheep/south.png",
		[TextureIdSheepW]="../images/npcs/sheep/west.png",
		[TextureIdRoseBush]="../images/objects/rosebush.png",
		[TextureIdCoins]="../images/objects/coins.png",
		[TextureIdDog]="../images/npcs/dog/east.png",
		[TextureIdChestClosed]="../images/objects/chestclosed.png",
		[TextureIdChestOpen]="../images/objects/chestopen.png",
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
	MapObject *sheep=MapGen::addBuiltinObject(map, MapGen::BuiltinObject::Sheep, CoordAngle0, pos);
	if (sheep==NULL)
		return;
	sheep->setMovementModeRandomRadius(pos, 10*CoordsPerTile);
}

void demogenTownFolkModifyTilesFunctor(unsigned threadId, class Map *map, unsigned x, unsigned y, void *userData) {
	assert(map!=NULL);
	assert(userData!=NULL);

	const DemogenMapData *mapData=(const DemogenMapData *)userData;

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

	// Choose object.
	MapGen::BuiltinObject builtinObject=(rand()%3==0 ? MapGen::BuiltinObject::Dog : MapGen::BuiltinObject::OldBeardMan);

	// Add object.
	CoordVec pos(x*CoordsPerTile, y*CoordsPerTile);
	MapObject *object=MapGen::addBuiltinObject(map, builtinObject, CoordAngle0, pos);
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
	if (!demogenAddBaseTextures(mapData.map)) {
		printf("Could not add base textures.\n");
		if (mapData.map!=NULL)
			delete mapData.map;
		return EXIT_FAILURE;
	}

	// Run init modify tiles function.
	mapData.heightNoise=new FbnNoise(seed+17, 8, 8.0);
	mapData.temperatureNoise=new FbnNoise(seed+19, 8, 1.0);

	const char *progressStringInit="Initializing tile parameters ";
	MapGen::modifyTiles(mapData.map, 0, 0, mapData.width, mapData.height, threadCount, &demogenInitModifyTilesFunctor, &mapData, &mapGenModifyTilesProgressString, (void *)progressStringInit);
	printf("\n");

	delete mapData.heightNoise;
	mapData.heightNoise=NULL;
	delete mapData.temperatureNoise;
	mapData.temperatureNoise=NULL;

	// Recalculate stats such as min/max height required for future calls.
	const char *progressStringGlobalStats1="Collecting global statistics (1/3) ";
	MapGen::recalculateStats(mapData.map, 0, 0, mapData.width, mapData.height, threadCount, &mapGenModifyTilesProgressString, (void *)progressStringGlobalStats1);
	printf("\n");

	printf("	Min height %f, max height %f\n", mapData.map->minHeight, mapData.map->maxHeight);
	printf("	Min temperature %f, max temperature %f\n", mapData.map->minTemperature, mapData.map->maxTemperature);
	printf("	Min moisture %f, max moisture %f\n", mapData.map->minMoisture, mapData.map->maxMoisture);

	/*
	// Calculate sea level.
	printf("Searching for sea level (with desired land coverage %.2f%%) (1/3)...\n", desiredLandFraction*100.0);
	mapData.map->seaLevel=MapGen::narySearch(mapData.map, 0, 0, mapData.width, mapData.height, threadCount, 63, desiredLandFraction, 0.45, mapData.map->minHeight, mapData.map->maxHeight, &mapGenNarySearchGetFunctorHeight, NULL);
	printf("	Sea level %f\n", mapData.map->seaLevel);

	// Run glacier calculation.

	Note: this is disabled for now as it is quite slow, although it does produce good effects

	const char *progressStringGlaciers="Applying glacial effects ";
	MapGen::ParticleFlow glacierGen(mapData.map, 7, false);
	glacierGen.dropParticles(0, 0, mapData.width, mapData.height, 1.0/64.0, threadCount, &mapGenModifyTilesProgressString, (void *)progressStringGlaciers);
	printf("\n");

	// Recalculate stats such as min/max height required for future calls.
	const char *progressStringGlobalStats2="Collecting global statistics (2/3) ";
	MapGen::recalculateStats(mapData.map, 0, 0, mapData.width, mapData.height, &mapGenModifyTilesProgressString, (void *)progressStringGlobalStats2);
	printf("\n");

	printf("	Min height %f, max height %f\n", mapData.map->minHeight, mapData.map->maxHeight);
	printf("	Min temperature %f, max temperature %f\n", mapData.map->minTemperature, mapData.map->maxTemperature);
	printf("	Min moisture %f, max moisture %f\n", mapData.map->minMoisture, mapData.map->maxMoisture);
	*/

	// Calculate sea level.
	printf("Searching for sea level (with desired land coverage %.2f%%) (2/3)...\n", desiredLandFraction*100.0);
	mapData.map->seaLevel=MapGen::narySearch(mapData.map, 0, 0, mapData.width, mapData.height, threadCount, 63, desiredLandFraction, 0.45, mapData.map->minHeight, mapData.map->maxHeight, &mapGenNarySearchGetFunctorHeight, NULL);
	printf("	Sea level %f\n", mapData.map->seaLevel);

	// Run moisture/river calculation.
	const char *progressStringRivers="Generating moisture/river data ";
	MapGen::ParticleFlow riverGen(mapData.map, 2, true);
	riverGen.dropParticles(0, 0, mapData.width, mapData.height, 1.0/16.0, 1/*.....threadCount*/, &mapGenModifyTilesProgressString, (void *)progressStringRivers);
	printf("\n");

	// Recalculate stats such as min/max height required for future calls.
	const char *progressStringGlobalStats3="Collecting global statistics (3/3) ";
	MapGen::recalculateStats(mapData.map, 0, 0, mapData.width, mapData.height, threadCount, &mapGenModifyTilesProgressString, (void *)progressStringGlobalStats3);
	printf("\n");

	printf("	Min height %f, max height %f\n", mapData.map->minHeight, mapData.map->maxHeight);
	printf("	Min temperature %f, max temperature %f\n", mapData.map->minTemperature, mapData.map->maxTemperature);
	printf("	Min moisture %f, max moisture %f\n", mapData.map->minMoisture, mapData.map->maxMoisture);

	// Calculate sea level.
	printf("Searching for sea level (with desired land coverage %.2f%%) (3/3)...\n", desiredLandFraction*100.0);
	mapData.map->seaLevel=MapGen::narySearch(mapData.map, 0, 0, mapData.width, mapData.height, threadCount, 63, desiredLandFraction, 0.45, mapData.map->minHeight, mapData.map->maxHeight, &mapGenNarySearchGetFunctorHeight, NULL);
	printf("	Sea level %f\n", mapData.map->seaLevel);

	// Calculate alpine level.
	printf("Searching for alpine level (with desired coverage %.2f%%)...\n", desiredAlpineFraction*100.0);
	mapData.map->alpineLevel=MapGen::narySearch(mapData.map, 0, 0, mapData.width, mapData.height, threadCount, 63, desiredAlpineFraction, 0.45, mapData.map->minHeight, mapData.map->maxHeight, &mapGenNarySearchGetFunctorHeight, NULL);
	printf("	Alpine level %f\n", mapData.map->alpineLevel);

	// Calculate cold threshold.
	double desiredColdCoverage=0.4;
	printf("Searching for cold threshold (with desired coverage %.2f%%)...\n", desiredColdCoverage*100.0);
	mapData.coldThreshold=MapGen::narySearch(mapData.map, 0, 0, mapData.width, mapData.height, threadCount, 63, desiredColdCoverage, 0.45, mapData.map->minTemperature, mapData.map->maxTemperature, &mapGenNarySearchGetFunctorTemperature, NULL);
	printf("	Cold temperature %f\n", mapData.coldThreshold);

	// Calculate hot threshold level.
	double desiredHotCoverage=0.2;
	printf("Searching for hot threshold level (with desired coverage %.2f%%)...\n", desiredHotCoverage*100.0);
	mapData.hotThreshold=MapGen::narySearch(mapData.map, 0, 0, mapData.width, mapData.height, threadCount, 63, desiredHotCoverage, 0.45, mapData.map->minTemperature, mapData.map->maxTemperature, &mapGenNarySearchGetFunctorTemperature, NULL);
	printf("	Hot temperature %f\n", mapData.hotThreshold);

	// Calculate river moisture threshold.
	double desiredRiverCoverage=0.005;
	printf("Searching for river moisture threshold (with desired coverage %.2f%%)...\n", desiredRiverCoverage*100.0);
	mapData.riverMoistureThreshold=MapGen::narySearch(mapData.map, 0, 0, mapData.width, mapData.height, threadCount, 63, desiredRiverCoverage, 0.45, mapData.map->minMoisture, mapData.map->maxMoisture, &mapGenNarySearchGetFunctorMoisture, NULL);
	printf("	River moisture threshold %f\n", mapData.riverMoistureThreshold);

	// Calculate forest threshold.
	mapData.forestNoise=new FbnNoise(seed+23, 8, 8.0);

	printf("Searching for forest level (with desired land coverage %.2f%%)...\n", desiredForestFraction*100.0);
	mapData.map->forestLevel=MapGen::narySearch(mapData.map, 0, 0, mapData.width, mapData.height, threadCount, 63, desiredForestFraction, 0.005, -1.0, 1.0, &mapGenNarySearchGetFunctorNoise, mapData.forestNoise);
	printf("	Forest level %f\n", mapData.map->forestLevel);

	// Run modify tiles for forests.
	size_t biomesModifyTilesArrayCount=3;
	MapGen::ModifyTilesManyEntry biomesModifyTilesArray[biomesModifyTilesArrayCount];
	biomesModifyTilesArray[0].functor=&demogenGroundModifyTilesFunctor;
	biomesModifyTilesArray[0].userData=&mapData;
	biomesModifyTilesArray[1].functor=&demogenGrassForestModifyTilesFunctor;
	biomesModifyTilesArray[1].userData=&mapData;
	biomesModifyTilesArray[2].functor=&demogenSandForestModifyTilesFunctor;
	biomesModifyTilesArray[2].userData=&mapData;

	const char *progressStringBiomes="Assigning tile textures for biomes ";
	MapGen::modifyTilesMany(mapData.map, 0, 0, mapData.width, mapData.height, threadCount, biomesModifyTilesArrayCount, biomesModifyTilesArray, &mapGenModifyTilesProgressString, (void *)progressStringBiomes);
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
	MapGen::modifyTilesMany(mapData.map, 0, 0, mapData.width, mapData.height, 1, npcModifyTilesArrayCount, npcModifyTilesArray, &mapGenModifyTilesProgressString, (void *)progressStringNpcsAnimals);
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
