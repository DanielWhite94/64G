#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "mapgen.h"
#include "../physics/coord.h"
#include "../fbnnoise.h"

using namespace Engine;

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
					MapTile tile(*heightArrayPtr>=0.0 ? 1 : 11); // TODO: Do not hardcode texture ids.
					CoordVec vec(x*Physics::CoordsPerTile, y*Physics::CoordsPerTile);
					map->setTileAtCoordVec(vec, tile);
				}

				// Update progress (if needed).
				if (y%yProgressDelta==yProgressDelta-1)
					printf("MapGen: creating map %f%%.\n", ((y+1)*100.0)/height); // TODO: this better
			}

			// Add objects.
			MapObject *npc1=addBuiltinObject(map, BuiltinObject::OldBeardMan, CoordAngle0, CoordVec(200*Physics::CoordsPerTile, 523*Physics::CoordsPerTile));
			npc1->setMovementModeConstantVelocity(CoordVec(2,1)); // east south east

			addBuiltinObjectForest(map, BuiltinObject::Tree1, CoordVec(200*Physics::CoordsPerTile, 535*Physics::CoordsPerTile), CoordVec(80*Physics::CoordsPerTile, 23*Physics::CoordsPerTile), CoordVec(3*Physics::CoordsPerTile/2, 3*Physics::CoordsPerTile/2));

			// Tidy up.
			free(heightArray);

			return map;
		}

		MapObject *MapGen::addBuiltinObject(class Map *map, BuiltinObject builtin, CoordAngle rotation, const CoordVec &pos) {
			switch(builtin) {
				case BuiltinObject::OldBeardMan: {
					// Create hitmask.
					const unsigned hitmaskW=4, hitmaskH=5;
					HitMask hitmask;
					unsigned x, y;
					for(y=(8-hitmaskH)/2; y<(8+hitmaskH)/2; ++y)
						for(x=(8-hitmaskW)/2; x<(8+hitmaskW)/2; ++x)
							hitmask.setXY(x, y, true);

					// Create object.
					MapObject *object=new MapObject(rotation, pos, 1, 1);
					object->setHitMaskByTileOffset(0, 0, hitmask);

					// Add object to map.
					map->addObject(object);

					return object;
				} break;
				case BuiltinObject::Tree1: {
					// Create hitmask.
					const char *hitmaskStr=
						"_____##_"
						"_##_####"
						"########"
						"########"
						"######__"
						"___##___"
						"_######_"
						"_######_";
					HitMask hitmask(hitmaskStr);

					// Create object.
					MapObject *object=new MapObject(rotation, pos, 1, 1);
					object->setHitMaskByTileOffset(0, 0, hitmask);

					// Add object to map.
					map->addObject(object);

					return object;
				} break;
			}

			assert(false);
			return NULL;
		}

		void MapGen::addBuiltinObjectForest(class Map *map, BuiltinObject builtin, const CoordVec &topLeft, const CoordVec &widthHeight, const CoordVec &interval) {
			assert(map!=NULL);
			assert(widthHeight.x>=0 && widthHeight.y>=0);
			assert(interval.x>0 && interval.y>0);

			CoordVec pos;
			for(pos.y=topLeft.y; pos.y<topLeft.y+widthHeight.y; pos.y+=interval.y)
				for(pos.x=topLeft.x; pos.x<topLeft.x+widthHeight.x; pos.x+=interval.x) {
					CoordVec randomOffset=CoordVec(rand()%interval.x, rand()%interval.y); // TODO: ensure they do not overlap - i.e. take into account size of builtin object.
					addBuiltinObject(map, builtin, CoordAngle0, pos+randomOffset);
				}
		}
	};
};
