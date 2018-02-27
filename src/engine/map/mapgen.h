#ifndef ENGINE_MAP_MAPGEN_H
#define ENGINE_MAP_MAPGEN_H

#include <queue>
#include <iomanip>

#include "map.h"
#include "../noisearray.h"

namespace Engine {
	namespace Map {
		void mapGenModifyTilesProgressString(class Map *map, unsigned y, unsigned height, void *userData);
		void mapGenGenerateBinaryNoiseModifyTilesFunctor(class Map *map, unsigned x, unsigned y, void *userData);

		struct MapGenRoad {
			int x0, y0, x1, y1;
			int trueX1, trueY1;
			int width;
			double weight;

			int getLen(void) const {
				return (x1-x0)+(y1-y0);
			};

			bool isHorizontal(void) const {
				return (y0==y1);
			}

			bool isVertical(void) const {
				return (x0==x1);
			}
		};

		class CompareMapGenRoadWeight {
		public:
			bool operator()(MapGenRoad &x, MapGenRoad &y) {
				return (x.weight<y.weight);
			}
		};

		class MapGen {
		public:
			static const MapTexture::Id TextureIdNone=0;
			static const MapTexture::Id TextureIdGrass0=1;
			static const MapTexture::Id TextureIdGrass1=2;
			static const MapTexture::Id TextureIdGrass2=3;
			static const MapTexture::Id TextureIdGrass3=4;
			static const MapTexture::Id TextureIdGrass4=5;
			static const MapTexture::Id TextureIdGrass5=6;
			static const MapTexture::Id TextureIdBrickPath=7;
			static const MapTexture::Id TextureIdDirt=8;
			static const MapTexture::Id TextureIdDock=9;
			static const MapTexture::Id TextureIdWater=10;
			static const MapTexture::Id TextureIdTree1=11;
			static const MapTexture::Id TextureIdTree2=12;
			static const MapTexture::Id TextureIdTree3=13;
			static const MapTexture::Id TextureIdMan1=14;
			static const MapTexture::Id TextureIdOldManN=15;
			static const MapTexture::Id TextureIdOldManE=16;
			static const MapTexture::Id TextureIdOldManS=17;
			static const MapTexture::Id TextureIdOldManW=18;
			static const MapTexture::Id TextureIdHouseDoorBL=19;
			static const MapTexture::Id TextureIdHouseDoorBR=20;
			static const MapTexture::Id TextureIdHouseDoorTL=21;
			static const MapTexture::Id TextureIdHouseDoorTR=22;
			static const MapTexture::Id TextureIdHouseRoof=23;
			static const MapTexture::Id TextureIdHouseRoofTop=24;
			static const MapTexture::Id TextureIdHouseWall2=25;
			static const MapTexture::Id TextureIdHouseWall3=26;
			static const MapTexture::Id TextureIdHouseWall4=27;
			static const MapTexture::Id TextureIdHouseChimney=28;
			static const MapTexture::Id TextureIdHouseChimneyTop=29;
			static const MapTexture::Id TextureIdSand=30;
			static const MapTexture::Id TextureIdHotSand=31;
			static const MapTexture::Id TextureIdShopCobbler=32;
			static const MapTexture::Id TextureIdSnow=33;
			static const MapTexture::Id TextureIdDeepWater=34;
			static const MapTexture::Id TextureIdHighAlpine=35;
			static const MapTexture::Id TextureIdLowAlpine=36;
			static const MapTexture::Id TextureIdHeatMapMin=37;
			static const MapTexture::Id TextureIdHeatMapRange=256;
			static const MapTexture::Id TextureIdHeatMapMax=TextureIdHeatMapMin+TextureIdHeatMapRange;
			static const MapTexture::Id TextureIdNB=TextureIdHeatMapMax;

			enum class BuiltinObject {
				OldBeardMan,
				Tree1,
				Tree2,
				Bush,
			};

