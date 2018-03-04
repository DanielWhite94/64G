#include <algorithm>
#include <cassert>
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "mapgen.h"
#include "../physics/coord.h"
#include "../util.h"

using namespace Engine;

namespace Engine {
	namespace Map {
		void MapGen::RiverGen::dropParticles(unsigned x0, unsigned y0, unsigned x1, unsigned y1, double coverage, ModifyTilesProgress *progressFunctor, void *progressUserData) {
			assert(map!=NULL);
			assert(x0<=x1);
			assert(y0<=y1);
			assert(coverage>=0.0);

			// TODO: Fix the distribution and calculations if the given area is not a multiple of the region size (in either direction).

			// Record start time.
			Util::TimeMs startTime=Util::getTimeMs();

			// Run progress functor initially (if needed).
			if (progressFunctor!=NULL) {
				Util::TimeMs elapsedTimeMs=Util::getTimeMs()-startTime;
				progressFunctor(map, 0.0, elapsedTimeMs, progressUserData);
			}

			// Compute region loop bounds.
			unsigned rX0=x0/MapRegion::tilesWide;
			unsigned rY0=y0/MapRegion::tilesHigh;
			unsigned rX1=x1/MapRegion::tilesWide;
			unsigned rY1=y1/MapRegion::tilesHigh;

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
			double trialsPerRegion=coverage*MapRegion::tilesWide*MapRegion::tilesHigh;

			// Loop over regions (this saves unnecessary loading and saving of regions compared to picking random locations across the whole area given).
			unsigned rI=0;
			for(auto const& regionPos: regionList) {
				// Compute number of trials to perform for this region (may be 0).
				unsigned trials=floor(trialsPerRegion)+(trialsPerRegion>Util::randFloatInInterval(0.0, 1.0) ? 1 : 0);

				// Runs said number of trials.
				unsigned tileOffsetBaseX=regionPos.x*MapRegion::tilesWide;
				unsigned tileOffsetBaseY=regionPos.y*MapRegion::tilesHigh;
				for(unsigned i=0; i<trials; ++i) {
					// Compute random tile position within this region.
					double randX=Util::randFloatInInterval(0.0, MapRegion::tilesWide);
					double randY=Util::randFloatInInterval(0.0, MapRegion::tilesHigh);

					double tileX=tileOffsetBaseX+randX;
					double tileY=tileOffsetBaseY+randY;

					// Grab tile.
					const MapTile *tile=map->getTileAtOffset((int)floor(tileX), (int)floor(tileY), Map::Map::GetTileFlag::None);
					if (tile==NULL)
						continue;

					// Ocean tile?
					if (tile->getHeight()<seaLevel)
						continue;

					// Drop particle.
					dropParticle(tileX, tileY);
				}

				// Call progress functor (if needed).
				if (progressFunctor!=NULL) {
					double progress=(rI+1.0)/regionList.size();
					Util::TimeMs elapsedTimeMs=Util::getTimeMs()-startTime;
					progressFunctor(map, progress, elapsedTimeMs, progressUserData);
				}

				++rI;
			}
		}

