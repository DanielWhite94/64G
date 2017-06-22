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

		bool MapGen::addBaseTextures(class Map *map) {
			const char *texturePaths[TextureIdNB]={
				[TextureIdNone]=NULL, // Implies no tile.
				[TextureIdGrass0]="../images/tiles/grass0.png",
				[TextureIdGrass1]="../images/tiles/grass1.png",
				[TextureIdGrass2]="../images/tiles/grass2.png",
				[TextureIdGrass3]="../images/tiles/grass3.png",
				[TextureIdGrass4]="../images/tiles/grass4.png",
				[TextureIdGrass5]="../images/tiles/grass5.png",
				[TextureIdBrickPath]="../images/tiles/tile.png",
				[TextureIdDirt]="../images/tiles/dirt.png",
				[TextureIdDock]="../images/tiles/dock.png",
				[TextureIdWater]="../images/tiles/water.png",
				[TextureIdTree1]="../images/objects/tree1.png",
				[TextureIdTree2]="../images/objects/tree2.png",
				[TextureIdMan1]="../images/objects/man1.png",
				[TextureIdOldManN]="../images/npcs/oldbeardman/north.png",
				[TextureIdOldManE]="../images/npcs/oldbeardman/east.png",
				[TextureIdOldManS]="../images/npcs/oldbeardman/south.png",
				[TextureIdOldManW]="../images/npcs/oldbeardman/west.png",
				[TextureIdHouseDoorBL]="../images/tiles/house/doorbl.png",
				[TextureIdHouseDoorBR]="../images/tiles/house/doorbr.png",
				[TextureIdHouseDoorTL]="../images/tiles/house/doortl.png",
				[TextureIdHouseDoorTR]="../images/tiles/house/doortr.png",
				[TextureIdHouseRoof]="../images/tiles/house/roof.png",
				[TextureIdHouseRoofTop]="../images/tiles/house/rooftop.png",
				[TextureIdHouseWall2]="../images/tiles/house/wall2.png",
				[TextureIdHouseWall3]="../images/tiles/house/wall3.png",
				[TextureIdHouseWall4]="../images/tiles/house/wall4.png",
				[TextureIdHouseChimney]="../images/tiles/house/chimney.png",
				[TextureIdHouseChimneyTop]="../images/tiles/house/chimneytop.png",
			};
			const int textureScales[TextureIdNB]={
				[TextureIdNone]=1,
				[TextureIdGrass0]=4,
				[TextureIdGrass1]=4,
				[TextureIdGrass2]=4,
				[TextureIdGrass3]=4,
				[TextureIdGrass4]=4,
				[TextureIdGrass5]=4,
				[TextureIdBrickPath]=4,
				[TextureIdDirt]=4,
				[TextureIdDock]=4,
				[TextureIdWater]=4,
				[TextureIdTree1]=4,
				[TextureIdTree2]=4,
				[TextureIdMan1]=4,
				[TextureIdOldManN]=4,
				[TextureIdOldManE]=4,
				[TextureIdOldManS]=4,
				[TextureIdOldManW]=4,
				[TextureIdHouseDoorBL]=4,
				[TextureIdHouseDoorBR]=4,
				[TextureIdHouseDoorTL]=4,
				[TextureIdHouseDoorTR]=4,
				[TextureIdHouseRoof]=4,
				[TextureIdHouseRoofTop]=4,
				[TextureIdHouseWall2]=4,
				[TextureIdHouseWall3]=4,
				[TextureIdHouseWall4]=4,
				[TextureIdHouseChimney]=4,
				[TextureIdHouseChimneyTop]=4,
			};

			bool success=true;
			unsigned textureId;
			for(textureId=1; textureId<TextureIdNB; ++textureId)
				success&=map->addTexture(new MapTexture(textureId, texturePaths[textureId], textureScales[textureId]));

			return success;
		};

		bool MapGen::generateWaterLand(class Map *map, unsigned xOffset, unsigned yOffset, unsigned width, unsigned height, unsigned waterTextureId, unsigned landTextureId, unsigned tileLayer, const double targetLandFraction) {
			assert(map!=NULL);

			// Choose parameters.
			const unsigned heightNoiseWidth=1024;
			const unsigned heightNoiseHeight=1024;
			const double heightResolution=200.0;

			double *heightArray=(double *)malloc(sizeof(double)*heightNoiseHeight*heightNoiseWidth);
			assert(heightArray!=NULL); // TODO: better
			double *heightArrayPtr;


			unsigned x, y;

			// Calculate heightArray.
			FbnNoise heightNose(8, 1.0/heightResolution, 1.0, 2.0, 0.5);
			unsigned noiseYProgressDelta=heightNoiseHeight/16;
			// TODO: Loop over in a more cache-friendly manner (i.e. do all of region 0, then all of region 1, etc).
			float freqFactorX=(((double)width)/heightNoiseWidth)/8.0;
			float freqFactorY=(((double)height)/heightNoiseHeight)/8.0;
			heightArrayPtr=heightArray;
			for(y=0;y<heightNoiseHeight;++y) {
				for(x=0;x<heightNoiseWidth;++x,++heightArrayPtr)
					// Calculate noise value to represent the height here.
					*heightArrayPtr=heightNose.eval(x*freqFactorX, y*freqFactorY);

				// Update progress (if needed).
				if (y%noiseYProgressDelta==noiseYProgressDelta-1)
					printf("MapGen: generating height noise %.1f%%.\n", ((y+1)*100.0)/heightNoiseHeight); // TODO: this better
			}

			// Choose land height (using a binary search).
			double heightXFactor=((double)heightNoiseWidth)/width;
			double heightYFactor=((double)heightNoiseHeight)/height;

			printf("MapGen: choosing water/land threshold...\n");

			double minLandHeight=-1.0;
			double maxLandHeight=1.0;
			const unsigned iterMax=8;
			for(unsigned iter=0; iter<iterMax; ++iter) {
				// Guess the midpoint of our bounds.
				double guessLandHeight=(maxLandHeight+minLandHeight)/2.0;

				// Calculate how much land this guess would create.
				double guessLand=0.0;
				for(y=0;y<height;++y) {
					unsigned heightY=y*heightYFactor;
					for(x=0;x<width;++x) {
						unsigned heightX=x*heightXFactor;
						double height=heightArray[heightX+heightY*heightNoiseWidth];
						guessLand+=(height>=guessLandHeight);
					}
				}
				double guessLandFraction=guessLand/(((double)height)*((double)width));

				// Print progress update.
				printf("	MapGen: choosing water/land threshold %u/%u (min %.3f, max %.3f, guessLandHeight %0.3f, guessLandFraction %0.3f).\n", iter, iterMax, minLandHeight, maxLandHeight, guessLandHeight, guessLandFraction);

				// How good was our guess?
				const double errorEpsilon=0.005;
				double errorDelta=guessLandFraction-targetLandFraction;
				if (errorDelta>errorEpsilon)
					// Too much land - choose upper half interval.
					minLandHeight=guessLandHeight;
				else if (errorDelta<-errorEpsilon)
					// Not enough land - choose lower half interval.
					maxLandHeight=guessLandHeight;
				else
					// Near enough!
					break;
			}

			double landHeight=(maxLandHeight+minLandHeight)/2.0;

			// Create base tile layer - water/land.
			unsigned baseLayerYProgressDelta=height/16;
			printf("MapGen: creating water/land tiles... (land height %.2f)\n", landHeight);
			for(y=0;y<height;++y) {
				unsigned heightY=y*heightYFactor;
				for(x=0;x<width;++x) {
					unsigned heightX=x*heightXFactor;
					double height=heightArray[heightX+heightY*heightNoiseWidth];

					MapTile tile((height>=landHeight ? landTextureId : waterTextureId), tileLayer);
					CoordVec vec((xOffset+x)*Physics::CoordsPerTile, (yOffset+y)*Physics::CoordsPerTile);
					map->setTileAtCoordVec(vec, tile);
				}

				// Update progress (if needed).
				if (y%baseLayerYProgressDelta==baseLayerYProgressDelta-1)
					printf("MapGen: creating tiles %.1f%%.\n", ((y+1)*100.0)/height); // TODO: this better
			}

			// Tidy up.
			free(heightArray);

			return true;
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
					object->setTextureIdForAngle(CoordAngle0, TextureIdOldManS);
					object->setTextureIdForAngle(CoordAngle90, TextureIdOldManW);
					object->setTextureIdForAngle(CoordAngle180, TextureIdOldManN);
					object->setTextureIdForAngle(CoordAngle270, TextureIdOldManE);

					// Add object to map.
					if (!map->addObject(object)) {
						delete object;
						return NULL;
					}

					return object;
				} break;
				case BuiltinObject::Tree1: {
					// Create hitmask.
					const char hitmaskStr[64+1]=
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
					object->setTextureIdForAngle(CoordAngle0, TextureIdTree1);
					object->setTextureIdForAngle(CoordAngle90, TextureIdTree1);
					object->setTextureIdForAngle(CoordAngle180, TextureIdTree1);
					object->setTextureIdForAngle(CoordAngle270, TextureIdTree1);

					// Add object to map.
					if (!map->addObject(object)) {
						delete object;
						return NULL;
					}

					return object;
				} break;
				case BuiltinObject::Tree2: {
					const char hitmask0Str[64+1]=
						"________"
						"________"
						"________"
						"________"
						"________"
						"________"
						"________"
						"________";
					const char hitmask1Str[64+1]=
						"________"
						"________"
						"________"
						"________"
						"________"
						"________"
						"________"
						"________";
					const char hitmask2Str[64+1]=
						"_#######"
						"__######"
						"___#####"
						"_____###"
						"_____###"
						"_______#"
						"________"
						"________";
					HitMask hitmask00(hitmask0Str);
					HitMask hitmask01(hitmask1Str);
					HitMask hitmask02(hitmask2Str);

					HitMask hitmask10=hitmask00;
					hitmask10.flipHorizontally();
					HitMask hitmask11=hitmask01;
					hitmask11.flipHorizontally();
					HitMask hitmask12=hitmask02;
					hitmask12.flipHorizontally();

					// Create object.
					MapObject *object=new MapObject(rotation, pos, 2, 3);
					object->setHitMaskByTileOffset(0, 0, hitmask00);
					object->setHitMaskByTileOffset(0, 1, hitmask01);
					object->setHitMaskByTileOffset(0, 2, hitmask02);
					object->setHitMaskByTileOffset(1, 0, hitmask10);
					object->setHitMaskByTileOffset(1, 1, hitmask11);
					object->setHitMaskByTileOffset(1, 2, hitmask12);
					object->setTextureIdForAngle(CoordAngle0, TextureIdTree2);
					object->setTextureIdForAngle(CoordAngle90, TextureIdTree2);
					object->setTextureIdForAngle(CoordAngle180, TextureIdTree2);
					object->setTextureIdForAngle(CoordAngle270, TextureIdTree2);

					// Add object to map.
					if (!map->addObject(object)) {
						delete object;
						return NULL;
					}

					return object;
				} break;
				case BuiltinObject::Bush: {
					// Create hitmask.
					const char hitmaskStr[64+1]=
						"________"
						"________"
						"________"
						"________"
						"________"
						"________"
						"________"
						"________";
					HitMask hitmask(hitmaskStr);

					// Create object.
					MapObject *object=new MapObject(rotation, pos, 1, 1);
					object->setHitMaskByTileOffset(0, 0, hitmask);
					object->setTextureIdForAngle(CoordAngle0, TextureIdGrass5);
					object->setTextureIdForAngle(CoordAngle90, TextureIdGrass5);
					object->setTextureIdForAngle(CoordAngle180, TextureIdGrass5);
					object->setTextureIdForAngle(CoordAngle270, TextureIdGrass5);

					// Add object to map.
					if (!map->addObject(object)) {
						delete object;
						return NULL;
					}

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

			// Simply call addBuiltinObjectForestWithTestFunctor with NULL test functor.
			addBuiltinObjectForestWithTestFunctor(map, builtin, topLeft, widthHeight, interval, NULL, NULL);
		}

		void MapGen::addBuiltinObjectForestWithTestFunctor(class Map *map, BuiltinObject builtin, const CoordVec &topLeft, const CoordVec &widthHeight, const CoordVec &interval, MapGenAddBuiltinObjectForestTestFunctor *testFunctor, void *testFunctorUserData) {
			assert(map!=NULL);
			assert(widthHeight.x>=0 && widthHeight.y>=0);
			assert(interval.x>0 && interval.y>0);

			// Loop over rectangular region.
			CoordVec pos;
			for(pos.y=topLeft.y; pos.y<topLeft.y+widthHeight.y; pos.y+=interval.y)
				for(pos.x=topLeft.x; pos.x<topLeft.x+widthHeight.x; pos.x+=interval.x) {
					// Attempt to place object in this region.
					unsigned i;
					for(i=0; i<4; ++i) {
						// Calculate exact position.
						CoordVec randomOffset=CoordVec(rand()%interval.x, rand()%interval.y);
						CoordVec exactPosition=pos=randomOffset;

						// Run test functor.
						if (testFunctor!=NULL && !testFunctor(map, builtin, exactPosition, testFunctorUserData))
							continue;

						// Add object.
						if (addBuiltinObject(map, builtin, CoordAngle0, exactPosition)!=NULL)
							break;
					}
				}
		}
	};
};
