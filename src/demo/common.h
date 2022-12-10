#ifndef COMMON_H
#define COMMON_H

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

static const MapItem::Id ItemIdCoins=0;
static const MapItem::Id ItemIdNB=1;

enum DemoGenTileLayer {
	DemoGenTileLayerGround,
	DemoGenTileLayerDecoration,
	DemoGenTileLayerHalf,
	DemoGenTileLayerFull,
};

#endif
