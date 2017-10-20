#ifndef ENGINE_MAP_MAPGEN_H
#define ENGINE_MAP_MAPGEN_H

#include <queue>
#include <iomanip>

#include "map.h"

namespace Engine {
	namespace Map {
		void mapGenModifyTilesProgressString(class Map *map, unsigned y, unsigned height, void *userData);
		void mapGenGenerateBinaryNoiseModifyTilesFunctor(class Map *map, unsigned x, unsigned y, void *userData);

		struct MapGenRoad {
			unsigned x0, y0, x1, y1;
			unsigned trueX1, trueY1;
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
			static const unsigned TextureIdNone=0;
			static const unsigned TextureIdGrass0=1;
			static const unsigned TextureIdGrass1=2;
			static const unsigned TextureIdGrass2=3;
			static const unsigned TextureIdGrass3=4;
			static const unsigned TextureIdGrass4=5;
			static const unsigned TextureIdGrass5=6;
			static const unsigned TextureIdBrickPath=7;
			static const unsigned TextureIdDirt=8;
			static const unsigned TextureIdDock=9;
			static const unsigned TextureIdWater=10;
			static const unsigned TextureIdTree1=11;
			static const unsigned TextureIdTree2=12;
			static const unsigned TextureIdMan1=13;
			static const unsigned TextureIdOldManN=14;
			static const unsigned TextureIdOldManE=15;
			static const unsigned TextureIdOldManS=16;
			static const unsigned TextureIdOldManW=17;
			static const unsigned TextureIdHouseDoorBL=18;
			static const unsigned TextureIdHouseDoorBR=19;
			static const unsigned TextureIdHouseDoorTL=20;
			static const unsigned TextureIdHouseDoorTR=21;
			static const unsigned TextureIdHouseRoof=22;
			static const unsigned TextureIdHouseRoofTop=23;
			static const unsigned TextureIdHouseWall2=24;
			static const unsigned TextureIdHouseWall3=25;
			static const unsigned TextureIdHouseWall4=26;
			static const unsigned TextureIdHouseChimney=27;
			static const unsigned TextureIdHouseChimneyTop=28;
			static const unsigned TextureIdNB=29;

			enum class BuiltinObject {
				OldBeardMan,
				Tree1,
				Tree2,
				Bush,
			};

			struct GenerateBinaryNoiseModifyTilesData {
				const double *heightArray;
				double heightYFactor, heightXFactor;
				unsigned heightNoiseWidth;

				double threshold;
				unsigned lowTextureId, highTextureId;

				unsigned tileLayer;
			};

			typedef bool (ObjectTestFunctor)(class Map *map, BuiltinObject builtin, const CoordVec &position, void *userData);

			typedef bool (TileTestFunctor)(class Map *map, unsigned x, unsigned y, unsigned w, unsigned h, void *userData);

			typedef void (ModifyTilesFunctor)(class Map *map, unsigned x, unsigned y, void *userData);
			typedef void (ModifyTilesProgress)(class Map *map, unsigned y, unsigned height, void *userData);

			struct ModifyTilesManyEntry {
				ModifyTilesFunctor *functor;
				void *userData;
			} ;

			MapGen(unsigned width, unsigned height);
			~MapGen();

			static bool addBaseTextures(class Map *map);

			static MapObject *addBuiltinObject(class Map *map, BuiltinObject builtin, CoordAngle rotation, const CoordVec &pos);
			static void addBuiltinObjectForest(class Map *map, BuiltinObject builtin, const CoordVec &topLeft, const CoordVec &widthHeight, const CoordVec &interval);
			static void addBuiltinObjectForestWithTestFunctor(class Map *map, BuiltinObject builtin, const CoordVec &topLeft, const CoordVec &widthHeight, const CoordVec &interval, ObjectTestFunctor *testFunctor, void *testFunctorUserData);

			static bool addHouse(class Map *map, unsigned x, unsigned y, unsigned w, unsigned h, unsigned tileLayer, bool showDoor, TileTestFunctor *testFunctor, void *testFunctorUserData);

			static bool addTown(class Map *map, unsigned x0, unsigned y0, unsigned x1, unsigned y1, unsigned tileLayer, TileTestFunctor *testFunctor, void *testFunctorUserData);

			static void modifyTiles(class Map *map, unsigned x, unsigned y, unsigned width, unsigned height, ModifyTilesFunctor *functor, void *functorUserData, unsigned progressDelta, ModifyTilesProgress *progressFunctor, void *progressUserData);
			static void modifyTilesMany(class Map *map, unsigned x, unsigned y, unsigned width, unsigned height, size_t functorArrayCount, MapGen::ModifyTilesManyEntry *functorArray[], unsigned progressDelta, ModifyTilesProgress *progressFunctor, void *progressUserData);
		private:
			unsigned width, height;
		};
	};
};

#endif
