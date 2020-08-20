#include <algorithm>
#include <cassert>
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <random>

#include "mapgen.h"
#include "../physics/coord.h"
#include "../util.h"

using namespace Engine;

namespace Engine {
	namespace Map {
		struct MapGenEdgeDetectTraceHeightContoursData {
			unsigned contourIndex, contourCount;
			double heightThreshold;

			MapGen::EdgeDetect::ProgressFunctor *functor;
			void *userData;

			Util::TimeMs startTimeMs;
		};

		struct MapGenEdgeDetectTraceClearScratchBitsModifyTilesProgressData {
			MapGen::EdgeDetect::ProgressFunctor *functor;
			void *userData;

			double progressRatio;
		};

		struct MapGenFloodFillFillClearScratchBitModifyTilesProgressData {
			MapGen::FloodFill::ProgressFunctor *functor;
			void *userData;

			double progressRatio;
		};

		void mapGenEdgeDetectTraceHeightContoursProgressFunctor(class Map *map, double progress, Util::TimeMs elapsedTimeMs, void *userData);
		void mapGenEdgeDetectTraceClearScratchBitsModifyTilesProgressFunctor(class Map *map, double progress, Util::TimeMs elapsedTimeMs, void *userData);

		void mapGenFloodFillFillClearScratchBitModifyTilesProgressFunctor(class Map *map, double progress, Util::TimeMs elapsedTimeMs, void *userData);

		void MapGen::ParticleFlow::dropParticles(unsigned x0, unsigned y0, unsigned x1, unsigned y1, double coverage, ModifyTilesProgress *progressFunctor, void *progressUserData) {
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
					MapTile *tempTile=map->getTileAtOffset(xi, yi, Map::GetTileFlag::None);
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
			// Check bounds.
			if (x<0 || x>=Map::regionsSize*MapRegion::tilesSize)
				return unknownValue;
			if (y<0 || y>=Map::regionsSize*MapRegion::tilesSize)
				return unknownValue;

			// Grab tile.
			const MapTile *tile=map->getTileAtOffset(x, y, Map::Map::GetTileFlag::None);
			if (tile==NULL)
				return unknownValue;

			// Return height.
			return tile->getHeight();
		}

		void MapGen::ParticleFlow::depositAt(int x, int y, double w, double ds) {
			// Check bounds.
			if (x<0 || x>=Map::regionsSize*MapRegion::tilesSize)
				return;
			if (y<0 || y>=Map::regionsSize*MapRegion::tilesSize)
				return;

			// Grab tile.
			MapTile *tile=map->getTileAtOffset(x, y, Map::GetTileFlag::Dirty);
			if (tile==NULL)
				return;

			// Adjust height.
			double delta=ds*w;
			tile->setHeight(tile->getHeight()+delta);
		}

		void MapGen::EdgeDetect::trace(SampleFunctor *sampleFunctor, void *sampleUserData, EdgeFunctor *edgeFunctor, void *edgeUserData, ProgressFunctor *progressFunctor, void *progressUserData) {
			assert(sampleFunctor!=NULL);

			// The following is an implementation of the Square Tracing method.

			const double preModifyTilesProgressRatio=0.1; // assume roughly 10% of the time is spent in scratch bit clearing modify tiles call

			unsigned mapWidth=map->getWidth();
			unsigned mapHeight=map->getHeight();

			Util::TimeMs startTimeMs=Util::getTimeMs();
			unsigned long long progressMax=mapWidth*mapHeight;
			unsigned long long progress=0.0;

			// Give a progress update
			if (progressFunctor!=NULL)
				progressFunctor(map, 0.0, Util::getTimeMs()-startTimeMs, progressUserData);

			// Clear scratch bits (these are used to indicate from which directions we have previously entered a tile on, to avoid retracing similar edges)
			MapGenEdgeDetectTraceClearScratchBitsModifyTilesProgressData modifyTileFunctorData;
			modifyTileFunctorData.functor=progressFunctor;
			modifyTileFunctorData.userData=progressUserData;
			modifyTileFunctorData.progressRatio=preModifyTilesProgressRatio;

			uint64_t scratchBitMask=(((uint64_t)1)<<scratchBits[0])|(((uint64_t)1)<<scratchBits[1])|(((uint64_t)1)<<scratchBits[2])|(((uint64_t)1)<<scratchBits[3]);
			modifyTiles(map, 0, 0, mapWidth, mapHeight, &mapGenBitsetIntersectionModifyTilesFunctor, (void *)(uintptr_t)~scratchBitMask, (progressFunctor!=NULL ? &mapGenEdgeDetectTraceClearScratchBitsModifyTilesProgressFunctor : NULL), &modifyTileFunctorData);

			// Loop over regions
			unsigned rYEnd=mapHeight/MapRegion::tilesSize;
			for(unsigned rY=0; rY<rYEnd; ++rY) {
				unsigned rXEnd=mapWidth/MapRegion::tilesSize;
				for(unsigned rX=0; rX<rXEnd; ++rX) {
					// Calculate region tile boundaries
					unsigned tileX0=rX*MapRegion::tilesSize;
					unsigned tileY0=rY*MapRegion::tilesSize;
					unsigned tileX1=std::min(mapWidth, (rX+1)*MapRegion::tilesSize);
					unsigned tileY1=std::min(mapHeight, (rY+1)*MapRegion::tilesSize);

					// Loop over all tiles in this region, looking for starting points to trace from
					int startX, startY;
					for(startY=tileY0; startY<tileY1; ++startY) {
						for(startX=tileX0; startX<tileX1; ++startX) {
							// Give a progress update
							if (progressFunctor!=NULL && progress%256==0)
								progressFunctor(map, preModifyTilesProgressRatio+(1.0-preModifyTilesProgressRatio)*((double)progress)/progressMax, Util::getTimeMs()-startTimeMs, progressUserData);
							++progress;

							// We require an 'inside' tile to start tracing.
							if (!sampleFunctor(map, startX, startY, sampleUserData))
								continue;

							// Setup variables
							int currX=startX;
							int currY=startY;
							int velX=1;
							int velY=0;
							unsigned currDir=DirectionEast;

							// First tile is always 'inside', so turn left
							turnLeft(&velX, &velY, &currDir);

							currX+=velX;
							currY+=velY;
							bool currIsWithinMap=(currX>=0 && currY>=0 && currX<mapWidth && currY<mapHeight);

							// Walk the edge between 'inside' and 'outside' tiles
							// Note: we use Jacob's stopping criterion where we also ensure we return to the start tile with the same velocity as we started
							bool foundOutside=false;
							while(currX!=startX || currY!=startY || velX!=1 || velY!=0) {
								// Has this tile already been handled?
								if (currIsWithinMap) {
									// Check relevant cache bit based on current direction
									MapTile *tile=map->getTileAtOffset(currX, currY, Engine::Map::Map::GetTileFlag::None);
									if (tile!=NULL && tile->getBitsetN(scratchBits[currDir])) {
										// We have already entered this tile in this direction before - no point retracing it again.
										// So simply quit tracing this particular edge and move onto a new starting tile.

										// Reset foundOutside flag as a hack method to avoid calling edge functor for a final (unnecessary) time
										foundOutside=false;

										break;
									}
								}

								// Determine if current tile is 'inside' or 'outside'
								if (currIsWithinMap && sampleFunctor(map, currX, currY, sampleUserData)) {
									// 'inside' tile
									if (edgeFunctor!=NULL && foundOutside) {
										// Mark relevant cache bit
										MapTile *tile=map->getTileAtOffset(currX, currY, Engine::Map::Map::GetTileFlag::Dirty);
										if (tile!=NULL)
											tile->setBitsetN(scratchBits[currDir], true);

										// Call user's edge functor
										if (edgeFunctor!=NULL)
											edgeFunctor(map, currX, currY, edgeUserData);
									}

									turnLeft(&velX, &velY, &currDir);
								} else {
									// 'outside' tile
									foundOutside=true;

									turnRight(&velX, &velY, &currDir);
								}

								// Move to next tile
								currX+=velX;
								currY+=velY;
								currIsWithinMap=(currX>=0 && currY>=0 && currX<mapWidth && currY<mapHeight);
							}

							// Ensure start/end tile is considered as part of the boundary (this is done after the trace so we can compute foundOutside)
							if (edgeFunctor!=NULL && foundOutside && currIsWithinMap) {
								// Mark relevant cache bit
								MapTile *tile=map->getTileAtOffset(currX, currY, Engine::Map::Map::GetTileFlag::Dirty);
								if (tile!=NULL)
									tile->setBitsetN(scratchBits[currDir], true);

								// Call user's edge functor
								if (edgeFunctor!=NULL)
									edgeFunctor(map, currX, currY, edgeUserData);
							}
						}
					}
				}
			}

			// Give a progress update
			if (progressFunctor!=NULL)
				progressFunctor(map, 1.0, Util::getTimeMs()-startTimeMs, progressUserData);
		}

