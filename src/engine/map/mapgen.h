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
		void mapGenNArySearchModifyTilesFunctor(unsigned threadId, class Map *map, unsigned x, unsigned y, void *userData);
		double mapGenNarySearchGetFunctorHeight(class Map *map, unsigned x, unsigned y, void *userData);
		double mapGenNarySearchGetFunctorTemperature(class Map *map, unsigned x, unsigned y, void *userData);
		double mapGenNarySearchGetFunctorMoisture(class Map *map, unsigned x, unsigned y, void *userData);
		double mapGenNarySearchGetFunctorNoise(class Map *map, unsigned x, unsigned y, void *userData); // userData should be a pointer to an FbnNoise instance

		bool mapGenEdgeDetectHeightThresholdSampleFunctor(class Map *map, unsigned x, unsigned y, void *userData); // Returns true for tiles which exceed height threshold passed in via a pointer to a double in userData.
		bool mapGenEdgeDetectLandSampleFunctor(class Map *map, unsigned x, unsigned y, void *userData); // Returns true for land tiles (those whose height exceeds sea level).
		void mapGenEdgeDetectBitsetNEdgeFunctor(class Map *map, unsigned x, unsigned y, void *userData); // Sets a bit true in the bitset associated with each boundary tile. The bit set is determined by the userData argument (cast to unsigned via uintptr_t).
		void mapGenEdgeDetectBitsetFullEdgeFunctor(class Map *map, unsigned x, unsigned y, void *userData); // Similar to mapGenEdgeDetectBitsetNEdgeFunctor except the userData is interpreted as a full 64 bit bitset to OR into to each tile's existing bitset.
		void mapGenEdgeDetectStringProgressFunctor(class Map *map, double progress, Util::TimeMs elapsedTimeMs, void *userData);

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

			typedef double (NArySearchGetFunctor)(class Map *map, unsigned x, unsigned y, void *userData);

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

			struct NArySearchData {
				class Map *map;

				NArySearchGetFunctor *getFunctor;
				void *getUserData;

				double sampleMin, sampleMax, sampleRange;
				double sampleConversionFactor; // equal to (data->sampleCount+1.0)/data->sampleRange, see MapGen::narySearchValueToSample
				int sampleCount;
				unsigned long long int *sampleTally, sampleTotal;
			};

			class ParticleFlow {
			public:
				ParticleFlow(Map *map, int erodeRadius, bool incMoisture): map(map), erodeRadius(erodeRadius), incMoisture(incMoisture) {
					double skew=0.8;
					seaLevelExcess=map->minHeight+(map->seaLevel-map->minHeight)*skew;
				}; // Requires the map have seaLevel set.
				~ParticleFlow() {};

				void dropParticles(unsigned x0, unsigned y0, unsigned x1, unsigned y1, double coverage, unsigned threadCount, Gen::ModifyTilesProgress *progressFunctor, void *progressUserData); // Calls dropParticle on random coordinates within the given area, where dropParticle is ran floor(coverage*(x1-x0)*(y1-y0)) times.
				void dropParticle(double x, double y);

			private:
				Map *map;
				int erodeRadius;
				bool incMoisture;
				double seaLevelExcess;

				double hMap(int x, int y, double unknownValue); // Returns height of tile at (x,y), returning unknownValue if tile is out of bounds or could not be loaded.
				void depositAt(int x, int y, double w, double ds); // Adjusts the height of a tile at the given (x,y). Does nothing if the tile is out of bounds or could not be loaded.
			};

			class EdgeDetect {
			public:
				// This class provides algorithms to trace an edge around a group of tiles,
				// based on a boolean function applied to each tile to determine if it
				// should be 'inside' or 'outside' the edge.

				static const unsigned DirectionEast=0;
				static const unsigned DirectionNorth=1;
				static const unsigned DirectionWest=2;
				static const unsigned DirectionSouth=3;
				static const unsigned DirectionNB=4;

				// This should return true if tile is considered to be 'inside', and false for 'outside'
				// (e.g. if looking for continents then land tiles should return true and ocean tiles false).
				typedef bool (SampleFunctor)(class Map *map, unsigned x, unsigned y, void *userData);

				// This is called for each tile which is determined to be part of the edge ('inside' tiles only).
				typedef void (EdgeFunctor)(class Map *map, unsigned x, unsigned y, void *userData);

				typedef void (ProgressFunctor)(class Map *map, double progress, Util::TimeMs elapsedTimeMs, void *userData);

				// Each scratchBits array entry should contain a unique tile bitset index which can be used freely by the trace algorithm internally.
				EdgeDetect(Map *map, unsigned gScratchBits[4]): map(map) {
					memcpy(scratchBits, gScratchBits, sizeof(unsigned)*4);
				};
				~EdgeDetect() {};

				void trace(SampleFunctor *sampleFunctor, void *sampleUserData, EdgeFunctor *edgeFunctor, void *edgeUserData, ProgressFunctor *progressFunctor, void *progressUserData);

				void traceHeightContours(int contourCount, ProgressFunctor *progressFunctor, void *progressUserData); // Uses tile height and bitset fields, setting bit TileBitsetIndexContour for each tile which is part of a height contour
			private:
				Map *map;

				unsigned scratchBits[DirectionNB]; // bitset indexes that can be used freely during tracing to store which directions we have entered each tile from previously (to avoid retracing)

				void turnLeft(int *dx, int *dy, unsigned *dir) {
					int temp=*dx;
					*dx=*dy;
					*dy=-temp;
					*dir=(*dir+1)%DirectionNB;
				}

				void turnRight(int *dx, int *dy, unsigned *dir) {
					int temp=*dx;
					*dx=-*dy;
					*dy=temp;
					*dir=(*dir+DirectionNB-1)%DirectionNB;
				}

				void moveForward(int *x, int *y, int dx, int dy) {
					assert(x!=NULL);
					assert(*x>=0 && *x<map->getWidth());
					assert(y!=NULL);
					assert(*y>=0 && *y<map->getHeight());
					assert(dx==-1 || dx==0 || dx==1);
					assert(dy==-1 || dy==0 || dy==1);

					*x=(*x+map->getWidth()+dx)%map->getWidth();
					*y=(*y+map->getHeight()+dy)%map->getHeight();
				}
			};

			MapGen();
			~MapGen();

			// Call a functor for a set of tiles in a region representing a random forest with fixed density.
			static void addForest(class Map *map, const CoordVec &topLeft, const CoordVec &widthHeight, const CoordVec &interval, AddForestFunctor *functor, void *functorUserData);

			static bool addHouse(class Map *map, unsigned baseX, unsigned baseY, unsigned totalW, unsigned totalH, const AddHouseParameters *params);

			// Note: in houseParams the following fields will be set internally by addTown: roofHeight, doorOffset, chimneyOffset and the ShowDoor flag will be set as needed
			static bool addTown(class Map *map, unsigned x0, unsigned y0, unsigned x1, unsigned y1, const AddTownParameters *params, const AddHouseParameters *houseParams, int townPop);
			static bool addTowns(class Map *map, unsigned x0, unsigned y0, unsigned x1, unsigned y1, const AddTownParameters *params, const AddHouseParameters *houseParams, double totalPopulation);

			static double narySearch(class Map *map, unsigned x, unsigned y, unsigned width, unsigned height, unsigned threadCount, int n, double threshold, double epsilon, double sampleMin, double sampleMax, NArySearchGetFunctor *getFunctor, void *getUserData); // Returns (approximate) height/moisture etc value for which threshold fraction of the tiles in the given region are lower than said value.
			static int narySearchValueToSample(const NArySearchData *data, double value);
			static double narySearchSampleToValue(const NArySearchData *data, int sample);

			static void recalculateStats(class Map *map, unsigned x, unsigned y, unsigned width, unsigned height, unsigned threadCount, Gen::ModifyTilesProgress *progressFunctor, void *progressUserData); // Updates map's min/maxHeight and other such fields.

		private:
		};
	};
};

#endif