		void MapGen::RiverGen::dropParticle(double xp, double yp) {
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

			if (h<0.0)
				return; // TODO: Use proper sea level?

			if (h00==DBL_MIN || h01==DBL_MIN || h10==DBL_MIN || h11==DBL_MIN)
				return; // TODO: think about this

			int maxPathLen=4.0*(MapRegion::tilesWide+MapRegion::tilesHigh);
			for(int pathLen=0; pathLen<maxPathLen; ++pathLen) {
				// Increment moisture counter for the current tile.
				MapTile *tempTile=map->getTileAtOffset(xi, yi, Map::GetTileFlag::None);
				if (tempTile!=NULL)
					tempTile->setMoisture(tempTile->getMoisture()+w);

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

				double nxp=xp+dx;
				double nyp=yp+dy;

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

				if (nh<0.0)
					return; // TODO: Use proper sea level?

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

					for (int y=yi-1; y<=yi+2; ++y) {
						double yo=y-yp;
						double yo2=yo*yo;

						for (int x=xi-1; x<=xi+2; ++x) {
							double xo=x-xp;

							double w=1-(xo*xo+yo2)*0.25f;
							if (w<=0)
								continue;
							w*=0.1591549430918953f;

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

		double MapGen::RiverGen::hMap(int x, int y, double unknownValue) {
			// Check bounds.
			if (x<0 || x>=Map::regionsWide*MapRegion::tilesWide)
				return unknownValue;
			if (y<0 || y>=Map::regionsHigh*MapRegion::tilesHigh)
				return unknownValue;

			// Grab tile.
			const MapTile *tile=map->getTileAtOffset(x, y, Map::Map::GetTileFlag::None);
			if (tile==NULL)
				return unknownValue;

			// Return height.
			return tile->getHeight();
		}

		void MapGen::RiverGen::depositAt(int x, int y, double w, double ds) {
			// Check bounds.
			if (x<0 || x>=Map::regionsWide*MapRegion::tilesWide)
				return;
			if (y<0 || y>=Map::regionsHigh*MapRegion::tilesHigh)
				return;

			// Grab tile.
			MapTile *tile=map->getTileAtOffset(x, y, Map::GetTileFlag::Dirty);
			if (tile==NULL)
				return;

			// Adjust height.
			double delta=ds*w;
			tile->setHeight(tile->getHeight()+delta);
		}

		void mapGenGenerateBinaryNoiseModifyTilesFunctor(class Map *map, unsigned x, unsigned y, void *userData) {
			assert(map!=NULL);
			assert(userData!=NULL);

			const MapGen::GenerateBinaryNoiseModifyTilesData *data=(const MapGen::GenerateBinaryNoiseModifyTilesData *)userData;

			// Calculate height.
			double height=data->noiseArray->eval(x, y);

			// Update tile layer.
			MapTile *tile=map->getTileAtOffset(x, y, Map::Map::GetTileFlag::CreateDirty);
			if (tile!=NULL) {
				MapTile::Layer layer={.textureId=(height>=data->threshold ? data->highTextureId : data->lowTextureId)};
				tile->setLayer(data->tileLayer, layer);
			}
		}

		void mapGenModifyTilesProgressString(class Map *map, double progress, Util::TimeMs elapsedTimeMs, void *userData) {
			assert(map!=NULL);
			assert(progress>=0.0 && progress<=1.0);
			assert(userData!=NULL);

			const char *string=(const char *)userData;

			// Clear old line.
			Util::clearConsoleLine();

			// Print start of new line, including users message and the percentage complete.
			printf("%s%.1f%% ", string, progress*100.0);

			// Append time elapsed so far.
			Util::TimeMs remainder=elapsedTimeMs/1000;
			if (remainder>24*60*60) {
				Util::TimeMs days=remainder/24*60*60;
				remainder-=days*24*60*60;
				printf("%llud", days);
			}
			if (remainder>60*60) {
				Util::TimeMs hours=remainder/60*60;
				remainder-=hours*60*60;
				printf("%lluh", hours);
			}
			if (remainder>60) {
				Util::TimeMs minutes=remainder/60;
				remainder-=minutes*60;
				printf("%llum", minutes);
			}
			Util::TimeMs seconds=remainder;
			printf("%llus", seconds);

			// Attempt to compute estimated total time.
			Util::TimeMs estimatedTimeS;
			if (progress>=0.0001 && progress<=0.9999 && (estimatedTimeS=elapsedTimeMs/(1000.0*progress))<365llu*24llu*60llu*60llu && estimatedTimeS>0 && estimatedTimeS>elapsedTimeMs/1000) {
				printf(" (~");
				Util::TimeMs remainder=estimatedTimeS-elapsedTimeMs/1000;
				if (remainder>24*60*60) {
					Util::TimeMs days=remainder/24*60*60;
					remainder-=days*24*60*60;
					printf("%llud", days);
				}
				if (remainder>60*60) {
					Util::TimeMs hours=remainder/60*60;
					remainder-=hours*60*60;
					printf("%lluh", hours);
				}
				if (remainder>60) {
					Util::TimeMs minutes=remainder/60;
					remainder-=minutes*60;
					printf("%llum", minutes);
				}
				Util::TimeMs seconds=remainder;
				printf("%llus", seconds);

				printf(" remaining)");
			}

			// Flush output manually (as we are not printing a newline).
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
				[TextureIdTree3]="../images/objects/tree3.png",
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
				[TextureIdHotSand]="../images/tiles/hotsand.png",
				[TextureIdSnow]="../images/tiles/snow.png",
				[TextureIdShopCobbler]="../images/tiles/shops/cobbler.png",
				[TextureIdDeepWater]="../images/tiles/deepwater.png",
				[TextureIdHighAlpine]="../images/tiles/highalpine.png",
				[TextureIdLowAlpine]="../images/tiles/lowalpine.png",
			};
			int textureScales[TextureIdNB]={
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
				[TextureIdTree3]=4,
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
				[TextureIdHotSand]=4,
				[TextureIdShopCobbler]=4,
				[TextureIdDeepWater]=4,
				[TextureIdSnow]=4,
				[TextureIdHighAlpine]=4,
				[TextureIdLowAlpine]=4,
			};

			unsigned textureId;
			char heatmapPaths[TextureIdHeatMapRange][128];
			for(textureId=TextureIdHeatMapMin; textureId<TextureIdHeatMapMax; ++textureId) {
				unsigned index=textureId-TextureIdHeatMapMin;
				sprintf(heatmapPaths[index], "../images/tiles/heatmap/%u.png", index);

				textureScales[textureId]=4;
				texturePaths[textureId]=heatmapPaths[index];
			}

			bool success=true;
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
						CoordVec randomOffset=CoordVec(Util::randIntInInterval(0,interval.x), Util::randIntInInterval(0,interval.y));
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

			unsigned doorOffset=Util::randIntInInterval(1, totalW-2);
			unsigned chimneyOffset=Util::randIntInInterval(0, totalW);

			// Call addHouseFull to do most of the work.
			return addHouseFull(map, AddHouseFullFlags::All, baseX, baseY, totalW, totalH, roofHeight, tileLayer, doorOffset, chimneyOffset, testFunctor, testFunctorUserData);
		}

		bool MapGen::addHouseFull(class Map *map, AddHouseFullFlags flags, unsigned baseX, unsigned baseY, unsigned totalW, unsigned totalH, unsigned roofHeight, unsigned tileLayer, unsigned doorXOffset, unsigned chimneyXOffset, TileTestFunctor *testFunctor, void *testFunctorUserData) {
			assert(map!=NULL);

			unsigned tx, ty;

			const int doorW=2;

			// Check arguments are reasonable.
			if (totalW<5 || totalH<5)
				return false;
			if (roofHeight>=totalH-2)
				return false;
			if ((flags & AddHouseFullFlags::ShowDoor) && doorXOffset+doorW>totalW)
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
					map->getTileAtCoordVec(CoordVec((baseX+tx)*Physics::CoordsPerTile, (baseY+totalH-1-ty)*Physics::CoordsPerTile), Map::Map::GetTileFlag::CreateDirty)->setLayer(tileLayer, {.textureId=texture});
				}

			// Add door.
			if (flags & AddHouseFullFlags::ShowDoor) {
				int doorX=doorXOffset+baseX;
				map->getTileAtCoordVec(CoordVec(doorX*Physics::CoordsPerTile, (baseY+totalH-1)*Physics::CoordsPerTile), Map::Map::GetTileFlag::CreateDirty)->setLayer(tileLayer, {.textureId=TextureIdHouseDoorBL});
				map->getTileAtCoordVec(CoordVec((doorX+1)*Physics::CoordsPerTile, (baseY+totalH-1)*Physics::CoordsPerTile), Map::Map::GetTileFlag::CreateDirty)->setLayer(tileLayer, {.textureId=TextureIdHouseDoorBR});
				map->getTileAtCoordVec(CoordVec(doorX*Physics::CoordsPerTile, (baseY+totalH-2)*Physics::CoordsPerTile), Map::Map::GetTileFlag::CreateDirty)->setLayer(tileLayer, {.textureId=TextureIdHouseDoorTL});
				map->getTileAtCoordVec(CoordVec((doorX+1)*Physics::CoordsPerTile, (baseY+totalH-2)*Physics::CoordsPerTile), Map::Map::GetTileFlag::CreateDirty)->setLayer(tileLayer, {.textureId=TextureIdHouseDoorTR});
			}

			// Add main part of roof.
			for(ty=0;ty<roofHeight-1;++ty) // -1 due to ridge tiles added later
				for(tx=0;tx<totalW;++tx)
					map->getTileAtCoordVec(CoordVec((baseX+tx)*Physics::CoordsPerTile, (baseY+1+ty)*Physics::CoordsPerTile), Map::Map::GetTileFlag::CreateDirty)->setLayer(tileLayer, {.textureId=TextureIdHouseRoof});

			// Add roof top ridge.
			for(tx=0;tx<totalW;++tx)
				map->getTileAtCoordVec(CoordVec((baseX+tx)*Physics::CoordsPerTile, baseY*Physics::CoordsPerTile), Map::Map::GetTileFlag::CreateDirty)->setLayer(tileLayer, {.textureId=TextureIdHouseRoofTop});

			// Add chimney.
			if (flags & AddHouseFullFlags::ShowChimney) {
				int chimneyX=chimneyXOffset+baseX;
				map->getTileAtCoordVec(CoordVec(chimneyX*Physics::CoordsPerTile, baseY*Physics::CoordsPerTile), Map::Map::GetTileFlag::CreateDirty)->setLayer(tileLayer, {.textureId=TextureIdHouseChimneyTop});
				map->getTileAtCoordVec(CoordVec(chimneyX*Physics::CoordsPerTile, (baseY+1)*Physics::CoordsPerTile), Map::Map::GetTileFlag::CreateDirty)->setLayer(tileLayer, {.textureId=TextureIdHouseChimney});
			}

			return true;
		}

		bool MapGen::addTown(class Map *map, unsigned x0, unsigned y0, unsigned x1, unsigned y1, unsigned roadTileLayer, unsigned houseTileLayer, int townPop, TileTestFunctor *testFunctor, void *testFunctorUserData) {
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
						map->getTileAtCoordVec(CoordVec(b*Physics::CoordsPerTile, a*Physics::CoordsPerTile), Map::Map::GetTileFlag::CreateDirty)->setLayer(roadTileLayer, {.textureId=(road.width>=3 ? TextureIdBrickPath : TextureIdDirt)});

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

						houseData.doorOffset=Util::randIntInInterval(1, houseData.mapW-2);
						houseData.chimneyOffset=Util::randIntInInterval(0, houseData.mapW);

						// Attempt to add the house.
						if (!addHouseFull(map, houseData.flags, houseData.x, houseData.y, houseData.mapW, houseData.mapH, houseData.roofHeight, houseTileLayer, houseData.doorOffset, houseData.chimneyOffset, NULL, NULL))
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
				if (houseData.flags & AddHouseFullFlags::ShowDoor) {
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
							signTextureId=TextureIdShopCobbler;
						break;
						default:
							signTextureId=TextureIdHouseWall2;
						break;
					}

					// Compute sign position.
					int signOffset;
					if (houseData.doorOffset<houseData.mapW/2)
						signOffset=2;
					else
						signOffset=-1;
					int signX=signOffset+houseData.doorOffset+houseData.x;

					// Add sign.
					map->getTileAtCoordVec(CoordVec(signX*Physics::CoordsPerTile, (houseData.y+houseData.mapH-2)*Physics::CoordsPerTile), Map::Map::GetTileFlag::CreateDirty)->setLayer(houseTileLayer, {.textureId=signTextureId});
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

			const double initialTownPop=Util::randFloatInInterval(10.0,20.0)*sqrt(totalPopulation);
			for(double townPop=initialTownPop; townPop>=30; townPop*=Util::randFloatInInterval(0.6,0.8)) {
				const double townSizeSqKm=townPop/peoplePerSqKm;

				const int townSize=1000.0*sqrt(townSizeSqKm);
				const unsigned desiredCount=ceil(initialTownPop/townPop);
				printf("	attempting to add %u towns, each with pop %.0f, size %.3fkm^2 (%'im per side)\n", desiredCount, townPop, townSizeSqKm, townSize);

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
						addedCount+=MapGen::addTown(map, townX0, townY, townX1, townY, roadTileLayer, houseTileLayer, townPop, testFunctor, testFunctorUserData);
					} else {
						int townY0=townY-townSize/2;
						int townY1=townY+townSize/2;
						if (townY0<0)
							continue;
						addedCount+=MapGen::addTown(map, townX, townY0, townX, townY1, roadTileLayer, houseTileLayer, townPop, testFunctor, testFunctorUserData);
					}
				}

				printf("		added %u\n", addedCount);
			}

			return true;
		}

