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
		void edgeDetectStringProgressFunctor(class Map *map, double progress, Util::TimeMs elapsedTimeMs, void *userData);

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
				EdgeDetect(class Map *map, unsigned gScratchBits[4]): map(map) {
					memcpy(scratchBits, gScratchBits, sizeof(unsigned)*4);
				};
				~EdgeDetect() {};

				void trace(SampleFunctor *sampleFunctor, void *sampleUserData, EdgeFunctor *edgeFunctor, void *edgeUserData, ProgressFunctor *progressFunctor, void *progressUserData);

				void traceHeightContours(int contourCount, ProgressFunctor *progressFunctor, void *progressUserData); // Uses tile height and bitset fields, setting bit TileBitsetIndexContour for each tile which is part of a height contour
			private:
				class Map *map;

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

	};
};

#endif
