#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "mapgen.h"
#include "../physics/coord.h"
#include "../util.h"

using namespace Engine;

namespace Engine {
	namespace Map {
		void mapGenGenerateBinaryNoiseModifyTilesFunctor(class Map *map, unsigned x, unsigned y, void *userData) {
			assert(map!=NULL);
			assert(userData!=NULL);

			const MapGen::GenerateBinaryNoiseModifyTilesData *data=(const MapGen::GenerateBinaryNoiseModifyTilesData *)userData;

			// Calculate height.
			double height=data->noiseArray->eval(x, y);

			// Update tile layer.
			MapTile *tile=map->getTileAtOffset(x, y, true);
			if (tile!=NULL) {
				MapTile::Layer layer={.textureId=(height>=data->threshold ? data->highTextureId : data->lowTextureId)};
				tile->setLayer(data->tileLayer, layer);
			}
		}

		void mapGenModifyTilesProgressString(class Map *map, unsigned regionY, unsigned regionHeight, void *userData) {
			assert(map!=NULL);
			assert(regionY<regionHeight);
			assert(userData!=NULL);

			const char *string=(const char *)userData;

			Util::clearConsoleLine();
			printf("%s%.1f%%", string, ((regionY+1)*100.0)/regionHeight);
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
				[TextureIdSand]="../images/tiles/sand.png",
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
				[TextureIdSand]=4,
			};

			bool success=true;
			unsigned textureId;
			for(textureId=1; textureId<TextureIdNB; ++textureId)
				success&=map->addTexture(new MapTexture(textureId, texturePaths[textureId], textureScales[textureId]));

			return success;
		};

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

		bool MapGen::addHouse(class Map *map, unsigned baseX, unsigned baseY, unsigned totalW, unsigned totalH, unsigned tileLayer, bool showDoor, TileTestFunctor *testFunctor, void *testFunctorUserData) {
			assert(map!=NULL);

			// Check arguments are reasonable.
			if (totalW<=3 || totalH<=3)
				return false;

			// Choose parameters.
			const double roofRatio=0.6;
			unsigned roofHeight=(int)floor(roofRatio*totalH);

			unsigned doorOffset=(rand()%(totalW-3))+1;
			unsigned chimneyOffset=rand()%totalW;

			// Call addHouseFull to do most of the work.
			return addHouseFull(map, AddHouseFullFlags::All, baseX, baseY, totalW, totalH, roofHeight, tileLayer, doorOffset, chimneyOffset, testFunctor, testFunctorUserData);
		}