		void MapGen::EdgeDetect::traceHeightContours(int contourCount, MapGen::EdgeDetect::ProgressFunctor *progressFunctor, void *progressUserData) {
			// Check for bad contourCount
			if (contourCount<1)
				return;

			// Create our own wrapper userData for progress functor
			MapGenEdgeDetectTraceHeightContoursData functorData;
			functorData.contourIndex=0;
			functorData.contourCount=contourCount;
			functorData.functor=progressFunctor;
			functorData.userData=progressUserData;
			functorData.startTimeMs=Util::getTimeMs();

			// Run edge detection at various height thresholds to trace all contour lines
			for(functorData.contourIndex=0; functorData.contourIndex<contourCount; ++functorData.contourIndex) {
				functorData.heightThreshold=map->seaLevel+((functorData.contourIndex+1.0)/(contourCount+1))*(map->maxHeight-map->seaLevel);
				trace(&mapGenEdgeDetectHeightThresholdSampleFunctor, &functorData.heightThreshold, &mapGenEdgeDetectBitsetNEdgeFunctor, (void *)(uintptr_t)MapGen::TileBitsetIndexContour, (progressFunctor!=NULL ? &mapGenEdgeDetectTraceHeightContoursProgressFunctor : NULL), (void *)&functorData);
			}
		}

