#include <algorithm>
#include <cassert>
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <thread>

#include "mapgen.h"
#include "../physics/coord.h"
#include "../util.h"
#include "../fbnnoise.h"

using namespace Engine;

namespace Engine {
	namespace Map {
		struct MapGenRecalculateStatsThreadData {
			double minHeight, maxHeight;
			double minTemperature, maxTemperature;
			double minMoisture, maxMoisture;
		};

		void mapGenRecalculateStatsModifyTilesFunctor(unsigned threadId, class Map *map, unsigned x, unsigned y, void *userData);

		MapGen::MapGen() {
		}

		MapGen::~MapGen() {
		};

		bool MapGen::addHouse(class Map *map, unsigned baseX, unsigned baseY, unsigned totalW, unsigned totalH, const AddHouseParameters *params) {
			assert(map!=NULL);
			assert(params!=NULL);

			unsigned tx, ty;

			const int doorW=(params->flags & AddHouseFlags::ShowDoor) ? 2 : 0;

			// Check arguments are reasonable.
			if (totalW<5 || totalH<5)
				return false;
			if (params->roofHeight>=totalH-2)
				return false;
			if ((params->flags & AddHouseFlags::ShowDoor) && params->doorXOffset+doorW>totalW)
				return false;
			if ((params->flags & AddHouseFlags::ShowChimney) && params->chimneyXOffset>=totalW)
				return false;

			// Check area is suitable.
			if (params->testFunctor!=NULL && !params->testFunctor(map, baseX, baseY, totalW, totalH, params->testFunctorUserData))
				return false;

			// Calculate constants.
			unsigned wallHeight=totalH-params->roofHeight;

			// Add walls.
			for(ty=0;ty<wallHeight;++ty)
				for(tx=0;tx<totalW;++tx) {
					MapTexture::Id texture;
					switch(ty%4) {
						case 0: texture=params->textureIdWall0; break;
						case 1: texture=params->textureIdWall1; break;
						case 2: texture=params->textureIdWall0; break;
						case 3: texture=params->textureIdWall1; break;
					}
					map->getTileAtCoordVec(CoordVec((baseX+tx)*Physics::CoordsPerTile, (baseY+totalH-1-ty)*Physics::CoordsPerTile), Map::Map::GetTileFlag::CreateDirty)->setLayer(params->tileLayer, {.textureId=texture, .hitmask=HitMask(HitMask::fullMask)});
				}

			// Add door.
			if (params->flags & AddHouseFlags::ShowDoor) {
				int doorX=params->doorXOffset+baseX;
				map->getTileAtCoordVec(CoordVec(doorX*Physics::CoordsPerTile, (baseY+totalH-1)*Physics::CoordsPerTile), Map::Map::GetTileFlag::CreateDirty)->setLayer(params->tileLayer, {.textureId=params->textureIdHouseDoorBL, .hitmask=HitMask(HitMask::fullMask)});
				map->getTileAtCoordVec(CoordVec((doorX+1)*Physics::CoordsPerTile, (baseY+totalH-1)*Physics::CoordsPerTile), Map::Map::GetTileFlag::CreateDirty)->setLayer(params->tileLayer, {.textureId=params->textureIdHouseDoorBR, .hitmask=HitMask(HitMask::fullMask)});
				map->getTileAtCoordVec(CoordVec(doorX*Physics::CoordsPerTile, (baseY+totalH-2)*Physics::CoordsPerTile), Map::Map::GetTileFlag::CreateDirty)->setLayer(params->tileLayer, {.textureId=params->textureIdHouseDoorTL, .hitmask=HitMask(HitMask::fullMask)});
				map->getTileAtCoordVec(CoordVec((doorX+1)*Physics::CoordsPerTile, (baseY+totalH-2)*Physics::CoordsPerTile), Map::Map::GetTileFlag::CreateDirty)->setLayer(params->tileLayer, {.textureId=params->textureIdHouseDoorTR, .hitmask=HitMask(HitMask::fullMask)});
			}

			// Add main part of roof.
			for(ty=0;ty<params->roofHeight-1;++ty) // -1 due to ridge tiles added later
				for(tx=0;tx<totalW;++tx)
					map->getTileAtCoordVec(CoordVec((baseX+tx)*Physics::CoordsPerTile, (baseY+1+ty)*Physics::CoordsPerTile), Map::Map::GetTileFlag::CreateDirty)->setLayer(params->tileLayer, {.textureId=params->textureIdHouseRoof, .hitmask=HitMask(HitMask::fullMask)});

			// Add roof top ridge.
			for(tx=0;tx<totalW;++tx)
				map->getTileAtCoordVec(CoordVec((baseX+tx)*Physics::CoordsPerTile, baseY*Physics::CoordsPerTile), Map::Map::GetTileFlag::CreateDirty)->setLayer(params->tileLayer, {.textureId=params->textureIdHouseRoofTop, .hitmask=HitMask(HitMask::fullMask)});

			// Add chimney.
			if (params->flags & AddHouseFlags::ShowChimney) {
				int chimneyX=params->chimneyXOffset+baseX;
				map->getTileAtCoordVec(CoordVec(chimneyX*Physics::CoordsPerTile, baseY*Physics::CoordsPerTile), Map::Map::GetTileFlag::CreateDirty)->setLayer(params->tileLayer, {.textureId=params->textureIdHouseChimneyTop, .hitmask=HitMask(HitMask::fullMask)});
				map->getTileAtCoordVec(CoordVec(chimneyX*Physics::CoordsPerTile, (baseY+1)*Physics::CoordsPerTile), Map::Map::GetTileFlag::CreateDirty)->setLayer(params->tileLayer, {.textureId=params->textureIdHouseChimney, .hitmask=HitMask(HitMask::fullMask)});
			}

			// Add some decoration.
			if ((params->flags & AddHouseFlags::AddDecoration)) {
				// Add path infront of door (if any).
				if (params->flags & AddHouseFlags::ShowDoor) {
					for(int offset=0; offset<doorW; ++offset) {
						int posX=baseX+params->doorXOffset+offset;
						map->getTileAtCoordVec(CoordVec(posX*Physics::CoordsPerTile, (baseY+totalH)*Physics::CoordsPerTile), Map::Map::GetTileFlag::CreateDirty)->setLayer(params->tileLayer, {.textureId=params->textureIdBrickPath, .hitmask=HitMask()});
					}
				}

				// Random chance of adding a rose bush at random position (avoiding the door, if any).
				if (Util::randIntInInterval(0, 4)==0) {
					int offset=Util::randIntInInterval(0, totalW-doorW);
					if ((params->flags & AddHouseFlags::ShowDoor) && offset>=params->doorXOffset)
						offset+=doorW;
					assert(offset>=0 && offset<totalW);

					int posX=offset+baseX;
					map->getTileAtCoordVec(CoordVec(posX*Physics::CoordsPerTile, (baseY+totalH)*Physics::CoordsPerTile), Map::Map::GetTileFlag::CreateDirty)->setLayer(params->decorationLayer, {.textureId=params->textureIdRoseBush, .hitmask=HitMask()});
				}
			}

			return true;
		}

