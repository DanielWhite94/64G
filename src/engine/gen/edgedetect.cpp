#include <cassert>

#include "common.h"
#include "edgedetect.h"
#include "modifytiles.h"

using namespace Engine;

namespace Engine {
	namespace Gen {
		struct EdgeDetectTraceHeightContoursData {
			unsigned contourIndex, contourCount;
			double heightThreshold;

			Util::ProgressFunctor *functor;
			void *userData;

			Util::TimeMs startTimeMs;
		};

		struct EdgeDetectTraceClearScratchBitsModifyTilesProgressData {
			Util::ProgressFunctor *functor;
			void *userData;

			double progressRatio;
		};

		bool edgeDetectTraceHeightContoursProgressFunctor(double progress, Util::TimeMs elapsedTimeMs, void *userData);
		bool edgeDetectTraceClearScratchBitsModifyTilesProgressFunctor(double progress, Util::TimeMs elapsedTimeMs, void *userData);

		void edgeDetectTraceFastModifyTilesFunctor(unsigned threadId, class Map *map, unsigned x, unsigned y, void *userData);

		bool edgeDetectHeightThresholdSampleFunctor(class Map *map, unsigned x, unsigned y, void *userData) {
			assert(map!=NULL);

			double height=*(const double *)userData;

			// Grab tile
			MapTile *tile=map->getTileAtOffset(x, y, Engine::Map::Map::GetTileFlag::None);
			if (tile==NULL)
				return false; // declare non-existing tiles as 'outside' of the edge

			// Check tile's height against passed threshold
			return (tile->getHeight()>height);
		}

		bool edgeDetectLandSampleFunctor(class Map *map, unsigned x, unsigned y, void *userData) {
			assert(map!=NULL);

			// Use common height-threshold sample functor with sea level as the threshold to determine between land/ocean
			return edgeDetectHeightThresholdSampleFunctor(map, x, y, &map->seaLevel);
		}

		void edgeDetectBitsetNEdgeFunctor(class Map *map, unsigned x, unsigned y, void *userData) {
			assert(map!=NULL);

			unsigned bitsetIndex=(unsigned)(uintptr_t)userData;

			// Grab tile.
			MapTile *tile=map->getTileAtOffset(x, y, Engine::Map::Map::GetTileFlag::Dirty);
			if (tile==NULL)
				return; // shouldn't really happen but just to be safe

			// Mark this tile as part of the boundary
			tile->setBitsetN(bitsetIndex, true);
		}

		void edgeDetectBitsetFullEdgeFunctor(class Map *map, unsigned x, unsigned y, void *userData) {
			assert(map!=NULL);

			uint64_t bitset=(uint64_t)(uintptr_t)userData;

			// Grab tile.
			MapTile *tile=map->getTileAtOffset(x, y, Engine::Map::Map::GetTileFlag::Dirty);
			if (tile==NULL)
				return; // shouldn't really happen but just to be safe

			// Mark this tile as part of the boundary
			tile->setBitset(tile->getBitset()|bitset);
		}