		void MapGen::FloodFill::fill(BoundaryFunctor *boundaryFunctor, void *boundaryUserData, FillFunctor *fillFunctor, void *fillUserData, ProgressFunctor *progressFunctor, void *progressUserData) {
			assert(boundaryFunctor!=NULL);
			assert(fillFunctor!=NULL);

			struct Segment {
				unsigned x0, x1, y;

				Segment(): x0(0), x1(0), y(0) {};
				Segment(unsigned x0, unsigned x1, unsigned y): x0(x0), x1(x1), y(y) {};
				~Segment() {};
			};

			// Initialisation
			const double preModifyTilesProgressRatio=0.1; // assume roughly 10% of the time is spent in scratch bit clearing modify tiles call

			unsigned mapWidth=map->getWidth();
			unsigned mapHeight=map->getHeight();

			Util::TimeMs startTimeMs=Util::getTimeMs();
			unsigned long long progressMax=mapWidth*mapHeight;
			unsigned long long progress=0.0;

			unsigned groupId=0;

			// Give a progress update
			if (progressFunctor!=NULL)
				progressFunctor(map, 0.0, Util::getTimeMs()-startTimeMs, progressUserData);

			// Clear scratch bits (these are used to indicate from which directions we have previously entered a tile on, to avoid retracing similar edges)
			MapGenFloodFillFillClearScratchBitModifyTilesProgressData modifyTileFunctorData;
			modifyTileFunctorData.functor=progressFunctor;
			modifyTileFunctorData.userData=progressUserData;
			modifyTileFunctorData.progressRatio=preModifyTilesProgressRatio;

			uint64_t scratchBitMask=(((uint64_t)1)<<scratchBit);
			modifyTiles(map, 0, 0, mapWidth, mapHeight, &mapGenBitsetIntersectionModifyTilesFunctor, (void *)(uintptr_t)~scratchBitMask, (progressFunctor!=NULL ? &mapGenFloodFillFillClearScratchBitModifyTilesProgressFunctor : NULL), &modifyTileFunctorData);

			// Loop over regions
			unsigned rYEnd=mapHeight/MapRegion::tilesSize;
			for(unsigned rY=0; rY<rYEnd; ++rY) {
				unsigned rXEnd=mapWidth/MapRegion::tilesSize;
				for(unsigned rX=0; rX<rXEnd; ++rX) {
					// Calculate region tile boundaries
					unsigned tileX0=rX*MapRegion::tilesSize;
					unsigned tileY0=rY*MapRegion::tilesSize;
					unsigned tileX1=std::min(mapWidth, (rX+1)*MapRegion::tilesSize);
					unsigned tileY1=std::min(mapHeight, (rY+1)*MapRegion::tilesSize);

					// Loop over all tiles in this region, looking for starting points to trace from
					int startX, startY;
					for(startY=tileY0; startY<tileY1; ++startY) {
						for(startX=tileX0; startX<tileX1; ++startX) {
							// Has this tile already been handled?
							MapTile *tile=map->getTileAtOffset(startX, startY, Map::Map::GetTileFlag::None);
							if (tile==NULL || tile->getBitsetN(scratchBit))
								continue;

							// Indicate we have handled this tile
							tile=map->getTileAtOffset(startX, startY, Map::Map::GetTileFlag::Dirty); // grab again but mark as dirty
							assert(tile!=NULL);
							tile->setBitsetN(scratchBit, true);

							if (progressFunctor!=NULL && progress%256==0)
								progressFunctor(map, preModifyTilesProgressRatio+(1.0-preModifyTilesProgressRatio)*((double)progress)/progressMax, Util::getTimeMs()-startTimeMs, progressUserData);
							++progress;

							// We need a non-boundary tile to start filling - so check now
							if (boundaryFunctor(map, startX, startY, boundaryUserData))
								continue;

							// Add start tile as an initial segment to start filling from
							std::vector<Segment> segments;
							segments.clear();
							segments.push_back(Segment(startX, startX+1, startY));

							// Main loop to handle each segment/scanline
							while(!segments.empty()) {
								// Grab new segment
								Segment segment=segments.back();
								segments.pop_back();

								// Extend segment left until we hit a boundary
								while(1) {
									if (segment.x0==0)
										break;

									if (boundaryFunctor(map, segment.x0-1, segment.y, boundaryUserData))
										break;

									--segment.x0;
								}

								// Extend segment right until we hit a boundary
								while(1) {
									if (segment.x1==mapWidth)
										break;

									if (boundaryFunctor(map, segment.x1, segment.y, boundaryUserData))
										break;

									++segment.x1;
								}

								// Loop over this segment to handle each tile within, and to find new segments to add to the stack
								bool aboveActive=false, belowActive=false;
								Segment aboveSegment, belowSegment;
								aboveSegment.y=segment.y-1;
								belowSegment.y=segment.y+1;
								for(unsigned loopX=segment.x0; loopX<segment.x1; ++loopX) {
									// Ensure scratch bit is set in this tile's bitset to indicate we have handled it
									MapTile *loopTile=map->getTileAtOffset(loopX, segment.y, Map::Map::GetTileFlag::Dirty);
									if (loopTile==NULL)
										continue; // TODO: think about this - shouldn't happen but what happens if it does?

									if (!loopTile->getBitsetN(scratchBit)) {
										// Invoke progress functor if needed
										if (progressFunctor!=NULL && progress%256==0)
											progressFunctor(map, preModifyTilesProgressRatio+(1.0-preModifyTilesProgressRatio)*((double)progress)/progressMax, Util::getTimeMs()-startTimeMs, progressUserData);
										++progress;

										// Set scratch bit to indicate we have handled this tile
										loopTile->setBitsetN(scratchBit, true);
									}

									// Call user's fill functor
									fillFunctor(map, loopX, segment.y, groupId, fillUserData);

									// Check for potential new segment above the current one
									if (segment.y>0) {
										bool aboveIsBoundary=boundaryFunctor(map, loopX, aboveSegment.y, boundaryUserData);
										if (!aboveIsBoundary) {
											MapTile *aboveTile=map->getTileAtOffset(loopX, aboveSegment.y, Map::Map::GetTileFlag::None);
											aboveIsBoundary=(aboveTile!=NULL && aboveTile->getBitsetN(scratchBit));
										}

										if (aboveActive) {
											if (aboveIsBoundary) {
												segments.push_back(aboveSegment);
												aboveActive=false;
											} else
												++aboveSegment.x1;
										} else if (!aboveIsBoundary) {
											aboveActive=true;
											aboveSegment.x0=loopX;
											aboveSegment.x1=loopX+1;
										}
									}

									// Check for potential new segment below the current one
									if (belowSegment.y<mapHeight) {
										bool belowIsBoundary=boundaryFunctor(map, loopX, belowSegment.y, boundaryUserData);
										if (!belowIsBoundary) {
											MapTile *belowTile=map->getTileAtOffset(loopX, belowSegment.y, Map::Map::GetTileFlag::None);
											belowIsBoundary=(belowTile!=NULL && belowTile->getBitsetN(scratchBit));
										}

										if (belowActive) {
											if (belowIsBoundary) {
												segments.push_back(belowSegment);
												belowActive=false;
											} else
												++belowSegment.x1;
										} else if (!belowIsBoundary) {
											belowActive=true;
											belowSegment.x0=loopX;
											belowSegment.x1=loopX+1;
										}
									}
								}

								// Handle any remaining new segments (essentially treating any tiles outside of the map as boundary tiles)
								if (aboveActive)
									segments.push_back(aboveSegment);
								if (belowActive)
									segments.push_back(belowSegment);
							}

							// Prepare for next group we find
							++groupId;
						}
					}
				}
			}

			// Give a progress update
			if (progressFunctor!=NULL)
				progressFunctor(map, 1.0, Util::getTimeMs()-startTimeMs, progressUserData);
		}

		void mapGenGenerateBinaryNoiseModifyTilesFunctor(class Map *map, unsigned x, unsigned y, void *userData) {
			assert(map!=NULL);
			assert(userData!=NULL);

			const MapGen::GenerateBinaryNoiseModifyTilesData *data=(const MapGen::GenerateBinaryNoiseModifyTilesData *)userData;

			// Calculate height.
			double height=data->noiseArray->eval(x, y);

			// Update tile layer.
			MapTile *tile=map->getTileAtOffset(x, y, Map::Map::GetTileFlag::CreateDirty);
			if (tile!=NULL)
				tile->setLayer(data->tileLayer, height>=data->threshold ? data->highLayer : data->lowLayer);
		}

