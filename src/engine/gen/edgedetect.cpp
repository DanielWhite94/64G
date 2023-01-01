#include <cassert>

#include "edgedetect.h"
#include "modifytiles.h"
#include "../map/mapgen.h"

using namespace Engine;

namespace Engine {
	namespace Gen {
		struct EdgeDetectTraceHeightContoursData {
			unsigned contourIndex, contourCount;
			double heightThreshold;

			EdgeDetect::ProgressFunctor *functor;
			void *userData;

			Util::TimeMs startTimeMs;
		};

		struct EdgeDetectTraceClearScratchBitsModifyTilesProgressData {
			EdgeDetect::ProgressFunctor *functor;
			void *userData;

			double progressRatio;
		};

		void edgeDetectTraceHeightContoursProgressFunctor(class Map *map, double progress, Util::TimeMs elapsedTimeMs, void *userData);
		void edgeDetectTraceClearScratchBitsModifyTilesProgressFunctor(class Map *map, double progress, Util::TimeMs elapsedTimeMs, void *userData);

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

		void edgeDetectStringProgressFunctor(class Map *map, double progress, Util::TimeMs elapsedTimeMs, void *userData) {
			assert(map!=NULL);
			assert(progress>=0.0 && progress<=1.0);
			assert(userData!=NULL);

			const char *string=(const char *)userData;

			// Clear old line.
			Util::clearConsoleLine();

			// Print start of new line, including users message and the percentage complete.
			printf("%s%.3f%% ", string, progress*100.0);

			// Append time elapsed so far.
			Util::printTime(elapsedTimeMs);

			// Attempt to compute estimated total time.
			if (progress>=0.0001 && progress<=0.9999) {
				Util::TimeMs estRemainingTimeMs=elapsedTimeMs*(1.0/progress-1.0);
				if (estRemainingTimeMs>=1000 && estRemainingTimeMs<365ll*24ll*60ll*60ll*1000ll) {
					printf(" (~");
					Util::printTime(estRemainingTimeMs);
					printf(" remaining)");
				}
			}

			// Flush output manually (as we are not printing a newline).
			fflush(stdout);
		}

		void EdgeDetect::trace(SampleFunctor *sampleFunctor, void *sampleUserData, EdgeFunctor *edgeFunctor, void *edgeUserData, ProgressFunctor *progressFunctor, void *progressUserData) {
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
			if (progressFunctor!=NULL)
				progressFunctor(map, 1.0, Util::getTimeMs()-startTimeMs, progressUserData);
		}

		void EdgeDetect::traceHeightContours(int contourCount, EdgeDetect::ProgressFunctor *progressFunctor, void *progressUserData) {
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
				trace(&edgeDetectHeightThresholdSampleFunctor, &functorData.heightThreshold, &edgeDetectBitsetNEdgeFunctor, (void *)(uintptr_t)MapGen::TileBitsetIndexContour, (progressFunctor!=NULL ? &edgeDetectTraceHeightContoursProgressFunctor : NULL), (void *)&functorData);
			}
		}

		void edgeDetectTraceHeightContoursProgressFunctor(class Map *map, double progress, Util::TimeMs elapsedTimeMs, void *userData) {
			assert(map!=NULL);
			assert(userData!=NULL);

			const EdgeDetectTraceHeightContoursData *functorData=(const EdgeDetectTraceHeightContoursData *)userData;

			// Calculate true progress and elapsed time (for the entire height contour operation, rather than this individual trace sub-operation)
			double trueProgress=(functorData->contourIndex+progress)/functorData->contourCount;

			Util::TimeMs trueElapsedTimeMs=Util::getTimeMs()-functorData->startTimeMs;

			// Call user progress functor (which cannot be NULL as we would not be executing this progress functor in the first place)
			functorData->functor(map, trueProgress, trueElapsedTimeMs, functorData->userData);
		}

		void edgeDetectTraceClearScratchBitsModifyTilesProgressFunctor(class Map *map, double progress, Util::TimeMs elapsedTimeMs, void *userData) {
			assert(map!=NULL);
			assert(userData!=NULL);

			const EdgeDetectTraceClearScratchBitsModifyTilesProgressData *functorData=(const EdgeDetectTraceClearScratchBitsModifyTilesProgressData *)userData;

			// Calculate true progress (for the entire trace operation, rather than this individual modify tiles sub-operation)
			double trueProgress=functorData->progressRatio*progress;

			// Invoke user's progress functor
			functorData->functor(map, trueProgress, elapsedTimeMs, functorData->userData);
		}
	};
};
