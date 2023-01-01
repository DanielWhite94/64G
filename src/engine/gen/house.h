#ifndef ENGINE_GEN_HOUSE_H
#define ENGINE_GEN_HOUSE_H

#include "../map/map.h"

namespace Engine {
	namespace Gen {
		typedef bool (TileTestFunctor)(class Map *map, int x, int y, int w, int h, void *userData); // should return true if house could be placed in specified area

		enum AddHouseFlags {
			None=0x0,
			ShowDoor=0x1,
			ShowChimney=0x2,
			AddDecoration=0x4,
			All=(ShowDoor|ShowChimney|AddDecoration),
		};

		struct AddHouseParameters {
			AddHouseFlags flags;

			unsigned tileLayer;
			unsigned decorationLayer;

			TileTestFunctor *testFunctor;
			void *testFunctorUserData;

			MapTexture::Id textureIdWall0;
			MapTexture::Id textureIdWall1;
			MapTexture::Id textureIdWall2;
			MapTexture::Id textureIdHouseDoorBL;
			MapTexture::Id textureIdHouseDoorBR;
			MapTexture::Id textureIdHouseDoorTL;
			MapTexture::Id textureIdHouseDoorTR;
			MapTexture::Id textureIdHouseRoof;
			MapTexture::Id textureIdHouseRoofTop;
			MapTexture::Id textureIdHouseChimneyTop;
			MapTexture::Id textureIdHouseChimney;
			MapTexture::Id textureIdBrickPath;
			MapTexture::Id textureIdRoseBush;

			unsigned roofHeight;
			unsigned doorXOffset;
			unsigned chimneyXOffset;
		};

		bool addHouse(class Map *map, unsigned baseX, unsigned baseY, unsigned totalW, unsigned totalH, const AddHouseParameters *params);
	};
};

#endif