		void mapGenBitsetUnionModifyTilesFunctor(class Map *map, unsigned x, unsigned y, void *userData) {
			assert(map!=NULL);

			uint64_t bitset=(uint64_t)(uintptr_t)userData;

			MapTile *tile=map->getTileAtOffset(x, y, Engine::Map::Map::GetTileFlag::Dirty);
			if (tile!=NULL)
				tile->setBitset(tile->getBitset()|bitset);
		}

		void mapGenBitsetIntersectionModifyTilesFunctor(class Map *map, unsigned x, unsigned y, void *userData) {
			assert(map!=NULL);

			uint64_t bitset=(uint64_t)(uintptr_t)userData;

			MapTile *tile=map->getTileAtOffset(x, y, Engine::Map::Map::GetTileFlag::Dirty);
			if (tile!=NULL)
				tile->setBitset(tile->getBitset()&bitset);
		}

		void mapGenPrintTime(Util::TimeMs timeMs) {
			const Util::TimeMs minuteFactor=60;
			const Util::TimeMs hourFactor=minuteFactor*60;
			const Util::TimeMs dayFactor=hourFactor*24;

			// Convert to seconds.
			timeMs/=1000;

			// Print time.
			bool output=false;

			if (timeMs>=dayFactor) {
				Util::TimeMs days=timeMs/dayFactor;
				timeMs-=days*dayFactor;
				printf("%llud", days);
				output=true;
			}

			if (timeMs>=hourFactor) {
				Util::TimeMs hours=timeMs/hourFactor;
				timeMs-=hours*hourFactor;
				printf("%llih", hours);
				output=true;
			}

			if (timeMs>=minuteFactor) {
				Util::TimeMs minutes=timeMs/minuteFactor;
				timeMs-=minutes*minuteFactor;
				printf("%llim", minutes);
				output=true;
			}

			if (timeMs>0 || !output)
				printf("%llis", timeMs);
		}

		void mapGenModifyTilesProgressString(class Map *map, double progress, Util::TimeMs elapsedTimeMs, void *userData) {
			assert(map!=NULL);
			assert(progress>=0.0 && progress<=1.0);
			assert(userData!=NULL);

			const char *string=(const char *)userData;

			// Clear old line.
			Util::clearConsoleLine();

			// Print start of new line, including users message and the percentage complete.
			printf("%s%.3f%% ", string, progress*100.0);

			// Append time elapsed so far.
			mapGenPrintTime(elapsedTimeMs);

			// Attempt to compute estimated total time.
			if (progress>=0.0001 && progress<=0.9999) {
				Util::TimeMs estRemainingTimeMs=elapsedTimeMs*(1.0/progress-1.0);
				if (estRemainingTimeMs>=1000 && estRemainingTimeMs<365ll*24ll*60ll*60ll*1000ll) {
					printf(" (~");
					mapGenPrintTime(estRemainingTimeMs);
					printf(" remaining)");
				}
			}

			// Flush output manually (as we are not printing a newline).
			fflush(stdout);
		}