		bool MapGen::addHouseFull(class Map *map, AddHouseFullFlags flags, unsigned baseX, unsigned baseY, unsigned totalW, unsigned totalH, unsigned roofHeight, unsigned tileLayer, unsigned doorXOffset, unsigned chimneyXOffset, TileTestFunctor *testFunctor, void *testFunctorUserData) {
			assert(map!=NULL);

			unsigned tx, ty;

			// Check arguments are reasonable.
			if (totalW<5 || totalH<5)
				return false;
			if (roofHeight>=totalH-2)
				return false;
			if ((flags & AddHouseFullFlags::ShowDoor) && doorXOffset>=totalW)
				return false;
			if ((flags & AddHouseFullFlags::ShowChimney) && chimneyXOffset>=totalW)
				return false;

			// Check area is suitable.
			if (testFunctor!=NULL && !testFunctor(map, baseX, baseY, totalW, totalH, testFunctorUserData))
				return false;

			// Calculate constants.
			unsigned wallHeight=totalH-roofHeight;

			// Add walls.
			for(ty=0;ty<wallHeight;++ty)
				for(tx=0;tx<totalW;++tx) {
					MapTexture::Id texture;
					switch(ty%4) {
						case 0: texture=TextureIdHouseWall3; break;
						case 1: texture=TextureIdHouseWall2; break;
						case 2: texture=TextureIdHouseWall4; break;
						case 3: texture=TextureIdHouseWall2; break;
					}
					map->getTileAtCoordVec(CoordVec((baseX+tx)*Physics::CoordsPerTile, (baseY+totalH-1-ty)*Physics::CoordsPerTile), true)->setLayer(tileLayer, {.textureId=texture});
				}

			// Add door.
			if (flags & AddHouseFullFlags::ShowDoor) {
				int doorX=doorXOffset+baseX;
				map->getTileAtCoordVec(CoordVec(doorX*Physics::CoordsPerTile, (baseY+totalH-1)*Physics::CoordsPerTile), true)->setLayer(tileLayer, {.textureId=TextureIdHouseDoorBL});
				map->getTileAtCoordVec(CoordVec((doorX+1)*Physics::CoordsPerTile, (baseY+totalH-1)*Physics::CoordsPerTile), true)->setLayer(tileLayer, {.textureId=TextureIdHouseDoorBR});
				map->getTileAtCoordVec(CoordVec(doorX*Physics::CoordsPerTile, (baseY+totalH-2)*Physics::CoordsPerTile), true)->setLayer(tileLayer, {.textureId=TextureIdHouseDoorTL});
				map->getTileAtCoordVec(CoordVec((doorX+1)*Physics::CoordsPerTile, (baseY+totalH-2)*Physics::CoordsPerTile), true)->setLayer(tileLayer, {.textureId=TextureIdHouseDoorTR});
			}

			// Add main part of roof.
			for(ty=0;ty<roofHeight-1;++ty) // -1 due to ridge tiles added later
				for(tx=0;tx<totalW;++tx)
					map->getTileAtCoordVec(CoordVec((baseX+tx)*Physics::CoordsPerTile, (baseY+1+ty)*Physics::CoordsPerTile), true)->setLayer(tileLayer, {.textureId=TextureIdHouseRoof});

			// Add roof top ridge.
			for(tx=0;tx<totalW;++tx)
				map->getTileAtCoordVec(CoordVec((baseX+tx)*Physics::CoordsPerTile, baseY*Physics::CoordsPerTile), true)->setLayer(tileLayer, {.textureId=TextureIdHouseRoofTop});

			// Add chimney.
			if (flags & AddHouseFullFlags::ShowChimney) {
				int chimneyX=chimneyXOffset+baseX;
				map->getTileAtCoordVec(CoordVec(chimneyX*Physics::CoordsPerTile, baseY*Physics::CoordsPerTile), true)->setLayer(tileLayer, {.textureId=TextureIdHouseChimneyTop});
				map->getTileAtCoordVec(CoordVec(chimneyX*Physics::CoordsPerTile, (baseY+1)*Physics::CoordsPerTile), true)->setLayer(tileLayer, {.textureId=TextureIdHouseChimney});
			}

			return true;
		}