			struct GenerateBinaryNoiseModifyTilesData {
				const NoiseArray *noiseArray;

				double threshold;
				MapTexture::Id lowTextureId, highTextureId;

				unsigned tileLayer;
			};

			typedef bool (ObjectTestFunctor)(class Map *map, BuiltinObject builtin, const CoordVec &position, void *userData);

			typedef bool (TileTestFunctor)(class Map *map, int x, int y, int w, int h, void *userData);

			typedef void (ModifyTilesFunctor)(class Map *map, unsigned x, unsigned y, void *userData);
			typedef void (ModifyTilesProgress)(class Map *map, unsigned regionY, unsigned regionHeight, void *userData);

			struct ModifyTilesManyEntry {
				ModifyTilesFunctor *functor;
				void *userData;
			};

			enum AddTownsShopType {
				Shoemaker,
				Furrier,
				Tailor,
				Barber,
				Jeweler,
				Tavern,
				Carpenters,
				Bakers,
				NB,
			};

			enum AddHouseFullFlags {
				None=0x0,
				ShowDoor=0x1,
				ShowChimney=0x2,
				All=(ShowDoor|ShowChimney),
			};

			struct AddTownHouseData {
				int x, y;

				int mapW, mapH;

				int genWidth, genDepth;

				bool isHorizontal; // derived from the road the house is conencted to
				bool side; // 0 = left/top, 1=/right/down
				AddHouseFullFlags flags;

				unsigned roofHeight;
				unsigned doorOffset; // Only valid if flags has ShowDoor
				unsigned chimneyOffset;
			};

			MapGen(unsigned width, unsigned height);
			~MapGen();

			static bool addBaseTextures(class Map *map);

			static MapObject *addBuiltinObject(class Map *map, BuiltinObject builtin, CoordAngle rotation, const CoordVec &pos);
			static void addBuiltinObjectForest(class Map *map, BuiltinObject builtin, const CoordVec &topLeft, const CoordVec &widthHeight, const CoordVec &interval);
			static void addBuiltinObjectForestWithTestFunctor(class Map *map, BuiltinObject builtin, const CoordVec &topLeft, const CoordVec &widthHeight, const CoordVec &interval, ObjectTestFunctor *testFunctor, void *testFunctorUserData);

			static bool addHouse(class Map *map, unsigned baseX, unsigned baseY, unsigned totalW, unsigned totalH, unsigned tileLayer, bool showDoor, TileTestFunctor *testFunctor, void *testFunctorUserData);
			static bool addHouseFull(class Map *map, AddHouseFullFlags flags, unsigned baseX, unsigned baseY, unsigned totalW, unsigned totalH, unsigned roofHeight, unsigned tileLayer, unsigned doorXOffset, unsigned chimneyXOffset, TileTestFunctor *testFunctor, void *testFunctorUserData);

			static bool addTown(class Map *map, unsigned x0, unsigned y0, unsigned x1, unsigned y1, unsigned roadTileLayer, unsigned houseTileLayer, int townPop, TileTestFunctor *testFunctor, void *testFunctorUserData);
			static bool addTowns(class Map *map, unsigned x0, unsigned y0, unsigned x1, unsigned y1, unsigned roadTileLayer, unsigned houseTileLayer, double totalPopulation,  TileTestFunctor *testFunctor, void *testFunctorUserData);

			static void modifyTiles(class Map *map, unsigned x, unsigned y, unsigned width, unsigned height, ModifyTilesFunctor *functor, void *functorUserData, ModifyTilesProgress *progressFunctor, void *progressUserData);
			static void modifyTilesMany(class Map *map, unsigned x, unsigned y, unsigned width, unsigned height, size_t functorArrayCount, MapGen::ModifyTilesManyEntry functorArray[], ModifyTilesProgress *progressFunctor, void *progressUserData);
		private:
			unsigned width, height;
		};
	};
};

#endif