		MapGen::MapGen() {
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
				[TextureIdRiver]="../images/tiles/water.png",
				[TextureIdHighAlpine]="../images/tiles/highalpine.png",
				[TextureIdLowAlpine]="../images/tiles/lowalpine.png",
				[TextureIdSheepN]="../images/npcs/sheep/north.png",
				[TextureIdSheepE]="../images/npcs/sheep/east.png",
				[TextureIdSheepS]="../images/npcs/sheep/south.png",
				[TextureIdSheepW]="../images/npcs/sheep/west.png",
				[TextureIdRoseBush]="../images/objects/rosebush.png",
				[TextureIdCoins]="../images/objects/coins.png",
				[TextureIdDog]="../images/npcs/dog/east.png",
				[TextureIdChestClosed]="../images/objects/chestclosed.png",
				[TextureIdChestOpen]="../images/objects/chestopen.png",
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
				[TextureIdRiver]=4,
				[TextureIdSnow]=4,
				[TextureIdHighAlpine]=4,
				[TextureIdLowAlpine]=4,
				[TextureIdSheepN]=8,
				[TextureIdSheepE]=8,
				[TextureIdSheepS]=8,
				[TextureIdSheepW]=8,
				[TextureIdRoseBush]=8,
				[TextureIdCoins]=8,
				[TextureIdDog]=8,
				[TextureIdChestClosed]=4,
				[TextureIdChestOpen]=4,
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
				case BuiltinObject::Sheep: {
					// Create hitmask.
					const char hitmaskStr[64+1]=
						"________"
						"________"
						"_######_"
						"_######_"
						"_######_"
						"_######_"
						"_######_"
						"_######_";
					HitMask hitmask(hitmaskStr);

					// Create object.
					MapObject *object=new MapObject(rotation, pos, 1, 1);
					object->setHitMaskByTileOffset(0, 0, hitmask);
					object->setTextureIdForAngle(CoordAngle0, TextureIdSheepS);
					object->setTextureIdForAngle(CoordAngle90, TextureIdSheepW);
					object->setTextureIdForAngle(CoordAngle180, TextureIdSheepN);
					object->setTextureIdForAngle(CoordAngle270, TextureIdSheepE);

					// Add object to map.
					if (!map->addObject(object)) {
						delete object;
						return NULL;
					}

					return object;
				} break;
				case BuiltinObject::Dog: {
					// Create hitmask.
					const char hitmaskStr[64+1]=
						"________"
						"________"
						"________"
						"_#####__"
						"_#####__"
						"_#####__"
						"___###__"
						"________";
					HitMask hitmask(hitmaskStr);

					// Create object.
					MapObject *object=new MapObject(rotation, pos, 1, 1);
					object->setHitMaskByTileOffset(0, 0, hitmask);
					object->setTextureIdForAngle(CoordAngle0, TextureIdDog);
					object->setTextureIdForAngle(CoordAngle90, TextureIdDog);
					object->setTextureIdForAngle(CoordAngle180, TextureIdDog);
					object->setTextureIdForAngle(CoordAngle270, TextureIdDog);

					// Add object to map.
					if (!map->addObject(object)) {
						delete object;
						return NULL;
					}

					return object;
				} break;
				case BuiltinObject::Chest:
					// Create hitmask.
					HitMask hitmask(HitMask::fullMask);

					// Create object.
					MapObject *object=new MapObject(rotation, pos, 1, 1);

					object->setHitMaskByTileOffset(0, 0, hitmask);

					object->setTextureIdForAngle(CoordAngle0, TextureIdChestClosed);
					object->setTextureIdForAngle(CoordAngle90, TextureIdChestClosed);
					object->setTextureIdForAngle(CoordAngle180, TextureIdChestClosed);
					object->setTextureIdForAngle(CoordAngle270, TextureIdChestClosed);

					object->setItemData(mapObjectItemTypeChest, 1);

					object->inventoryEmpty(24);
					MapObjectItem item={.type=mapObjectItemTypeCoins, .count=(MapObjectItemCount)(5+rand()%5)};
					object->inventoryAddItem(item);

					// Add object to map.
					if (!map->addObject(object)) {
						delete object;
						return NULL;
					}

					return object;
				break;
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

		bool MapGen::addHouse(class Map *map, unsigned baseX, unsigned baseY, unsigned totalW, unsigned totalH, unsigned tileLayer, unsigned decorationLayer, bool showDoor, TileTestFunctor *testFunctor, void *testFunctorUserData) {
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
			return addHouseFull(map, AddHouseFullFlags::All, baseX, baseY, totalW, totalH, roofHeight, tileLayer, decorationLayer, doorOffset, chimneyOffset, testFunctor, testFunctorUserData);
		}

		bool MapGen::addHouseFull(class Map *map, AddHouseFullFlags flags, unsigned baseX, unsigned baseY, unsigned totalW, unsigned totalH, unsigned roofHeight, unsigned tileLayer, unsigned decorationLayer, unsigned doorXOffset, unsigned chimneyXOffset, TileTestFunctor *testFunctor, void *testFunctorUserData) {
			assert(map!=NULL);

			unsigned tx, ty;

			const int doorW=(flags & AddHouseFullFlags::ShowDoor) ? 2 : 0;

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
					map->getTileAtCoordVec(CoordVec((baseX+tx)*Physics::CoordsPerTile, (baseY+totalH-1-ty)*Physics::CoordsPerTile), Map::Map::GetTileFlag::CreateDirty)->setLayer(tileLayer, {.textureId=texture, .hitmask=HitMask(HitMask::fullMask)});
				}

			// Add door.
			if (flags & AddHouseFullFlags::ShowDoor) {
				int doorX=doorXOffset+baseX;
				map->getTileAtCoordVec(CoordVec(doorX*Physics::CoordsPerTile, (baseY+totalH-1)*Physics::CoordsPerTile), Map::Map::GetTileFlag::CreateDirty)->setLayer(tileLayer, {.textureId=TextureIdHouseDoorBL, .hitmask=HitMask(HitMask::fullMask)});
				map->getTileAtCoordVec(CoordVec((doorX+1)*Physics::CoordsPerTile, (baseY+totalH-1)*Physics::CoordsPerTile), Map::Map::GetTileFlag::CreateDirty)->setLayer(tileLayer, {.textureId=TextureIdHouseDoorBR, .hitmask=HitMask(HitMask::fullMask)});
				map->getTileAtCoordVec(CoordVec(doorX*Physics::CoordsPerTile, (baseY+totalH-2)*Physics::CoordsPerTile), Map::Map::GetTileFlag::CreateDirty)->setLayer(tileLayer, {.textureId=TextureIdHouseDoorTL, .hitmask=HitMask(HitMask::fullMask)});
				map->getTileAtCoordVec(CoordVec((doorX+1)*Physics::CoordsPerTile, (baseY+totalH-2)*Physics::CoordsPerTile), Map::Map::GetTileFlag::CreateDirty)->setLayer(tileLayer, {.textureId=TextureIdHouseDoorTR, .hitmask=HitMask(HitMask::fullMask)});
			}

			// Add main part of roof.
			for(ty=0;ty<roofHeight-1;++ty) // -1 due to ridge tiles added later
				for(tx=0;tx<totalW;++tx)
					map->getTileAtCoordVec(CoordVec((baseX+tx)*Physics::CoordsPerTile, (baseY+1+ty)*Physics::CoordsPerTile), Map::Map::GetTileFlag::CreateDirty)->setLayer(tileLayer, {.textureId=TextureIdHouseRoof, .hitmask=HitMask(HitMask::fullMask)});

			// Add roof top ridge.
			for(tx=0;tx<totalW;++tx)
				map->getTileAtCoordVec(CoordVec((baseX+tx)*Physics::CoordsPerTile, baseY*Physics::CoordsPerTile), Map::Map::GetTileFlag::CreateDirty)->setLayer(tileLayer, {.textureId=TextureIdHouseRoofTop, .hitmask=HitMask(HitMask::fullMask)});

			// Add chimney.
			if (flags & AddHouseFullFlags::ShowChimney) {
				int chimneyX=chimneyXOffset+baseX;
				map->getTileAtCoordVec(CoordVec(chimneyX*Physics::CoordsPerTile, baseY*Physics::CoordsPerTile), Map::Map::GetTileFlag::CreateDirty)->setLayer(tileLayer, {.textureId=TextureIdHouseChimneyTop, .hitmask=HitMask(HitMask::fullMask)});
				map->getTileAtCoordVec(CoordVec(chimneyX*Physics::CoordsPerTile, (baseY+1)*Physics::CoordsPerTile), Map::Map::GetTileFlag::CreateDirty)->setLayer(tileLayer, {.textureId=TextureIdHouseChimney, .hitmask=HitMask(HitMask::fullMask)});
			}

			// Add some decoration.
			if ((flags & AddHouseFullFlags::AddDecoration)) {
				// Add path infront of door (if any).
				if (flags & AddHouseFullFlags::ShowDoor) {
					for(int offset=0; offset<doorW; ++offset) {
						int posX=baseX+doorXOffset+offset;
						map->getTileAtCoordVec(CoordVec(posX*Physics::CoordsPerTile, (baseY+totalH)*Physics::CoordsPerTile), Map::Map::GetTileFlag::CreateDirty)->setLayer(tileLayer, {.textureId=MapGen::TextureIdBrickPath, .hitmask=HitMask()});
					}
				}

				// Random chance of adding a rose bush at random position (avoiding the door, if any).
				if (Util::randIntInInterval(0, 4)==0) {
					int offset=Util::randIntInInterval(0, totalW-doorW);
					if ((flags & AddHouseFullFlags::ShowDoor) && offset>=doorXOffset)
						offset+=doorW;
					assert(offset>=0 && offset<totalW);

					int posX=offset+baseX;
					map->getTileAtCoordVec(CoordVec(posX*Physics::CoordsPerTile, (baseY+totalH)*Physics::CoordsPerTile), Map::Map::GetTileFlag::CreateDirty)->setLayer(decorationLayer, {.textureId=MapGen::TextureIdRoseBush, .hitmask=HitMask()});
				}
			}

			return true;
		}

