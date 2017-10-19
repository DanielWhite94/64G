#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "mapgen.h"
#include "../physics/coord.h"
#include "../fbnnoise.h"
#include "../util.h"

using namespace Engine;

namespace Engine {
	namespace Map {
		void mapGenGenerateWaterLandModifyTilesProgress(class Map *map, unsigned y, unsigned height, void *userData);
		void mapGenGenerateWaterLandModifyTilesFunctor(class Map *map, unsigned x, unsigned y, void *userData);

		void mapGenGenerateWaterLandModifyTilesFunctor(class Map *map, unsigned x, unsigned y, void *userData) {
			assert(map!=NULL);
			assert(userData!=NULL);

			const MapGen::GenerateWaterLandModifyTilesFunctorData *data=(const MapGen::GenerateWaterLandModifyTilesFunctorData *)userData;

			// Calculate height.
			unsigned heightY=y*data->heightYFactor;
			unsigned heightX=x*data->heightXFactor;
			double height=data->heightArray[heightX+heightY*data->heightNoiseWidth];

			// Update tile layer.
			MapTile *tile=map->getTileAtOffset(x, y);
			if (tile!=NULL) {
				MapTileLayer layer={.textureId=(height>=data->landHeight ? data->landTextureId : data->waterTextureId)};
				tile->setLayer(data->tileLayer, layer);
			}
		}

		void mapGenGenerateWaterLandModifyTilesProgress(class Map *map, unsigned y, unsigned height, void *userData) {
			assert(map!=NULL);
			assert(y<height);
			assert(userData==NULL);

			Util::clearConsoleLine();
			printf("MapGen: generating water/land tiles %.1f%%.", ((y+1)*100.0)/height); // TODO: this better
			fflush(stdout);
		}

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
				if (y%noiseYProgressDelta==noiseYProgressDelta-1) {
					Util::clearConsoleLine();
					printf("MapGen: generating height noise %.1f%%.", ((y+1)*100.0)/heightNoiseHeight); // TODO: this better
					fflush(stdout);
				}
			}
			printf("\n");

			// Choose land height (using a binary search).
			double heightXFactor=((double)heightNoiseWidth)/width;
			double heightYFactor=((double)heightNoiseHeight)/height;

			double landHeight=-0.12; // TODO: think about this - issue is land isnt reproducible at different sizes due to more/less land changing the landHeight

			// Create base tile layer - water/land.
			printf("MapGen: creating water/land tiles... (land height %.2f)\n", landHeight);

			unsigned progressDelta=height/16;

			GenerateWaterLandModifyTilesFunctorData *modifyTilesData=(GenerateWaterLandModifyTilesFunctorData *)malloc(sizeof(GenerateWaterLandModifyTilesFunctorData));
			modifyTilesData->heightArray=heightArray;
			modifyTilesData->heightXFactor=heightXFactor;
			modifyTilesData->heightYFactor=heightYFactor;
			modifyTilesData->heightNoiseWidth=heightNoiseWidth;
			modifyTilesData->landHeight=landHeight;
			modifyTilesData->landTextureId=landTextureId;
			modifyTilesData->waterTextureId=waterTextureId;
			modifyTilesData->tileLayer=tileLayer;

			modifyTiles(map, xOffset, yOffset, width, height, &mapGenGenerateWaterLandModifyTilesFunctor, (void *)modifyTilesData, progressDelta, &mapGenGenerateWaterLandModifyTilesProgress, NULL);

			free(modifyTilesData);

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