		void EdgeDetect::traceAccurate(unsigned scratchBits[DirectionNB], SampleFunctor *sampleFunctor, void *sampleUserData, EdgeFunctor *edgeFunctor, void *edgeUserData, Util::ProgressFunctor *progressFunctor, void *progressUserData) {
			assert(sampleFunctor!=NULL);

			// The following is an implementation of the Square Tracing method.

			const double preModifyTilesProgressRatio=0.1; // assume roughly 10% of the time is spent in scratch bit clearing modify tiles call

			unsigned mapWidth=map->getWidth();
			unsigned mapHeight=map->getHeight();

			Util::TimeMs startTimeMs=Util::getTimeMs();
			unsigned long long progressMax=mapWidth*mapHeight;
			unsigned long long progress=0.0;

			// Give a progress update
			if (progressFunctor!=NULL && !progressFunctor(0.0, Util::getTimeMs()-startTimeMs, progressUserData))
				return;

			// Clear scratch bits (these are used to indicate from which directions we have previously entered a tile on, to avoid retracing similar edges)
			EdgeDetectTraceClearScratchBitsModifyTilesProgressData modifyTileFunctorData;
			modifyTileFunctorData.functor=progressFunctor;
			modifyTileFunctorData.userData=progressUserData;
			modifyTileFunctorData.progressRatio=preModifyTilesProgressRatio;

			uint64_t scratchBitMask=(((uint64_t)1)<<scratchBits[0])|(((uint64_t)1)<<scratchBits[1])|(((uint64_t)1)<<scratchBits[2])|(((uint64_t)1)<<scratchBits[3]);
			Gen::modifyTiles(map, 0, 0, mapWidth, mapHeight, 1, &Gen::modifyTilesFunctorBitsetIntersection, (void *)(uintptr_t)~scratchBitMask, (progressFunctor!=NULL ? &edgeDetectTraceClearScratchBitsModifyTilesProgressFunctor : NULL), &modifyTileFunctorData);

			// Loop over regions
			unsigned rYEnd=mapHeight/MapRegion::tilesSize;
			for(unsigned rY=0; rY<rYEnd; ++rY) {
				unsigned rXEnd=mapWidth/MapRegion::tilesSize;
				for(unsigned rX=0; rX<rXEnd; ++rX) {
					// Calculate region tile boundaries
					unsigned tileX0=rX*MapRegion::tilesSize;
					unsigned tileY0=rY*MapRegion::tilesSize;
					unsigned tileX1=(rX+1)*MapRegion::tilesSize;
					unsigned tileY1=(rY+1)*MapRegion::tilesSize;

					// Loop over all tiles in this region, looking for starting points to trace from
					int startX, startY;
					for(startY=tileY0; startY<tileY1; ++startY) {
						for(startX=tileX0; startX<tileX1; ++startX) {
							// Give a progress update
							if (progressFunctor!=NULL && progress%256==0 && !progressFunctor(preModifyTilesProgressRatio+(1.0-preModifyTilesProgressRatio)*((double)progress)/progressMax, Util::getTimeMs()-startTimeMs, progressUserData))
								return;
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
							moveForward(&currX, &currY, velX, velY);

							// Walk the edge between 'inside' and 'outside' tiles
							// Note: we use Jacob's stopping criterion where we also ensure we return to the start tile with the same velocity as we started
							bool foundOutside=false;
							while(currX!=startX || currY!=startY || velX!=1 || velY!=0) {
								// Has this tile already been handled?
								// Check relevant cache bit based on current direction
								MapTile *tile=map->getTileAtOffset(currX, currY, Engine::Map::Map::GetTileFlag::None);
								if (tile!=NULL && tile->getBitsetN(scratchBits[currDir])) {
									// We have already entered this tile in this direction before - no point retracing it again.
									// So simply quit tracing this particular edge and move onto a new starting tile.

									// Reset foundOutside flag as a hack method to avoid calling edge functor for a final (unnecessary) time
									foundOutside=false;

									break;
								}

								// Determine if current tile is 'inside' or 'outside'
								if (sampleFunctor(map, currX, currY, sampleUserData)) {
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
								moveForward(&currX, &currY, velX, velY);
							}

							// Ensure start/end tile is considered as part of the boundary (this is done after the trace so we can compute foundOutside)
							if (edgeFunctor!=NULL && foundOutside) {
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
			if (progressFunctor!=NULL && !progressFunctor(1.0, Util::getTimeMs()-startTimeMs, progressUserData))
				return;
		}

		void EdgeDetect::traceFast(unsigned threadCount, SampleFunctor *sampleFunctor, void *sampleUserData, EdgeFunctor *edgeFunctor, void *edgeUserData, Util::ProgressFunctor *progressFunctor, void *progressUserData) {
			assert(sampleFunctor!=NULL);

			if (edgeFunctor==NULL)
				return;

			// Custom algorithm where we mark a tile as being part of an edge if it is inside while at least one neighbour is outside
			// (where out of bounds tiles are considered outside).

			// Use modify tiles to consider every tile
			TraceFastModifyTilesData modifyTilesData;
			modifyTilesData.sampleFunctor=sampleFunctor;
			modifyTilesData.sampleUserData=sampleUserData;
			modifyTilesData.edgeFunctor=edgeFunctor;
			modifyTilesData.edgeUserData=edgeUserData;

			Gen::modifyTiles(map, 0, 0, map->getWidth(), map->getHeight(), threadCount, &edgeDetectTraceFastModifyTilesFunctor, &modifyTilesData, progressFunctor, progressUserData);
		}

		void EdgeDetect::traceAccurateHeightContours(unsigned scratchBits[DirectionNB], int contourCount, Util::ProgressFunctor *progressFunctor, void *progressUserData) {
			// Check for bad contourCount
			if (contourCount<1)
				return;

			// Create our own wrapper userData for progress functor
			EdgeDetectTraceHeightContoursData functorData;
			functorData.contourIndex=0;
			functorData.contourCount=contourCount;
			functorData.functor=progressFunctor;
			functorData.userData=progressUserData;
			functorData.startTimeMs=Util::getTimeMs();

			// Run edge detection at various height thresholds to trace all contour lines
			for(functorData.contourIndex=0; functorData.contourIndex<contourCount; ++functorData.contourIndex) {
				functorData.heightThreshold=map->seaLevel+((functorData.contourIndex+1.0)/(contourCount+1))*(map->maxHeight-map->seaLevel);
				traceAccurate(scratchBits, &edgeDetectHeightThresholdSampleFunctor, &functorData.heightThreshold, &edgeDetectBitsetNEdgeFunctor, (void *)(uintptr_t)Gen::TileBitsetIndexContour, (progressFunctor!=NULL ? &edgeDetectTraceHeightContoursProgressFunctor : NULL), (void *)&functorData);
			}
		}

		void EdgeDetect::traceFastHeightContours(unsigned threadCount, int contourCount, Util::ProgressFunctor *progressFunctor, void *progressUserData) {
			// Check for bad contourCount
			if (contourCount<1)
				return;

			// Create our own wrapper userData for progress functor
			EdgeDetectTraceHeightContoursData functorData;
			functorData.contourIndex=0;
			functorData.contourCount=contourCount;
			functorData.functor=progressFunctor;
			functorData.userData=progressUserData;
			functorData.startTimeMs=Util::getTimeMs();

			// Run edge detection at various height thresholds to trace all contour lines
			for(functorData.contourIndex=0; functorData.contourIndex<contourCount; ++functorData.contourIndex) {
				functorData.heightThreshold=map->seaLevel+((functorData.contourIndex+1.0)/(contourCount+1))*(map->maxHeight-map->seaLevel);
				traceFast(threadCount, &edgeDetectHeightThresholdSampleFunctor, &functorData.heightThreshold, &edgeDetectBitsetNEdgeFunctor, (void *)(uintptr_t)Gen::TileBitsetIndexContour, (progressFunctor!=NULL ? &edgeDetectTraceHeightContoursProgressFunctor : NULL), (void *)&functorData);
			}
		}

		bool edgeDetectTraceHeightContoursProgressFunctor(double progress, Util::TimeMs elapsedTimeMs, void *userData) {
			assert(userData!=NULL);

			const EdgeDetectTraceHeightContoursData *functorData=(const EdgeDetectTraceHeightContoursData *)userData;

			// Calculate true progress and elapsed time (for the entire height contour operation, rather than this individual trace sub-operation)
			double trueProgress=(functorData->contourIndex+progress)/functorData->contourCount;

			Util::TimeMs trueElapsedTimeMs=Util::getTimeMs()-functorData->startTimeMs;

			// Call user progress functor (which cannot be NULL as we would not be executing this progress functor in the first place)
			return functorData->functor(trueProgress, trueElapsedTimeMs, functorData->userData);
		}

		bool edgeDetectTraceClearScratchBitsModifyTilesProgressFunctor(double progress, Util::TimeMs elapsedTimeMs, void *userData) {
			assert(userData!=NULL);

			const EdgeDetectTraceClearScratchBitsModifyTilesProgressData *functorData=(const EdgeDetectTraceClearScratchBitsModifyTilesProgressData *)userData;

			// Calculate true progress (for the entire trace operation, rather than this individual modify tiles sub-operation)
			double trueProgress=functorData->progressRatio*progress;

			// Invoke user's progress functor
			return functorData->functor(trueProgress, elapsedTimeMs, functorData->userData);
		}

		void edgeDetectTraceFastModifyTilesFunctor(unsigned threadId, class Map *map, unsigned x, unsigned y, void *userData) {
			assert(userData!=NULL);

			EdgeDetect::TraceFastModifyTilesData *data=(EdgeDetect::TraceFastModifyTilesData *)userData;

			// Is this tile not 'inside'? (and thus cannot be an edge)
			if (!data->sampleFunctor(map, x, y, data->sampleUserData))
				return;

			// If a single neighbour is not 'inside' then this is an edge
			unsigned xm1=map->decTileOffsetX(x);
			unsigned xp1=map->incTileOffsetX(x);
			unsigned ym1=map->decTileOffsetX(y);
			unsigned yp1=map->incTileOffsetX(y);

			if (!data->sampleFunctor(map, xm1, ym1, data->sampleUserData) ||
			    !data->sampleFunctor(map, x+0, ym1, data->sampleUserData) ||
			    !data->sampleFunctor(map, xp1, ym1, data->sampleUserData) ||
			    !data->sampleFunctor(map, xm1, y+0, data->sampleUserData) ||
			    !data->sampleFunctor(map, x+0, y+0, data->sampleUserData) ||
			    !data->sampleFunctor(map, xp1, y+0, data->sampleUserData) ||
			    !data->sampleFunctor(map, xm1, yp1, data->sampleUserData) ||
			    !data->sampleFunctor(map, x+0, yp1, data->sampleUserData) ||
			    !data->sampleFunctor(map, xp1, yp1, data->sampleUserData)) {
				// Call user's edge functor
				data->edgeFunctor(map, x, y, data->edgeUserData);
			}
		}
	};
};