		bool MapGen::addTown(class Map *map, unsigned x0, unsigned y0, unsigned x1, unsigned y1, unsigned roadTileLayer, unsigned houseTileLayer, unsigned houseDecorationLayer, int townPop, TileTestFunctor *testFunctor, void *testFunctorUserData) {
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
						map->getTileAtCoordVec(CoordVec(b*Physics::CoordsPerTile, a*Physics::CoordsPerTile), Map::Map::GetTileFlag::CreateDirty)->setLayer(roadTileLayer, {.textureId=(road.width>=3 ? TextureIdBrickPath : TextureIdDirt), .hitmask=HitMask()});

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
							if (!testFunctor(map, houseData.x-1, houseData.y, houseData.mapW+2, houseData.mapH, testFunctorUserData))
								continue;
						} else {
							if (!testFunctor(map, houseData.x, houseData.y-1, houseData.mapW, houseData.mapH+2, testFunctorUserData))
								continue;
						}

						// Compute house parameters.
						bool showDoor=(road.isHorizontal() && !houseData.side);
						houseData.flags=(AddHouseFullFlags)(AddHouseFullFlags::AddDecoration|AddHouseFullFlags::ShowChimney);
						if (showDoor)
							houseData.flags=(AddHouseFullFlags)(houseData.flags|AddHouseFullFlags::ShowDoor);

						const double houseRoofRatio=0.6;
						houseData.roofHeight=(int)floor(houseRoofRatio*houseData.mapH);

						houseData.doorOffset=Util::randIntInInterval(1, houseData.mapW-2);
						houseData.chimneyOffset=Util::randIntInInterval(0, houseData.mapW);