		void MapGen::modifyTiles(class Map *map, unsigned x, unsigned y, unsigned width, unsigned height, ModifyTilesFunctor *functor, void *functorUserData, ModifyTilesProgress *progressFunctor, void *progressUserData) {
			assert(map!=NULL);
			assert(functor!=NULL);

			MapGen::ModifyTilesManyEntry functorEntry;
			functorEntry.functor=functor,
			functorEntry.userData=functorUserData,

			MapGen::modifyTilesMany(map, x, y, width, height, 1, &functorEntry, progressFunctor, progressUserData);
		}

		void MapGen::modifyTilesMany(class Map *map, unsigned x, unsigned y, unsigned width, unsigned height, size_t functorArrayCount, ModifyTilesManyEntry functorArray[], ModifyTilesProgress *progressFunctor, void *progressUserData) {
			assert(map!=NULL);
			assert(functorArrayCount>0);
			assert(functorArray!=NULL);

			// Record start time.
			const Util::TimeMs startTime=Util::getTimeMs();

			// Calculate constants.
			const unsigned regionX0=x/MapRegion::tilesWide;
			const unsigned regionY0=y/MapRegion::tilesHigh;
			const unsigned regionX1=(x+width)/MapRegion::tilesWide;
			const unsigned regionY1=(y+height)/MapRegion::tilesHigh;
			const unsigned regionIMax=(regionX1-regionX0)*(regionY1-regionY0);

			// Initial progress update (if needed).
			if (progressFunctor!=NULL) {
				Util::TimeMs elapsedTimeMs=Util::getTimeMs()-startTime;
				progressFunctor(map, 0.0, elapsedTimeMs, progressUserData);
			}

			// Loop over each region
			unsigned regionX, regionY, regionI=0;
			for(regionY=regionY0; regionY<regionY1; ++regionY) {
				const unsigned regionYOffset=regionY*MapRegion::tilesHigh;
				for(regionX=regionX0; regionX<regionX1; ++regionX) {
					const unsigned regionXOffset=regionX*MapRegion::tilesWide;

					// Loop over all rows in this region.
					unsigned tileX, tileY;
					for(tileY=0; tileY<MapRegion::tilesHigh; ++tileY) {
						// Loop over all tiles in this row.
						for(tileX=0; tileX<MapRegion::tilesWide; ++tileX) {
							// Loop over functors.
							for(size_t functorId=0; functorId<functorArrayCount; ++functorId)
								functorArray[functorId].functor(map, regionXOffset+tileX, regionYOffset+tileY, functorArray[functorId].userData);
						}
					}

					// Update progress (if needed).
					if (progressFunctor!=NULL) {
						Util::TimeMs elapsedTimeMs=Util::getTimeMs()-startTime;
						double progress=(regionI+1)/((double)regionIMax);
						progressFunctor(map, progress, elapsedTimeMs, progressUserData);
					}

					++regionI;
				}
			}
		}