		bool MapGen::addTown(class Map *map, unsigned x0, unsigned y0, unsigned x1, unsigned y1, const AddTownParameters *params, const AddHouseParameters *houseParams, int townPop) {
			assert(map!=NULL);
			assert(x0<=x1);
			assert(y0<=y1);
			assert(params!=NULL);
			assert(params->roadTileLayer<MapTile::layersMax);
			assert(houseParams!=NULL);

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
				if (!params->testFunctor(map, testFunctorX, testFunctorY, testFunctorWidth, testFunctorHeight, params->testFunctorUserData))
					continue;

				// Add road.
				roads.push_back(road);

				int a, b;
				for(a=road.y0; a<road.trueY1; ++a)
					for(b=road.x0; b<road.trueX1; ++b)
						map->getTileAtCoordVec(CoordVec(b*Physics::CoordsPerTile, a*Physics::CoordsPerTile), Map::Map::GetTileFlag::CreateDirty)->setLayer(params->roadTileLayer, {.textureId=(road.width>=3 ? params->textureIdMajorPath : params->textureIdMinorPath), .hitmask=HitMask()});

				// Add potential child roads.
				MapGenRoad newRoad;
				for(unsigned i=0; i<16; ++i) {
					newRoad.width=(road.width*Util::randIntInInterval(5, 10))/10;
					int offset, jump=std::max(newRoad.width+8,road.getLen()/6);
					for(offset=jump; offset<=road.getLen()-newRoad.width; offset+=jump/2+Util::randIntInInterval(0, jump)) {
						// Create candidate child road.
						int newLen=Util::randIntInInterval(0, 4*((1u)<<(newRoad.width)));
						newRoad.weight=newLen*newRoad.width;

						bool greater=Util::randBool();
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

			// Generate buildings along roads.
			vector<AddTownHouseData> houses;
			for(const auto road: roads) {
				int minWidth=4;
				int maxWidth=std::min(road.width+7, road.getLen()-1);
				if (minWidth>=maxWidth)
					continue;

				AddTownHouseData houseData;
				houseData.isHorizontal=road.isHorizontal();

				for(unsigned i=0; i<100; ++i) {
					houseData.genWidth=Util::randIntInInterval(minWidth, maxWidth);
					houseData.genDepth=Util::randIntInInterval(5, 5+road.width);

					unsigned j;
					for(j=0; j<20; ++j) {
						// Choose house position.
						houseData.side=Util::randBool();
						int offset=Util::randIntInInterval(0, road.getLen()-houseData.genWidth);

						houseData.x=(road.isHorizontal() ? road.x0+offset : (houseData.side ? road.trueX1 : road.x0-houseData.genDepth-1));
						houseData.y=(road.isVertical() ? road.y0+offset : (houseData.side ? road.trueY1 : road.y0-houseData.genDepth-1));
						houseData.mapW=(road.isHorizontal() ? houseData.genWidth : houseData.genDepth);
						houseData.mapH=(road.isVertical() ? houseData.genWidth : houseData.genDepth);

						// Test house area is valid.
						if (road.isHorizontal()) {
							if (!params->testFunctor(map, houseData.x-1, houseData.y, houseData.mapW+2, houseData.mapH, params->testFunctorUserData))
								continue;
						} else {
							if (!params->testFunctor(map, houseData.x, houseData.y-1, houseData.mapW, houseData.mapH+2, params->testFunctorUserData))
								continue;
						}

						// Compute house parameters.
						AddHouseParameters houseParamsCopy=*houseParams;

						if (road.isHorizontal() && !houseData.side)
							houseParamsCopy.flags=(AddHouseFlags)(houseParamsCopy.flags|AddHouseFlags::ShowDoor);
						else
							houseParamsCopy.flags=(AddHouseFlags)(houseParamsCopy.flags&~AddHouseFlags::ShowDoor);

						const double houseRoofRatio=0.6;
						houseParamsCopy.roofHeight=(int)floor(houseRoofRatio*houseData.mapH);

						houseParamsCopy.doorXOffset=Util::randIntInInterval(1, houseData.mapW-2);
						houseParamsCopy.chimneyXOffset=Util::randIntInInterval(0, houseData.mapW);

						// Attempt to add the house.
						if (!addHouse(map, houseData.x, houseData.y, houseData.mapW, houseData.mapH, &houseParamsCopy))
							continue;

						// Add to array of houses.
						houses.push_back(houseData);

						break;
					}
				}
			}

			// Calculate number of shops desired.
			const int peoplePerShopType[AddTownsShopType::NB]={
				[AddTownsShopType::Shoemaker]=150,
				[AddTownsShopType::Furrier]=250,
				[AddTownsShopType::Tailor]=250,
				[AddTownsShopType::Barber]=350,
				[AddTownsShopType::Jeweler]=400,
				[AddTownsShopType::Tavern]=400,
				[AddTownsShopType::Carpenters]=550,
				[AddTownsShopType::Bakers]=800,
			};
			unsigned shopsPeopleTotal=0;
			for(int i=0; i<AddTownsShopType::NB; ++i)
				shopsPeopleTotal+=peoplePerShopType[i];

			// Give buildings some purpose.
			for(auto houseData : houses) {
				// Give the building a purpose (e.g. a shop).
				if (houseParams->flags & AddHouseFlags::ShowDoor) {
					// Choose sign.
					MapTexture::Id signTextureId;
					int r=Util::randIntInInterval(0, shopsPeopleTotal);
					int total=0;
					int i;
					for(i=0; i<AddTownsShopType::NB; ++i) {
						total+=peoplePerShopType[i];
						if (r<total)
							break;
					}

					switch(i) {
						case 0:
						case 1:
						case 2:
						case 3:
						case 4:
						case 5:
						case 6:
						case 7:
						case 8:
						case 9:
							signTextureId=params->textureIdShopSignCobbler;
						break;
						default:
							signTextureId=params->textureIdShopSignNone;
						break;
					}

					// Compute sign position.
					int signOffset;
					if (houseParams->doorXOffset<houseData.mapW/2)
						signOffset=2;
					else
						signOffset=-1;
					int signX=signOffset+houseParams->doorXOffset+houseData.x;

					// Add sign.
					map->getTileAtCoordVec(CoordVec(signX*Physics::CoordsPerTile, (houseData.y+houseData.mapH-2)*Physics::CoordsPerTile), Map::Map::GetTileFlag::CreateDirty)->setLayer(houseParams->tileLayer, {.textureId=signTextureId, .hitmask=HitMask(HitMask::fullMask)});
				}
			}

			return true;
		}

		bool MapGen::addTowns(class Map *map, unsigned x0, unsigned y0, unsigned x1, unsigned y1, const AddTownParameters *params, const AddHouseParameters *houseParams, double totalPopulation) {
			assert(map!=NULL);
			assert(x0<=x1);
			assert(y0<=y1);
			assert(params!=NULL);
			assert(houseParams!=NULL);

			const double peoplePerSqKm=20000.0;

			const double initialTownPop=Util::randFloatInInterval(10.0,20.0)*sqrt(totalPopulation);
			for(double townPop=initialTownPop; townPop>=30; townPop*=Util::randFloatInInterval(0.6,0.8)) {
				const double townSizeSqKm=townPop/peoplePerSqKm;

				const int townSize=1000.0*sqrt(townSizeSqKm);
				const unsigned desiredCount=ceil(initialTownPop/townPop);

				printf("	attempting to add %u towns, each with pop %.0f, size %.3fkm^2 (%'im per side)...", desiredCount, townPop, townSizeSqKm, townSize);
				fflush(stdout);

				const unsigned attemptMax=desiredCount*64;
				unsigned addedCount=0;
				for(unsigned i=0; i<attemptMax && addedCount<desiredCount; ++i) {
					int townX=Util::randIntInInterval(x0+townSize/2, x1-townSize/2);
					int townY=Util::randIntInInterval(y0+townSize/2, y1-townSize/2);
					bool horizontal=Util::randBool();

					if (horizontal) {
						int townX0=townX-townSize/2;
						int townX1=townX+townSize/2;
						if (townX0<0)
							continue;
						addedCount+=MapGen::addTown(map, townX0, townY, townX1, townY, params, houseParams, townPop);
					} else {
						int townY0=townY-townSize/2;
						int townY1=townY+townSize/2;
						if (townY0<0)
							continue;
						addedCount+=MapGen::addTown(map, townX, townY0, townX, townY1, params, houseParams, townPop);
					}
				}

				printf(" managed %u\n", addedCount);
			}

			return true;
		}

		void mapGenRecalculateStatsModifyTilesFunctor(unsigned threadId, class Map *map, unsigned x, unsigned y, void *userData) {
			assert(map!=NULL);
			assert(userData!=NULL);

			MapGenRecalculateStatsThreadData *dataArray=(MapGenRecalculateStatsThreadData *)userData;

			// Grab tile.
			const MapTile *tile=map->getTileAtOffset(x, y, Map::GetTileFlag::None);
			if (tile==NULL)
				return;

			// Update statistics.
			dataArray[threadId].minHeight=std::min(dataArray[threadId].minHeight, tile->getHeight());
			dataArray[threadId].maxHeight=std::max(dataArray[threadId].maxHeight, tile->getHeight());
			dataArray[threadId].minTemperature=std::min(dataArray[threadId].minTemperature, tile->getTemperature());
			dataArray[threadId].maxTemperature=std::max(dataArray[threadId].maxTemperature, tile->getTemperature());
			dataArray[threadId].minMoisture=std::min(dataArray[threadId].minMoisture, tile->getMoisture());
			dataArray[threadId].maxMoisture=std::max(dataArray[threadId].maxMoisture, tile->getMoisture());
		}

		void MapGen::recalculateStats(class Map *map, unsigned x, unsigned y, unsigned width, unsigned height, unsigned threadCount, Gen::ModifyTilesProgress *progressFunctor, void *progressUserData) {
			assert(map!=NULL);

			// Initialize thread data.
			MapGenRecalculateStatsThreadData *threadData=(MapGenRecalculateStatsThreadData *)malloc(sizeof(MapGenRecalculateStatsThreadData)*threadCount);
			for(unsigned i=0; i<threadCount; ++i) {
				threadData[i].minHeight=DBL_MAX;
				threadData[i].maxHeight=DBL_MIN;
				threadData[i].minTemperature=DBL_MAX;
				threadData[i].maxTemperature=DBL_MIN;
				threadData[i].minMoisture=DBL_MAX;
				threadData[i].maxMoisture=DBL_MIN;
			}

			// Use modifyTiles to loop over tiles and update the above fields
			Gen::modifyTiles(map, x, y, width, height, threadCount, &mapGenRecalculateStatsModifyTilesFunctor, threadData, progressFunctor, progressUserData);

			// Combine thread data to obtain final values
			map->minHeight=threadData[0].minHeight;
			map->maxHeight=threadData[0].maxHeight;
			map->minTemperature=threadData[0].minTemperature;
			map->maxTemperature=threadData[0].maxTemperature;
			map->minMoisture=threadData[0].minMoisture;
			map->maxMoisture=threadData[0].maxMoisture;

			for(unsigned i=1; i<threadCount; ++i) {
				map->minHeight=std::min(map->minHeight, threadData[i].minHeight);
				map->maxHeight=std::max(map->maxHeight, threadData[i].maxHeight);
				map->minTemperature=std::min(map->minTemperature, threadData[i].minTemperature);
				map->maxTemperature=std::max(map->maxTemperature, threadData[i].maxTemperature);
				map->minMoisture=std::min(map->minMoisture, threadData[i].minMoisture);
				map->maxMoisture=std::max(map->maxMoisture, threadData[i].maxMoisture);
			}
		}
	};
};
