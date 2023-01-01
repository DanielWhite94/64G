#ifndef ENGINE_MAP_MAPGEN_H
#define ENGINE_MAP_MAPGEN_H

#include <cstring>
#include <queue>
#include <iomanip>

#include "map.h"
#include "maptexture.h"
#include "maptile.h"
#include "../gen/modifytiles.h"
#include "../util.h"

namespace Engine {
	namespace Map {

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
			static const unsigned TileBitsetIndexContour=0;
			static const unsigned TileBitsetIndexLandmassBorder=1;

			typedef bool (AddForestFunctor)(class Map *map, const CoordVec &position, void *userData);

			typedef bool (TileTestFunctor)(class Map *map, int x, int y, int w, int h, void *userData);

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

			struct AddTownParameters {
				unsigned roadTileLayer;
				unsigned houseDecorationLayer;

				TileTestFunctor *testFunctor;
				void *testFunctorUserData;

				MapTexture::Id textureIdMajorPath;
				MapTexture::Id textureIdMinorPath;
				MapTexture::Id textureIdShopSignNone;
				MapTexture::Id textureIdShopSignCobbler;
			};

			// used internally
			struct AddTownHouseData {
				int x, y;

				int mapW, mapH;

				int genWidth, genDepth;

				bool isHorizontal; // derived from the road the house is conencted to
				bool side; // 0 = left/top, 1=/right/down
			};

			MapGen();
			~MapGen();

			// Call a functor for a set of tiles in a region representing a random forest with fixed density.
			static void addForest(class Map *map, const CoordVec &topLeft, const CoordVec &widthHeight, const CoordVec &interval, AddForestFunctor *functor, void *functorUserData);

			static bool addHouse(class Map *map, unsigned baseX, unsigned baseY, unsigned totalW, unsigned totalH, const AddHouseParameters *params);

			// Note: in houseParams the following fields will be set internally by addTown: roofHeight, doorOffset, chimneyOffset and the ShowDoor flag will be set as needed
			static bool addTown(class Map *map, unsigned x0, unsigned y0, unsigned x1, unsigned y1, const AddTownParameters *params, const AddHouseParameters *houseParams, int townPop);
			static bool addTowns(class Map *map, unsigned x0, unsigned y0, unsigned x1, unsigned y1, const AddTownParameters *params, const AddHouseParameters *houseParams, double totalPopulation);

			static void recalculateStats(class Map *map, unsigned x, unsigned y, unsigned width, unsigned height, unsigned threadCount, Gen::ModifyTilesProgress *progressFunctor, void *progressUserData); // Updates map's min/maxHeight and other such fields.

		private:
		};
	};
};

#endif