		bool MapGen::addTown(class Map *map, unsigned x0, unsigned y0, unsigned x1, unsigned y1, unsigned roadTileLayer, unsigned houseTileLayer, TileTestFunctor *testFunctor, void *testFunctorUserData) {
			assert(map!=NULL);
			assert(x0<=x1);
			assert(y0<=y1);
			assert(roadTileLayer<MapTile::layersMax);
			assert(houseTileLayer<MapTile::layersMax);

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
				int testFunctorX=road.x0-(road.isVertical() ? testFunctorMargin : 0);
				int testFunctorY=road.y0-(road.isHorizontal() ? testFunctorMargin : 0);
				int testFunctorWidth=road.trueX1-road.x0+(road.isVertical() ? testFunctorMargin*2 : 0);
				int testFunctorHeight=road.trueY1-road.y0+(road.isHorizontal() ? testFunctorMargin*2 : 0);
				if (!testFunctor(map, testFunctorX, testFunctorY, testFunctorWidth, testFunctorHeight, testFunctorUserData))
					continue;

				// Add road.
				roads.push_back(road);

				int a, b;
				for(a=road.y0; a<road.trueY1; ++a)
					for(b=road.x0; b<road.trueX1; ++b)
						map->getTileAtCoordVec(CoordVec(b*Physics::CoordsPerTile, a*Physics::CoordsPerTile), true)->setLayer(roadTileLayer, {.textureId=(road.width>=3 ? TextureIdBrickPath : TextureIdDirt)});

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

			if (roads.size()<3)
				return false;

			// Generate houses along roads.
			for(const auto road: roads) {
				int minWidth=4;
				int maxWidth=std::min(road.width+7, road.getLen()-1);
				if (minWidth>=maxWidth)
					continue;

				AddTownHouseData houseData;
				houseData.isHorizontal=road.isHorizontal();

				for(unsigned i=0; i<100; ++i) {
					houseData.genWidth=rand()%(maxWidth-minWidth)+minWidth;
					houseData.genDepth=rand()%road.width+5; // distance from edge with road to opposite edge

					unsigned j;
					for(j=0; j<20; ++j) {
						// Choose house position.
						houseData.side=(rand()%2==0);
						int offset=rand()%(road.getLen()-houseData.genWidth);

						houseData.x=(road.isHorizontal() ? road.x0+offset : (houseData.side ? road.trueX1 : road.x0-houseData.genDepth));
						houseData.y=(road.isVertical() ? road.y0+offset : (houseData.side ? road.trueY1 : road.y0-houseData.genDepth));
						houseData.mapW=(road.isHorizontal() ? houseData.genWidth : houseData.genDepth);
						houseData.mapH=(road.isVertical() ? houseData.genWidth : houseData.genDepth);

						// Test house area is valid.
						if (road.isHorizontal()) {
							if (!testFunctor(map, houseData.x-1, houseData.y, houseData.mapW+2, houseData.mapH, testFunctorUserData))
								continue;
						} else {
							if (!testFunctor(map, houseData.x, houseData.y-1, houseData.mapW, houseData.mapH+2, testFunctorUserData))
								continue;
						}

						// Compute house parameters.
						bool showDoor=(road.isHorizontal() && !houseData.side);
						houseData.flags=(AddHouseFullFlags)(AddHouseFullFlags::ShowChimney|(showDoor ? AddHouseFullFlags::ShowDoor : AddHouseFullFlags::None));

						const double houseRoofRatio=0.6;
						houseData.roofHeight=(int)floor(houseRoofRatio*houseData.mapH);

						houseData.doorOffset=(rand()%(houseData.mapW-3))+1;
						houseData.chimneyOffset=rand()%houseData.mapW;

						// Attempt to add the house.
						if (!addHouseFull(map, houseData.flags, houseData.x, houseData.y, houseData.mapW, houseData.mapH, houseData.roofHeight, houseTileLayer, houseData.doorOffset, houseData.chimneyOffset, NULL, NULL))
							continue;

						break;
					}
				}
			}

			return true;
		}

