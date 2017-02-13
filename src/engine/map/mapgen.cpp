#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "mapgen.h"
#include "../physics/coord.h"
#include "../fbnnoise.h"

using namespace Engine;
using namespace Engine::Map;

namespace Engine {
	namespace Map {
		MapGen::MapGen(unsigned gWidth, unsigned gHeight) {
			width=gWidth;
			height=gHeight;
		}

		MapGen::~MapGen() {
		};

		class Map *MapGen::generate(void) {
			// Choose parameters.
			const double cellWidth=1.0;
			const double cellHeight=1.0;
			const double heightResolution=200.0;

			double *heightArray=(double *)malloc(sizeof(double)*height*width);
			assert(heightArray!=NULL); // TODO: better
			double *heightArrayPtr;

			unsigned yProgressDelta=height/16;

			unsigned x, y;

			// Calculate heightArray.
			FbnNoise heightNose(8, 1.0/heightResolution, 1.0, 2.0, 0.5);
			// TODO: Loop over in a more cache-friendly manner (i.e. do all of region 0, then all of region 1, etc).
			float freqFactorX=(cellWidth/width)*1024.0;
			float freqFactorY=(cellHeight/height)*1024.0;
			heightArrayPtr=heightArray;
			for(y=0;y<height;++y) {
				for(x=0;x<width;++x,++heightArrayPtr)
					// Calculate noise value to represent the height here.
					*heightArrayPtr=heightNose.eval(x*freqFactorX, y*freqFactorY);

				// Update progress (if needed).
				if (y%yProgressDelta==yProgressDelta-1)
					printf("MapGen: base generation %.1f%%.\n", ((y+1)*100.0)/height); // TODO: this better
			}

			// Create Map.
			class Map *map=new Map();
			heightArrayPtr=heightArray;
			for(y=0;y<height;++y) {
				for(x=0;x<width;++x,++heightArrayPtr) {
					MapTile tile(*heightArrayPtr>=0.0 ? ((rand()%2==0) ? 1 : (rand()%6+2)) : 11); // TODO: Do not hardcode texture ids.
					CoordVec vec(x*Physics::CoordsPerTile, y*Physics::CoordsPerTile);
					map->setTileAtCoordVec(vec, tile);
				}

				// Update progress (if needed).
				if (y%yProgressDelta==yProgressDelta-1)
					printf("MapGen: creating map %f%%.\n", ((y+1)*100.0)/height); // TODO: this better
			}

			// Tidy up.
			free(heightArray);

			return map;
		}
	};
};
