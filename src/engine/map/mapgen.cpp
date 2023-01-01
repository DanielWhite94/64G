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

		void MapGen::ParticleFlow::dropParticles(unsigned x0, unsigned y0, unsigned x1, unsigned y1, double coverage, unsigned threadCount, Gen::ModifyTilesProgress *progressFunctor, void *progressUserData) {
			assert(map!=NULL);
			assert(x0<=x1);
			assert(y0<=y1);
			assert(coverage>=0.0);

			// TODO: Fix the distribution and calculations if the given area is not a multiple of the region size (in either direction).

			// Record start time.
			Util::TimeMs startTimeMs=Util::getTimeMs();

			// Run progress functor initially (if needed).
			Util::TimeMs lastProgressTimeMs=0;
			if (progressFunctor!=NULL) {
				Util::TimeMs currTimeMs=Util::getTimeMs();
				progressFunctor(map, 0.0, currTimeMs-startTimeMs, progressUserData);
				lastProgressTimeMs=Util::getTimeMs();
			}

			// Compute region loop bounds.
			unsigned rX0=x0/MapRegion::tilesSize;
			unsigned rY0=y0/MapRegion::tilesSize;
			unsigned rX1=x1/MapRegion::tilesSize;
			unsigned rY1=y1/MapRegion::tilesSize;

			// Generate list of regions.
			struct RegionPos { unsigned x, y; } regionPos;
			std::vector<RegionPos> regionList;
			for(regionPos.y=rY0; regionPos.y<rY1; ++regionPos.y)
				for(regionPos.x=rX0; regionPos.x<rX1; ++regionPos.x)
					regionList.push_back(regionPos);

			// Shuffle list.
			auto rng=std::default_random_engine{};
			std::shuffle(std::begin(regionList), std::end(regionList), rng);

			// Calculate constants
			double trialsPerRegion=coverage*MapRegion::tilesSize*MapRegion::tilesSize;

			// Loop over regions (this saves unnecessary loading and saving of regions compared to picking random locations across the whole area given).
			unsigned rI=0;
			for(auto const& regionPos: regionList) {
				// Compute number of trials to perform for this region (may be 0).
				unsigned trials=floor(trialsPerRegion)+(trialsPerRegion>Util::randFloatInInterval(0.0, 1.0) ? 1 : 0);

				// Run said number of trials.
				unsigned tileOffsetBaseX=regionPos.x*MapRegion::tilesSize;
				unsigned tileOffsetBaseY=regionPos.y*MapRegion::tilesSize;
				for(unsigned i=0; i<trials; ++i) {
					// Compute random tile position within this region.
					double randX=Util::randFloatInInterval(0.0, MapRegion::tilesSize);
					double randY=Util::randFloatInInterval(0.0, MapRegion::tilesSize);

					double tileX=tileOffsetBaseX+randX;
					double tileY=tileOffsetBaseY+randY;

					// Grab tile.
					const MapTile *tile=map->getTileAtOffset((int)floor(tileX), (int)floor(tileY), Map::Map::GetTileFlag::None);
					if (tile==NULL)
						continue;

					// Drop particle.
					dropParticle(tileX, tileY);

					// Call progress functor (if needed).
					if (progressFunctor!=NULL) {
						Util::TimeMs currTimeMs=Util::getTimeMs();
						if (currTimeMs-lastProgressTimeMs>500) {
							double progress=(rI+((double)i)/trials)/regionList.size();
							progressFunctor(map, progress, currTimeMs-startTimeMs, progressUserData);
							lastProgressTimeMs=currTimeMs;
						}
					}
				}

				// Call progress functor (if needed).
				if (progressFunctor!=NULL) {
					double progress=(rI+1.0)/regionList.size();
					Util::TimeMs elapsedTimeMs=Util::getTimeMs()-startTimeMs;
					progressFunctor(map, progress, elapsedTimeMs, progressUserData);
				}

				++rI;
			}
		}

		void MapGen::ParticleFlow::dropParticle(double xp, double yp) {
			const double Kq=20, Kw=0.0005, Kr=0.95, Kd=0.02, Ki=0.1/4.0, minSlope=0.03, Kg=20*2;

			#define DEPOSIT(H) \
				depositAt(xi  , yi  , (1-xf)*(1-yf), ds); \
				depositAt(xi+1, yi  ,    xf *(1-yf), ds); \
				depositAt(xi  , yi+1, (1-xf)*   yf , ds); \
				depositAt(xi+1, yi+1,    xf *   yf , ds); \
				(H)+=ds;

			int xi=floor(xp);
			int yi=floor(yp);
			double xf=xp-xi;
			double yf=yp-yi;

			double dx=0.0, dy=0.0;
			double s=0.0, v=0.0, w=1.0;

			double h=hMap(xi, yi, DBL_MIN);

			double h00=h;
			double h01=hMap(xi, yi+1, DBL_MIN);
			double h10=hMap(xi+1, yi, DBL_MIN);
			double h11=hMap(xi+1, yi+1, DBL_MIN);

			if (h<seaLevelExcess)
				return;

			if (h00==DBL_MIN || h01==DBL_MIN || h10==DBL_MIN || h11==DBL_MIN)
				return; // TODO: think about this

			int maxPathLen=4.0*(MapRegion::tilesSize+MapRegion::tilesSize);
			for(int pathLen=0; pathLen<maxPathLen; ++pathLen) {
				// Increment moisture counter for the current tile.
				if (incMoisture) {
					// Calculate normalised x and y values for the tile
					int tempX=xi, tempY=yi;
					tempX=(tempX+map->getWidth())%map->getWidth();
					tempY=(tempY+map->getHeight())%map->getHeight();

					assert(tempX>=0 && tempX<map->getWidth());
					assert(tempY>=0 && tempY<map->getHeight());

					// Lookup tile
					MapTile *tempTile=map->getTileAtOffset(tempX, tempY, Map::GetTileFlag::None);
					if (tempTile!=NULL)
						tempTile->setMoisture(tempTile->getMoisture()+w);
				}

				// calc gradient
				double gx=h00+h01-h10-h11;
				double gy=h00+h10-h01-h11;

				// calc next pos
				dx=(dx-gx)*Ki+gx;
				dy=(dy-gy)*Ki+gy;

				double dl=sqrt(dx*dx+dy*dy);
				if (dl<=0.01) {
					// pick random dir
					double a=Util::randFloatInInterval(0.0, 2*M_PI);
					dx=cos(a);
					dy=sin(a);
				} else {
					dx/=dl;
					dy/=dl;
				}

				double nxp=std::fmod(xp+map->getWidth()+dx, map->getWidth());
				double nyp=std::fmod(yp+map->getHeight()+dy, map->getHeight());

				assert(nxp>=0.0 && nxp<map->getWidth());
				assert(nyp>=0.0 && nyp<map->getHeight());

				// sample next height
				int nxi=floor(nxp);
				int nyi=floor(nyp);
				double nxf=nxp-nxi;
				double nyf=nyp-nyi;

				double nh00=hMap(nxi  , nyi  , DBL_MIN);
				double nh10=hMap(nxi+1, nyi  , DBL_MIN);
				double nh01=hMap(nxi  , nyi+1, DBL_MIN);
				double nh11=hMap(nxi+1, nyi+1, DBL_MIN);

				if (nh00==DBL_MIN || nh01==DBL_MIN || nh10==DBL_MIN || nh11==DBL_MIN)
					return; // TODO: think about this

				double nh=(nh00*(1-nxf)+nh10*nxf)*(1-nyf)+(nh01*(1-nxf)+nh11*nxf)*nyf;

				if (nh<seaLevelExcess)
					return;

				// if higher than current, try to deposit sediment up to neighbour height
				if (nh>=h) {
					double ds=(nh-h)+0.001f;

					if (ds>=s) {
						// deposit all sediment and stop
						ds=s;
						DEPOSIT(h)
						s=0;
						break;
					}

					DEPOSIT(h)
					s-=ds;
					v=0;
				}

				// compute transport capacity
				double dh=h-nh;
				double slope=dh;

				double q=std::max(slope, minSlope)*v*w*Kq;

				// deposit/erode (don't erode more than dh)
				double ds=s-q;
				if (ds>=0) {
					// deposit
					ds*=Kd;

					DEPOSIT(dh)
					s-=ds;
				} else {
					// erode
					ds*=-Kr;
					ds=std::min(ds, dh*0.99);

					double distfactor=1.0/(erodeRadius*erodeRadius);
					double wTotal=0.0;
					for (int y=yi-erodeRadius+1; y<=yi+erodeRadius; ++y) {
						double yo=y-yp;
						double yo2=yo*yo;
						for (int x=xi-erodeRadius+1; x<=xi+erodeRadius; ++x) {
							double xo=x-xp;
							double xo2=xo*xo;

							double w=1-(xo2+yo2)*distfactor;
							if (w<=0)
								continue;

							wTotal+=w;
						}
					}

					for (int y=yi-erodeRadius+1; y<=yi+erodeRadius; ++y) {
						double yo=y-yp;
						double yo2=yo*yo;
						for (int x=xi-erodeRadius+1; x<=xi+erodeRadius; ++x) {
							double xo=x-xp;
							double xo2=xo*xo;

							double w=1-(xo2+yo2)*distfactor;
							if (w<=0)
								continue;

							w/=wTotal;

							depositAt(x, y, -w, ds);
						}
					}

					dh-=ds;
					s+=ds;
				}

				// move to the neighbour
				v=sqrt(v*v+Kg*dh);
				w*=1-Kw;

				xp=nxp; yp=nyp;
				xi=nxi; yi=nyi;
				xf=nxf; yf=nyf;

				h=nh;
				h00=nh00;
				h10=nh10;
				h01=nh01;
				h11=nh11;
			}

			#undef DEPOSIT
		}

		double MapGen::ParticleFlow::hMap(int x, int y, double unknownValue) {
			// To simplify calls to this function, allow x and y values in the interval [-mapwidth,mapwidth]
			x=(x+map->getWidth())%map->getWidth();
			y=(y+map->getHeight())%map->getHeight();

			assert(x>=0 && x<map->getWidth());
			assert(y>=0 && y<map->getHeight());

			// Grab tile.
			const MapTile *tile=map->getTileAtOffset(x, y, Map::Map::GetTileFlag::None);
			if (tile==NULL)
				return unknownValue;

			// Return height.
			return tile->getHeight();
		}

		void MapGen::ParticleFlow::depositAt(int x, int y, double w, double ds) {
			// To simplify calls to this function, allow x and y values in the interval [-mapwidth,mapwidth]
			x=(x+map->getWidth())%map->getWidth();
			y=(y+map->getHeight())%map->getHeight();

			assert(x>=0 && x<map->getWidth());
			assert(y>=0 && y<map->getHeight());

			// Grab tile.
			MapTile *tile=map->getTileAtOffset(x, y, Map::GetTileFlag::Dirty);
			if (tile==NULL)
				return;

			// Adjust height.
			double delta=ds*w;
			tile->setHeight(tile->getHeight()+delta);
		}

		MapGen::MapGen() {
		}

		MapGen::~MapGen() {
		};

		void MapGen::addForest(class Map *map, const CoordVec &topLeft, const CoordVec &widthHeight, const CoordVec &interval, AddForestFunctor *functor, void *functorUserData) {
			assert(map!=NULL);
			assert(widthHeight.x>=0 && widthHeight.y>=0);
			assert(interval.x>0 && interval.y>0);
			assert(functor!=NULL);

			// Loop over rectangular region.
			CoordVec pos;
			for(pos.y=topLeft.y; pos.y<topLeft.y+widthHeight.y; pos.y+=interval.y)
				for(pos.x=topLeft.x; pos.x<topLeft.x+widthHeight.x; pos.x+=interval.x) {
					// Attempt to place object in this region.
					unsigned i;
					for(i=0; i<4; ++i) {
						// Calculate exact position.
						CoordVec randomOffset=CoordVec(Util::randIntInInterval(0,interval.x), Util::randIntInInterval(0,interval.y));
						CoordVec exactPosition=pos+randomOffset;

						// Run functor.
						if (functor(map, exactPosition, functorUserData))
							break;
					}
				}
		}

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