		bool MapGen::addTowns(class Map *map, unsigned x0, unsigned y0, unsigned x1, unsigned y1, unsigned roadTileLayer, unsigned houseTileLayer, double totalPopulation, MapGen::TileTestFunctor *testFunctor, void *testFunctorUserData) {
			assert(map!=NULL);
			assert(x0<=x1);
			assert(y0<=y1);
			assert(roadTileLayer<MapTile::layersMax);
			assert(houseTileLayer<MapTile::layersMax);

			const double peoplePerSqKm=20000.0;

			const double initialTownPop=Util::randInInterval(10.0,20.0)*sqrt(totalPopulation);
			for(double townPop=initialTownPop; townPop>=30; townPop*=Util::randInInterval(0.6,0.8)) {
				const double townSizeSqKm=townPop/peoplePerSqKm;

				const int townSize=1000.0*sqrt(townSizeSqKm);
				//const unsigned desiredCount=1;
				const unsigned desiredCount=ceil(initialTownPop/townPop);
				printf("	attempting to add %u towns, each with pop %.0f, size %.3fkm^2 (%'im per side)\n", desiredCount, townPop, townSizeSqKm, townSize);

				const unsigned attemptMax=desiredCount*4;
				unsigned addedCount=0;
				for(unsigned i=0; i<attemptMax && addedCount<desiredCount; ++i) {
					int townX=Util::randInInterval(x0, x1);
					int townY=Util::randInInterval(y0, y1);
					bool horizontal=(rand()%2==0);

					if (horizontal) {
						int townX0=townX-townSize/2;
						int townX1=townX+townSize/2;
						if (townX0<0)
							continue;
						addedCount+=MapGen::addTown(map, townX0, townY, townX1, townY, roadTileLayer, houseTileLayer, testFunctor, testFunctorUserData);
					} else {
						int townY0=townY-townSize/2;
						int townY1=townY+townSize/2;
						if (townY0<0)
							continue;
						addedCount+=MapGen::addTown(map, townX, townY0, townX, townY1, roadTileLayer, houseTileLayer, testFunctor, testFunctorUserData);
					}
				}
			}

			return true;
		}

		void MapGen::modifyTiles(class Map *map, unsigned x, unsigned y, unsigned width, unsigned height, ModifyTilesFunctor *functor, void *functorUserData, ModifyTilesProgress *progressFunctor, void *progressUserData) {
			assert(map!=NULL);
			assert(functor!=NULL);

			MapGen::ModifyTilesManyEntry entry={
				.functor=functor,
				.userData=functorUserData,
			};
			MapGen::ModifyTilesManyEntry *functorArray[1]={&entry};

			MapGen::modifyTilesMany(map, x, y, width, height, 1, functorArray, progressFunctor, progressUserData);
		}

		void MapGen::modifyTilesMany(class Map *map, unsigned x, unsigned y, unsigned width, unsigned height, size_t functorArrayCount, ModifyTilesManyEntry *functorArray[], ModifyTilesProgress *progressFunctor, void *progressUserData) {
			assert(map!=NULL);
			assert(functorArrayCount>0);
			assert(functorArray!=NULL);

			// Calculate constants.
			const unsigned regionX0=x/MapRegion::tilesWide;
			const unsigned regionY0=y/MapRegion::tilesHigh;
			const unsigned regionX1=(x+width)/MapRegion::tilesWide;
			const unsigned regionY1=(y+height)/MapRegion::tilesHigh;

			// Initial progress update (if needed).
			if (progressFunctor!=NULL)
				progressFunctor(map, 0, regionY1-regionY0, progressUserData);

			// Loop over each region
			unsigned regionX, regionY;
			for(regionY=regionY0; regionY<regionY1; ++regionY) {
				const unsigned regionYOffset=regionY*MapRegion::tilesHigh;
				for(regionX=regionX0; regionX<regionX1; ++regionX) {
					const unsigned regionXOffset=regionX*MapRegion::tilesWide;

					// Set region dirty.
					MapRegion *region=map->getRegionAtOffset(regionX, regionY, true);
					region->setDirty();

					// Loop over all rows in this region.
					unsigned tileX, tileY;
					for(tileY=0; tileY<MapRegion::tilesHigh; ++tileY) {
						// Loop over all tiles in this row.
						for(tileX=0; tileX<MapRegion::tilesWide; ++tileX) {
							// Loop over functors.
							for(size_t functorId=0; functorId<functorArrayCount; ++functorId)
								functorArray[functorId]->functor(map, regionXOffset+tileX, regionYOffset+tileY, functorArray[functorId]->userData);
						}
					}
				}

				// Update progress (if needed).
				if (progressFunctor!=NULL)
					progressFunctor(map, regionY-regionY0, regionY1-regionY0, progressUserData);
			}
		}
	};
};
