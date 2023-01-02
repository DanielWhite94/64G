#ifndef ENGINE_GEN_EDGEDETECT_H
#define ENGINE_GEN_EDGEDETECT_H

#include <cstring>

#include "../map/map.h"

namespace Engine {
	namespace Gen {
		bool edgeDetectHeightThresholdSampleFunctor(class Map *map, unsigned x, unsigned y, void *userData); // Returns true for tiles which exceed height threshold passed in via a pointer to a double in userData.
		bool edgeDetectLandSampleFunctor(class Map *map, unsigned x, unsigned y, void *userData); // Returns true for land tiles (those whose height exceeds sea level).
		void edgeDetectBitsetNEdgeFunctor(class Map *map, unsigned x, unsigned y, void *userData); // Sets a bit true in the bitset associated with each boundary tile. The bit set is determined by the userData argument (cast to unsigned via uintptr_t).
		void edgeDetectBitsetFullEdgeFunctor(class Map *map, unsigned x, unsigned y, void *userData); // Similar to edgeDetectBitsetNEdgeFunctor except the userData is interpreted as a full 64 bit bitset to OR into to each tile's existing bitset.

		class EdgeDetect {
			public:
				// This class provides algorithms to trace an edge around a group of tiles,
				// based on a boolean function applied to each tile to determine if it
				// should be 'inside' or 'outside' the edge.
				// Note that when tracing we 'wrap around' the edges of the map as if it were a torus.

				static const unsigned DirectionEast=0;
				static const unsigned DirectionNorth=1;
				static const unsigned DirectionWest=2;
				static const unsigned DirectionSouth=3;
				static const unsigned DirectionNB=4;

				// This should return true if tile is considered to be 'inside', and false for 'outside'
				// (e.g. if looking for continents then land tiles should return true and ocean tiles false).
				// Out of bounds tiles should be considered outside and return false.
				typedef bool (SampleFunctor)(class Map *map, unsigned x, unsigned y, void *userData);

				// This is called for each tile which is determined to be part of the edge ('inside' tiles only).
				typedef void (EdgeFunctor)(class Map *map, unsigned x, unsigned y, void *userData);

				typedef void (ProgressFunctor)(double progress, Util::TimeMs elapsedTimeMs, void *userData);

				// Used internally by traceFast
				struct TraceFastModifyTilesData {
					SampleFunctor *sampleFunctor;
					void *sampleUserData;

					EdgeFunctor *edgeFunctor;
					void *edgeUserData;
				};

				EdgeDetect(class Map *map): map(map) {
				};
				~EdgeDetect() {};


				// The following is an implementation of the Square Tracing method. It should produce nice crisp edges but is slow.
				// Each scratchBits array entry should contain a unique tile bitset index which can be used freely by the trace algorithm internally.
				// (to store which directions we have entered each tile from previously to avoid retracing)
				void traceAccurate(unsigned scratchBits[DirectionNB], SampleFunctor *sampleFunctor, void *sampleUserData, EdgeFunctor *edgeFunctor, void *edgeUserData, ProgressFunctor *progressFunctor, void *progressUserData);

				// The follow is a custom algorithm designed for speed at the expense of edge quality.
				void traceFast(unsigned threadCount, SampleFunctor *sampleFunctor, void *sampleUserData, EdgeFunctor *edgeFunctor, void *edgeUserData, ProgressFunctor *progressFunctor, void *progressUserData);

				// Trace a series of contours based on given number of height thresholds.
				// For each tile which is determined to be part of a contour we set bit TileBitsetIndexContour in the tile's bitset.
				void traceAccurateHeightContours(unsigned scratchBits[DirectionNB], int contourCount, ProgressFunctor *progressFunctor, void *progressUserData);
				void traceFastHeightContours(unsigned threadCount, int contourCount, ProgressFunctor *progressFunctor, void *progressUserData);
			private:
				class Map *map;

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

	};
};

#endif