						// Attempt to add the house.
						if (!addHouseFull(map, houseData.flags, houseData.x, houseData.y, houseData.mapW, houseData.mapH, houseData.roofHeight, houseTileLayer, houseDecorationLayer, houseData.doorOffset, houseData.chimneyOffset, NULL, NULL))
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
					map->getTileAtCoordVec(CoordVec(signX*Physics::CoordsPerTile, (houseData.y+houseData.mapH-2)*Physics::CoordsPerTile), Map::Map::GetTileFlag::CreateDirty)->setLayer(houseTileLayer, {.textureId=signTextureId, .hitmask=HitMask(HitMask::fullMask)});
				}
			}

			return true;
		}

		bool MapGen::addTowns(class Map *map, unsigned x0, unsigned y0, unsigned x1, unsigned y1, unsigned roadTileLayer, unsigned houseTileLayer, unsigned houseDecorationLayer, double totalPopulation, MapGen::TileTestFunctor *testFunctor, void *testFunctorUserData) {
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
						addedCount+=MapGen::addTown(map, townX0, townY, townX1, townY, roadTileLayer, houseTileLayer, houseDecorationLayer, townPop, testFunctor, testFunctorUserData);
					} else {
						int townY0=townY-townSize/2;
						int townY1=townY+townSize/2;
						if (townY0<0)
							continue;
						addedCount+=MapGen::addTown(map, townX, townY0, townX, townY1, roadTileLayer, houseTileLayer, houseDecorationLayer, townPop, testFunctor, testFunctorUserData);
					}
				}

				printf(" managed %u\n", addedCount);
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
			const unsigned regionX0=x/MapRegion::tilesSize;
			const unsigned regionY0=y/MapRegion::tilesSize;
			const unsigned regionX1=(x+width)/MapRegion::tilesSize;
			const unsigned regionY1=(y+height)/MapRegion::tilesSize;
			const unsigned regionIMax=(regionX1-regionX0)*(regionY1-regionY0);

			// Initial progress update (if needed).
			if (progressFunctor!=NULL) {
				Util::TimeMs elapsedTimeMs=Util::getTimeMs()-startTime;
				progressFunctor(map, 0.0, elapsedTimeMs, progressUserData);
			}

			// Loop over each region
			unsigned regionX, regionY, regionI=0;
			for(regionY=regionY0; regionY<regionY1; ++regionY) {
				const unsigned regionYOffset=regionY*MapRegion::tilesSize;
				for(regionX=regionX0; regionX<regionX1; ++regionX) {
					const unsigned regionXOffset=regionX*MapRegion::tilesSize;

					// Loop over all rows in this region.
					unsigned tileX, tileY;
					for(tileY=0; tileY<MapRegion::tilesSize; ++tileY) {
						// Loop over all tiles in this row.
						for(tileX=0; tileX<MapRegion::tilesSize; ++tileX) {
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
			map->minTemperature=std::min(map->minTemperature, tile->getTemperature());
			map->maxTemperature=std::max(map->maxTemperature, tile->getTemperature());
			map->minMoisture=std::min(map->minMoisture, tile->getMoisture());
			map->maxMoisture=std::max(map->maxMoisture, tile->getMoisture());
		}

		void MapGen::recalculateStats(class Map *map, unsigned x, unsigned y, unsigned width, unsigned height, ModifyTilesProgress *progressFunctor, void *progressUserData) {
			assert(map!=NULL);

			// Initialize stats.
			map->minHeight=DBL_MAX;
			map->maxHeight=DBL_MIN;
			map->minTemperature=DBL_MAX;
			map->maxTemperature=DBL_MIN;
			map->minMoisture=DBL_MAX;
			map->maxMoisture=DBL_MIN;

			// Use modifyTiles with a read-only functor.
			modifyTiles(map, x, y, width, height, &mapGenRecalculateStatsModifyTilesFunctor, NULL, progressFunctor, progressUserData);
		}

		double MapGen::narySearch(class Map *map, unsigned x, unsigned y, unsigned width, unsigned height, int n, double threshold, double epsilon, double sampleMin, double sampleMax, NArySearchGetFunctor *getFunctor, void *getUserData) {
			assert(map!=NULL);
			assert(n>0);
			assert(0.0<=threshold<=1.0);
			assert(getFunctor!=NULL);

			// Calculate expected iterMax and use it just in case we cannot narrow down the range for some reason.
			// This calculation is based on the idea that each iteration reduces the range by #samples+1..
			int iterMax=ceil(log((sampleMax-sampleMin)/(2*epsilon))/log(n+1));

			// Initialize data struct.
			NArySearchData data;
			data.map=map;
			data.getFunctor=getFunctor;
			data.getUserData=getUserData;
			data.sampleCount=n;
			data.sampleTally=(long long int *)malloc(sizeof(long long int)*data.sampleCount); // TODO: Check return.
			data.sampleMin=sampleMin;
			data.sampleMax=sampleMax;
			data.sampleRange=data.sampleMax-data.sampleMin;

			// Loop, running iterations up to the maximum.
			for(int iter=0; iter<iterMax; ++iter) {
				assert(data.sampleMax>=data.sampleMin);

				// Update data struct for this iteration.
				for(int i=0; i<data.sampleCount; ++i)
					data.sampleTally[i]=0;
				data.sampleTotal=0;

				// Have we hit desired accuracy?
				if (data.sampleRange/2.0<=epsilon)
					break;

				// Run data collection functor.
				char progressString[1024];
				sprintf(progressString, "	%i/%i - interval [%f, %f] (range %f): ", iter+1, iterMax, data.sampleMin, data.sampleMax, data.sampleRange);
				modifyTiles(data.map, x, y, width, height, &mapGenNArySearchModifyTilesFunctor, &data, &mapGenModifyTilesProgressString, (void *)progressString);
				printf("\n");

				// Update min/max based on collected data.
				// TODO: this can be improved by looping to find window which contains the fraction we want, then updating min/max together and breaking
				double newSampleMin=data.sampleMin;
				double newSampleMax=data.sampleMax;
				int sampleIndex;
				for(sampleIndex=0; sampleIndex<data.sampleCount; ++sampleIndex) {
					double fraction=data.sampleTally[sampleIndex]/((double)data.sampleTotal);
					double sampleHeight=narySearchSampleToValue(&data, sampleIndex);
					if (fraction>threshold)
						newSampleMin=std::max(newSampleMin, sampleHeight);
					if (fraction<threshold)
						newSampleMax=std::min(newSampleMax, sampleHeight);
				}
				data.sampleMin=newSampleMin;
				data.sampleMax=newSampleMax;
				data.sampleRange=data.sampleMax-data.sampleMin;
			}

			// Tidy up.
			free(data.sampleTally);

			// Write out final range.
			printf("	final interval [%f, %f] (range %f, 2*epsilon %f)\n", data.sampleMin, data.sampleMax, data.sampleRange, 2*epsilon);

			// Return midpoint of interval.
			return (data.sampleMin+data.sampleMax)/2.0;
		}

		int MapGen::narySearchValueToSample(const NArySearchData *data, double height) {
			assert(data!=NULL);

			// TODO: We can presumably optimise this to avoid calling narySearchSampleToValue twice.

			// Check height is in range.
			if (height<narySearchSampleToValue(data, 0))
				return -1;

			if (height>=narySearchSampleToValue(data, data->sampleCount-1))
				return data->sampleCount-1;

			// Determine which sample bucket this height falls into.
			int result=floor(((height-data->sampleMin)/data->sampleRange)*(data->sampleCount+1.0)-1.0);
			assert(result>=0 && result<data->sampleCount);
			return result;
		}

		double MapGen::narySearchSampleToValue(const NArySearchData *data, int sample) {
			assert(data!=NULL);
			assert(sample>=0 && sample<data->sampleCount);

			return data->sampleMin+data->sampleRange*((sample+1.0)/(data->sampleCount+1.0));
		}

		void mapGenNArySearchModifyTilesFunctor(class Map *map, unsigned x, unsigned y, void *userData) {
			assert(map!=NULL);
			assert(userData!=NULL);

			MapGen::NArySearchData *data=(MapGen::NArySearchData *)userData;

			// Grab value.
			double value=data->getFunctor(map, x, y, data->getUserData);

			// Compute sample index.
			int sample=MapGen::narySearchValueToSample(data, value);

			// Update tally array.
			for(int i=0; i<=sample; ++i)
				++data->sampleTally[i];
			++data->sampleTotal;
		}

		double mapGenNarySearchGetFunctorHeight(class Map *map, unsigned x, unsigned y, void *userData) {
			assert(map!=NULL);
			assert(userData==NULL);

			// Grab tile.
			const MapTile *tile=map->getTileAtOffset(x, y, Map::Map::GetTileFlag::None);
			assert(tile!=NULL);

			// Return height.
			return tile->getHeight();
		}

		double mapGenNarySearchGetFunctorTemperature(class Map *map, unsigned x, unsigned y, void *userData) {
			assert(map!=NULL);
			assert(userData==NULL);

			// Grab tile.
			const MapTile *tile=map->getTileAtOffset(x, y, Map::Map::GetTileFlag::None);
			assert(tile!=NULL);

			// Return height.
			return tile->getTemperature();
		}

		double mapGenNarySearchGetFunctorMoisture(class Map *map, unsigned x, unsigned y, void *userData) {
			assert(map!=NULL);
			assert(userData==NULL);

			// Grab tile.
			const MapTile *tile=map->getTileAtOffset(x, y, Map::Map::GetTileFlag::None);
			assert(tile!=NULL);

			// Return height.
			return tile->getMoisture();
		}

		bool mapGenEdgeDetectHeightThresholdSampleFunctor(class Map *map, unsigned x, unsigned y, void *userData) {
			assert(map!=NULL);

			double height=*(const double *)userData;

			// Grab tile
			MapTile *tile=map->getTileAtOffset(x, y, Engine::Map::Map::GetTileFlag::None);
			if (tile==NULL)
				return false; // declare non-existing tiles as 'outside' of the edge

			// Check tile's height against passed threshold
			return (tile->getHeight()>height);
		}

		bool mapGenEdgeDetectLandSampleFunctor(class Map *map, unsigned x, unsigned y, void *userData) {
			assert(map!=NULL);

			// Use common height-threshold sample functor with sea level as the threshold to determine between land/ocean
			return mapGenEdgeDetectHeightThresholdSampleFunctor(map, x, y, &map->seaLevel);
		}

		void mapGenEdgeDetectBitsetNEdgeFunctor(class Map *map, unsigned x, unsigned y, void *userData) {
			assert(map!=NULL);

			unsigned bitsetIndex=(unsigned)(uintptr_t)userData;

			// Grab tile.
			MapTile *tile=map->getTileAtOffset(x, y, Engine::Map::Map::GetTileFlag::Dirty);
			if (tile==NULL)
				return; // shouldn't really happen but just to be safe

			// Mark this tile as part of the boundary
			tile->setBitsetN(bitsetIndex, true);
		}

		void mapGenEdgeDetectBitsetFullEdgeFunctor(class Map *map, unsigned x, unsigned y, void *userData) {
			assert(map!=NULL);

			uint64_t bitset=(uint64_t)(uintptr_t)userData;

			// Grab tile.
			MapTile *tile=map->getTileAtOffset(x, y, Engine::Map::Map::GetTileFlag::Dirty);
			if (tile==NULL)
				return; // shouldn't really happen but just to be safe

			// Mark this tile as part of the boundary
			tile->setBitset(tile->getBitset()|bitset);
		}

		void mapGenEdgeDetectStringProgressFunctor(class Map *map, double progress, Util::TimeMs elapsedTimeMs, void *userData) {
			assert(map!=NULL);
			assert(progress>=0.0 && progress<=1.0);
			assert(userData!=NULL);

			const char *string=(const char *)userData;

			// Clear old line.
			Util::clearConsoleLine();

			// Print start of new line, including users message and the percentage complete.
			printf("%s%.3f%% ", string, progress*100.0);

			// Append time elapsed so far.
			mapGenPrintTime(elapsedTimeMs);

			// Attempt to compute estimated total time.
			if (progress>=0.0001 && progress<=0.9999) {
				Util::TimeMs estRemainingTimeMs=elapsedTimeMs*(1.0/progress-1.0);
				if (estRemainingTimeMs>=1000 && estRemainingTimeMs<365ll*24ll*60ll*60ll*1000ll) {
					printf(" (~");
					mapGenPrintTime(estRemainingTimeMs);
					printf(" remaining)");
				}
			}

			// Flush output manually (as we are not printing a newline).
			fflush(stdout);
		}

		bool mapGenFloodFillBitsetNBoundaryFunctor(class Map *map, unsigned x, unsigned y, void *userData) {
			assert(map!=NULL);

			unsigned bitsetIndex=(unsigned)(uintptr_t)userData;

			// Grab tile.
			MapTile *tile=map->getTileAtOffset(x, y, Engine::Map::Map::GetTileFlag::None);
			if (tile==NULL)
				return false;

			// Return value of relevant bit
			return tile->getBitsetN(bitsetIndex);
		}

		void mapGenFloodFillStringProgressFunctor(class Map *map, double progress, Util::TimeMs elapsedTimeMs, void *userData) {
			assert(map!=NULL);
			assert(progress>=0.0 && progress<=1.0);
			assert(userData!=NULL);

			const char *string=(const char *)userData;

			// Clear old line.
			Util::clearConsoleLine();

			// Print start of new line, including users message and the percentage complete.
			printf("%s%.3f%% ", string, progress*100.0);

			// Append time elapsed so far.
			mapGenPrintTime(elapsedTimeMs);

			// Attempt to compute estimated total time.
			if (progress>=0.0001 && progress<=0.9999) {
				Util::TimeMs estRemainingTimeMs=elapsedTimeMs*(1.0/progress-1.0);
				if (estRemainingTimeMs>=1000 && estRemainingTimeMs<365ll*24ll*60ll*60ll*1000ll) {
					printf(" (~");
					mapGenPrintTime(estRemainingTimeMs);
					printf(" remaining)");
				}
			}

			// Flush output manually (as we are not printing a newline).
			fflush(stdout);
		}

		void mapGenEdgeDetectTraceHeightContoursProgressFunctor(class Map *map, double progress, Util::TimeMs elapsedTimeMs, void *userData) {
			assert(map!=NULL);
			assert(userData!=NULL);

			const MapGenEdgeDetectTraceHeightContoursData *functorData=(const MapGenEdgeDetectTraceHeightContoursData *)userData;

			// Calculate true progress and elapsed time (for the entire height contour operation, rather than this individual trace sub-operation)
			double trueProgress=(functorData->contourIndex+progress)/functorData->contourCount;

			Util::TimeMs trueElapsedTimeMs=Util::getTimeMs()-functorData->startTimeMs;

			// Call user progress functor (which cannot be NULL as we would not be executing this progress functor in the first place)
			functorData->functor(map, trueProgress, trueElapsedTimeMs, functorData->userData);
		}

		void mapGenEdgeDetectTraceClearScratchBitsModifyTilesProgressFunctor(class Map *map, double progress, Util::TimeMs elapsedTimeMs, void *userData) {
			assert(map!=NULL);
			assert(userData!=NULL);

			const MapGenEdgeDetectTraceClearScratchBitsModifyTilesProgressData *functorData=(const MapGenEdgeDetectTraceClearScratchBitsModifyTilesProgressData *)userData;

			// Calculate true progress (for the entire trace operation, rather than this individual modify tiles sub-operation)
			double trueProgress=functorData->progressRatio*progress;

			// Invoke user's progress functor
			functorData->functor(map, trueProgress, elapsedTimeMs, functorData->userData);
		}

		void mapGenFloodFillFillClearScratchBitModifyTilesProgressFunctor(class Map *map, double progress, Util::TimeMs elapsedTimeMs, void *userData) {
			assert(map!=NULL);
			assert(userData!=NULL);

			const MapGenFloodFillFillClearScratchBitModifyTilesProgressData *functorData=(const MapGenFloodFillFillClearScratchBitModifyTilesProgressData *)userData;

			// Calculate true progress (for the entire trace operation, rather than this individual modify tiles sub-operation)
			double trueProgress=functorData->progressRatio*progress;

			// Invoke user's progress functor
			functorData->functor(map, trueProgress, elapsedTimeMs, functorData->userData);
		}
	};
};
