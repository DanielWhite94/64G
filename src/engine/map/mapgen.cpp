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
			const unsigned TextureIdNone=0;
			const unsigned TextureIdGrass0=1;
			const unsigned TextureIdGrass1=2;
			const unsigned TextureIdGrass2=3;
			const unsigned TextureIdGrass3=4;
			const unsigned TextureIdGrass4=5;
			const unsigned TextureIdGrass5=6;
			const unsigned TextureIdBrickPath=7;
			const unsigned TextureIdDirt=8;
			const unsigned TextureIdDock=9;
			const unsigned TextureIdWater=10;
			const unsigned TextureIdTree1=11;
			const unsigned TextureIdTree2=12;
			const unsigned TextureIdMan1=13;
			const unsigned TextureIdNB=14;
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
			};

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
			printf("MapGen: creating map...\n");
			class Map *map=new Map();

			// Create textures.
			printf("MapGen: creating textures...\n");
			bool textureError=false;
			unsigned textureId;
			for(textureId=1; textureId<TextureIdNB; ++textureId)
				textureError|=!map->addTexture(new MapTexture(textureId, texturePaths[textureId], textureScales[textureId]));

			// Create base tile layer - water/grass.
			printf("MapGen: creating water/land tiles...\n");
			heightArrayPtr=heightArray;
			for(y=0;y<height;++y) {
				for(x=0;x<width;++x,++heightArrayPtr) {
					MapTile tile(*heightArrayPtr>=0.0 ? rand()%5+1 : 10); // TODO: Do not hardcode texture ids.
					CoordVec vec(x*Physics::CoordsPerTile, y*Physics::CoordsPerTile);
					map->setTileAtCoordVec(vec, tile);
				}

				// Update progress (if needed).
				if (y%yProgressDelta==yProgressDelta-1)
					printf("MapGen: creating map %.1f%%.\n", ((y+1)*100.0)/height); // TODO: this better
			}

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
					object->tempSetTextureId(13);

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
					object->tempSetTextureId(11);

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
					object->tempSetTextureId(12);

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
					object->tempSetTextureId(6);

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

			CoordVec pos;
			for(pos.y=topLeft.y; pos.y<topLeft.y+widthHeight.y; pos.y+=interval.y)
				for(pos.x=topLeft.x; pos.x<topLeft.x+widthHeight.x; pos.x+=interval.x) {
					unsigned i;
					for(i=0; i<4; ++i) {
						CoordVec randomOffset=CoordVec(rand()%interval.x, rand()%interval.y);
						if (addBuiltinObject(map, builtin, CoordAngle0, pos+randomOffset)!=NULL)
							break;
					}
				}
		}
	};
};