		void MapGen::addBuiltinObjectForestWithTestFunctor(class Map *map, BuiltinObject builtin, const CoordVec &topLeft, const CoordVec &widthHeight, const CoordVec &interval, ObjectTestFunctor *testFunctor, void *testFunctorUserData) {
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
						CoordVec exactPosition=pos+randomOffset;

						// Run test functor.
						if (testFunctor!=NULL && !testFunctor(map, builtin, exactPosition, testFunctorUserData))
							continue;

						// Add object.
						if (addBuiltinObject(map, builtin, CoordAngle0, exactPosition)!=NULL)
							break;
					}
				}
		}

		bool MapGen::addHouse(class Map *map, unsigned x, unsigned y, unsigned w, unsigned h, unsigned tileLayer, bool showDoor, TileTestFunctor *testFunctor, void *testFunctorUserData) {
			assert(map!=NULL);

			unsigned tx, ty;

			// Choose parameters.
			const double roofRatio=0.6;

			// Check width and height are reasonable.
			if (w<5 || h<5)
				return false;

			// Check area is suitable.
			if (testFunctor!=NULL && !testFunctor(map, x, y, w, h, testFunctorUserData))
				return false;

			// Calculate constants.
			unsigned roofHeight=(int)floor(roofRatio*h);
			unsigned wallHeight=h-roofHeight;

			// Add walls.
			for(ty=0;ty<wallHeight;++ty)
				for(tx=0;tx<w;++tx) {
					unsigned texture;
					switch(ty%4) {
						case 0: texture=TextureIdHouseWall3; break;
						case 1: texture=TextureIdHouseWall2; break;
						case 2: texture=TextureIdHouseWall4; break;
						case 3: texture=TextureIdHouseWall2; break;
					}
					MapTile tile(texture, tileLayer);
					CoordVec vec((x+tx)*Physics::CoordsPerTile, (y+h-1-ty)*Physics::CoordsPerTile);
					map->setTileAtCoordVec(vec, tile);
				}

			// Add door.
			if (showDoor) {
				unsigned doorX=(rand()%(w-3))+x+1;
				map->setTileAtCoordVec(CoordVec(doorX*Physics::CoordsPerTile, (y+h-1)*Physics::CoordsPerTile), MapTile(TextureIdHouseDoorBL, tileLayer));
				map->setTileAtCoordVec(CoordVec((doorX+1)*Physics::CoordsPerTile, (y+h-1)*Physics::CoordsPerTile), MapTile(TextureIdHouseDoorBR, tileLayer));
				map->setTileAtCoordVec(CoordVec(doorX*Physics::CoordsPerTile, (y+h-2)*Physics::CoordsPerTile), MapTile(TextureIdHouseDoorTL, tileLayer));
				map->setTileAtCoordVec(CoordVec((doorX+1)*Physics::CoordsPerTile, (y+h-2)*Physics::CoordsPerTile), MapTile(TextureIdHouseDoorTR, tileLayer));
			}

			// Add main part of roof.
			for(ty=0;ty<roofHeight-1;++ty) // -1 due to ridge tiles added later
				for(tx=0;tx<w;++tx) {
					MapTile tile(TextureIdHouseRoof, tileLayer);
					CoordVec vec((x+tx)*Physics::CoordsPerTile, (y+1+ty)*Physics::CoordsPerTile);
					map->setTileAtCoordVec(vec, tile);
				}

			// Add roof top ridge.
			for(tx=0;tx<w;++tx) {
				MapTile tile(TextureIdHouseRoofTop, tileLayer);
				CoordVec vec((x+tx)*Physics::CoordsPerTile, y*Physics::CoordsPerTile);
				map->setTileAtCoordVec(vec, tile);
			}

			// Add chimney.
			unsigned chimneyX=rand()%w+x;
			map->setTileAtCoordVec(CoordVec(chimneyX*Physics::CoordsPerTile, y*Physics::CoordsPerTile), MapTile(TextureIdHouseChimneyTop, tileLayer));
			map->setTileAtCoordVec(CoordVec(chimneyX*Physics::CoordsPerTile, (y+1)*Physics::CoordsPerTile), MapTile(TextureIdHouseChimney, tileLayer));

			return true;
		}

		bool MapGen::addTown(class Map *map, unsigned x0, unsigned y0, unsigned x1, unsigned y1, unsigned tileLayer, TileTestFunctor *testFunctor, void *testFunctorUserData) {
			assert(map!=NULL);
			assert(x0<=x1);
			assert(y0<=y1);
			assert(tileLayer<MapTile::layersMax);

			// Check either horizontal or vertical.
			if (!((y0==y1) || (x0==x1)))
				return false;

			// Compute constants.
			const int townCentreX=(x0+x1)/2;
			const int townCentreY=(y0+y1)/2;
			const int townRadius=((x1-x0)+(y1-y0))/2+1;
			const int townRadiusSquared=townRadius*townRadius;

			// Add initial road.
			priority_queue<MapGenRoad, vector<MapGenRoad>, CompareMapGenRoadWeight> pq;
			vector<MapGenRoad> roads;

			MapGenRoad initialRoad;
			initialRoad.x0=x0;
			initialRoad.y0=y0;
			initialRoad.x1=x1;
			initialRoad.y1=y1;
			initialRoad.width=ceil(log2(initialRoad.getLen()));
			initialRoad.trueX1=(initialRoad.isVertical() ? initialRoad.x0+initialRoad.width : initialRoad.x1+1);
			initialRoad.trueY1=(initialRoad.isHorizontal() ? initialRoad.y0+initialRoad.width : initialRoad.y1+1);
			initialRoad.weight=initialRoad.getLen()*initialRoad.width;
			pq.push(initialRoad);

			// Add roads from queue.
			while(!pq.empty()) {
				// Pop road.
				MapGenRoad road=pq.top();
				pq.pop();

				// Can we add this road?
				if (road.width<2 || road.getLen()<5)
					continue;

				if ((road.x0-townCentreX)*(road.x0-townCentreX)+(road.y0-townCentreY)*(road.y0-townCentreY)>townRadiusSquared)
					continue;
				if ((road.x1-townCentreX)*(road.x1-townCentreX)+(road.y1-townCentreY)*(road.y1-townCentreY)>townRadiusSquared)
					continue;

				int testFunctorMargin=std::min(6*road.width-2, 25);
				if (!testFunctor(map, road.x0-(road.isVertical() ? testFunctorMargin : 0), road.y0-(road.isHorizontal() ? testFunctorMargin : 0), road.trueX1-road.x0+(road.isVertical() ? testFunctorMargin*2 : 0), road.trueY1-road.y0+(road.isHorizontal() ? testFunctorMargin*2 : 0), testFunctorUserData))
					continue;

				// Add road.
				roads.push_back(road);

				int a, b;
				for(a=road.y0; a<road.trueY1; ++a)
					for(b=road.x0; b<road.trueX1; ++b)
						map->setTileAtCoordVec(CoordVec(b*Physics::CoordsPerTile, a*Physics::CoordsPerTile), MapTile((road.width>=3 ? TextureIdBrickPath : TextureIdDirt), tileLayer));

				// Add potential child roads.
				MapGenRoad newRoad;

				for(unsigned i=0; i<16; ++i) {
					newRoad.width=(road.width*((rand()%5)+5))/10;
					int offset, jump=std::max(newRoad.width+8,road.getLen()/6);
					for(offset=jump; offset<=road.getLen()-newRoad.width; offset+=jump/2+rand()%jump) {
						// Create candidate child road.
						int newLen=rand()%(4*((1u)<<(newRoad.width)));
						newRoad.weight=newLen*newRoad.width;

						bool greater=rand()%2;
						int randOffset=0;
						newRoad.x0=(road.isHorizontal() ? road.x0+offset+randOffset : (greater ? road.trueX1 : road.x0-newLen-1));
						newRoad.y0=(road.isVertical() ? road.y0+offset+randOffset : (greater ? road.trueY1 : road.y0-newLen-1));
						newRoad.x1=(road.isHorizontal() ? newRoad.x0 : newRoad.x0+newLen);
						newRoad.y1=(road.isVertical() ? newRoad.y0 : newRoad.y0+newLen);

						newRoad.trueX1=(road.isHorizontal() ? newRoad.x0+newRoad.width : newRoad.x1+1);
						newRoad.trueY1=(road.isVertical() ? newRoad.y0+newRoad.width : newRoad.y1+1);

						// Push to queue.
						pq.push(newRoad);
					}
				}
			}

			// Add houses along roads.
			for(const auto road: roads) {
				//if (!road.isHorizontal())
				int minWidth=4;
				int maxWidth=std::min(road.width+7, road.getLen()-1);
				if (minWidth>=maxWidth)
					continue;

				for(unsigned i=0; i<100; ++i) {
					int width=rand()%(maxWidth-minWidth)+minWidth;
					int depth=rand()%road.width+5; // distance from edge with road to opposite edge

					unsigned j;
					for(j=0; j<20; ++j) {
						bool greater=(rand()%2==0);
						int offset=rand()%(road.getLen()-width);

						int hx=(road.isHorizontal() ? road.x0+offset : (greater ? road.trueX1 : road.x0-depth));
						int hy=(road.isVertical() ? road.y0+offset : (greater ? road.trueY1 : road.y0-depth));
						int hw=(road.isHorizontal() ? width : depth);
						int hh=(road.isVertical() ? width : depth);

						if (road.isHorizontal()) {
							if (testFunctor(map, hx-1, hy, hw+2, hh, testFunctorUserData)) {
								addHouse(map, hx, hy, hw, hh, tileLayer, !greater, NULL, NULL);
								break;
							}
						} else {
							if (testFunctor(map, hx, hy-1, hw, hh+2, testFunctorUserData)) {
								addHouse(map, hx, hy, hw, hh, tileLayer, false, NULL, NULL);
								break;
							}
						}
					}
				}
			}

			return true;
		}

		void MapGen::modifyTiles(class Map *map, unsigned x, unsigned y, unsigned width, unsigned height, ModifyTilesFunctor *functor, void *functorUserData, unsigned progressDelta, ModifyTilesProgress *progressFunctor, void *progressUserData) {
			assert(map!=NULL);
			assert(functor!=NULL);
			assert(progressDelta>0);

			unsigned tx, ty;
			for(ty=0;ty<height;++ty) {
				// Loop over each tile in this row.
				for(tx=0;tx<width;++tx) {
					// Set region dirty.
					unsigned regionX=(x+tx)/MapRegion::tilesWide;
					unsigned regionY=(y+ty)/MapRegion::tilesHigh;
					MapRegion *region=map->getRegionAtOffset(regionX, regionY);
					region->setDirty();

					// Call functor.
					functor(map, x+tx, y+ty, functorUserData);
				}

				// Update progress (if needed).
				if (progressFunctor!=NULL && ty%progressDelta==progressDelta-1)
					progressFunctor(map, ty, height, progressUserData);
			}
		}
	};
};