		void mapGenRecalculateStatsModifyTilesFunctor(class Map *map, unsigned x, unsigned y, void *userData) {
			assert(map!=NULL);
			assert(userData==NULL);

			// Grab tile.
			const MapTile *tile=map->getTileAtOffset(x, y, Map::GetTileFlag::None);
			if (tile==NULL)
				return;

			// Update statistics.
			map->minHeight=std::min(map->minHeight, tile->getHeight());
			map->maxHeight=std::max(map->maxHeight, tile->getHeight());
			map->minMoisture=std::min(map->minMoisture, tile->getMoisture());
			map->maxMoisture=std::max(map->maxMoisture, tile->getMoisture());
		}

		void MapGen::recalculateStats(class Map *map, unsigned x, unsigned y, unsigned width, unsigned height, ModifyTilesProgress *progressFunctor, void *progressUserData) {
			assert(map!=NULL);

			// Initialize stats.
			map->minHeight=DBL_MAX;
			map->maxHeight=DBL_MIN;
			map->minMoisture=DBL_MAX;
			map->maxMoisture=DBL_MIN;

			// Use modifyTiles with a read-only functor.
			modifyTiles(map, x, y, width, height, &mapGenRecalculateStatsModifyTilesFunctor, NULL, progressFunctor, progressUserData);
		}
	};
};
