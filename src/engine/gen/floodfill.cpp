#include <cassert>

#include "floodfill.h"
#include "modifytiles.h"

using namespace Engine;

namespace Engine {
	namespace Gen {
		struct FloodFillFillClearScratchBitModifyTilesProgressData {
			Util::ProgressFunctor *functor;
			void *userData;

			double progressRatio;
		};

		bool floodFillFillClearScratchBitModifyTilesProgressFunctor(double progress, Util::TimeMs elapsedTimeMs, void *userData);

		bool floodFillBitsetNBoundaryFunctor(class Map *map, unsigned x, unsigned y, void *userData) {
			assert(map!=NULL);

			unsigned bitsetIndex=(unsigned)(uintptr_t)userData;

			// Grab tile.
			MapTile *tile=map->getTileAtOffset(x, y, Engine::Map::Map::GetTileFlag::None);
			if (tile==NULL)
				return false;

			// Return value of relevant bit
			return tile->getBitsetN(bitsetIndex);
		}

		void FloodFill::fill(BoundaryFunctor *boundaryFunctor, void *boundaryUserData, FillFunctor *fillFunctor, void *fillUserData, Util::ProgressFunctor *progressFunctor, void *progressUserData) {
			assert(boundaryFunctor!=NULL);
			assert(fillFunctor!=NULL);

			struct Segment {
				unsigned x0, x1, y; // note: x1 can be less than or equal to x0 if the segment wraps around the right hand edge of the map

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
			if (progressFunctor!=NULL && !progressFunctor(0.0, Util::getTimeMs()-startTimeMs, progressUserData))
				return;

			// Clear scratch bits (these are used to indicate from which directions we have previously entered a tile on, to avoid retracing similar edges)
			FloodFillFillClearScratchBitModifyTilesProgressData modifyTileFunctorData;
			modifyTileFunctorData.functor=progressFunctor;
			modifyTileFunctorData.userData=progressUserData;
			modifyTileFunctorData.progressRatio=preModifyTilesProgressRatio;

			uint64_t scratchBitMask=(((uint64_t)1)<<scratchBit);
			Gen::modifyTiles(map, 0, 0, mapWidth, mapHeight, 1, &modifyTilesFunctorBitsetIntersection, (void *)(uintptr_t)~scratchBitMask, (progressFunctor!=NULL ? &floodFillFillClearScratchBitModifyTilesProgressFunctor : NULL), &modifyTileFunctorData);

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
							// Has this tile already been handled?
							MapTile *tile=map->getTileAtOffset(startX, startY, Map::Map::GetTileFlag::None);
							if (tile==NULL || tile->getBitsetN(scratchBit))
								continue;

							// Invoke progress functor if needed
							if (progressFunctor!=NULL && progress%256==0 && !progressFunctor(preModifyTilesProgressRatio+(1.0-preModifyTilesProgressRatio)*((double)progress)/progressMax, Util::getTimeMs()-startTimeMs, progressUserData))
								return;
							++progress;

							// We need a non-boundary tile to start filling - so check now
							if (boundaryFunctor(map, startX, startY, boundaryUserData))
								continue;

							// Add start tile as an initial segment to start filling from
							std::vector<Segment> segments;
							segments.clear();
							segments.push_back(Segment(startX, map->incTileOffsetX(startX), startY));

							// Main loop to handle each segment/scanline
							while(!segments.empty()) {
								// Grab new segment
								Segment segment=segments.back();
								segments.pop_back();

								assert(segment.x0<mapWidth);
								assert(segment.x1<mapWidth);
								assert(segment.y<mapHeight);

								// Extend segment left until we hit a boundary
								assert(segment.x0<mapWidth);
								unsigned extendLoopInitialX=segment.x0;
								while(1) {
									// Compute nextX value
									unsigned nextX=map->decTileOffsetX(segment.x0);

									// Check for boundary
									if (boundaryFunctor(map, nextX, segment.y, boundaryUserData))
										break;

									// Advance segment start x
									segment.x0=nextX;
									if (segment.x0==extendLoopInitialX)
										break;
								}

								// Extend segment right until we hit a boundary
								assert(segment.x1<mapWidth);
								extendLoopInitialX=segment.x1;
								while(1) {
									// Compute nextX value
									unsigned nextX=map->incTileOffsetX(segment.x1);

									// Check for boundary
									if (boundaryFunctor(map, segment.x1, segment.y, boundaryUserData))
										break;

									// Advance segment end x
									segment.x1=nextX;
									if (segment.x1==extendLoopInitialX)
										break;
								}

								// Loop over this segment to handle each tile within, and to find new segments to add to the stack
								bool aboveActive=false, belowActive=false;
								Segment aboveSegment, belowSegment;
								aboveSegment.y=map->decTileOffsetY(segment.y);
								belowSegment.y=map->incTileOffsetY(segment.y);
								unsigned loopX=segment.x0;
								while(1) {
									// Ensure scratch bit is set in this tile's bitset to indicate we have handled it
									MapTile *loopTile=map->getTileAtOffset(loopX, segment.y, Map::Map::GetTileFlag::Dirty);
									if (loopTile==NULL) {
										// Advance to next tile in this segment
										loopX=map->incTileOffsetX(loopX);
										if (loopX==segment.x1)
											break;

										continue; // TODO: think about this - shouldn't happen but what happens if it does?
									}

									// Have we already handled this tile?
									if (loopTile->getBitsetN(scratchBit)) {
										// Advance to next tile in this segment
										loopX=map->incTileOffsetX(loopX);
										if (loopX==segment.x1)
											break;

										continue;
									}

									// Invoke progress functor if needed
									if (progressFunctor!=NULL && progress%256==0 && !progressFunctor(preModifyTilesProgressRatio+(1.0-preModifyTilesProgressRatio)*((double)progress)/progressMax, Util::getTimeMs()-startTimeMs, progressUserData))
										return;
									++progress;

									// Set scratch bit to indicate we have handled this tile
									loopTile->setBitsetN(scratchBit, true);

									// Call user's fill functor
									fillFunctor(map, loopX, segment.y, groupId, fillUserData);

									// Check for potential new segment above the current one
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
											aboveSegment.x1=map->incTileOffsetX(aboveSegment.x1);
									} else if (!aboveIsBoundary) {
										aboveActive=true;
										aboveSegment.x0=loopX;
										aboveSegment.x1=map->incTileOffsetX(loopX);
									}

									// Check for potential new segment below the current one
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
											belowSegment.x1=map->incTileOffsetX(belowSegment.x1);
									} else if (!belowIsBoundary) {
										belowActive=true;
										belowSegment.x0=loopX;
										belowSegment.x1=map->incTileOffsetX(loopX);
									}

									// Advance to next tile in this segment
									loopX=map->incTileOffsetX(loopX);
									if (loopX==segment.x1)
										break;
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
			if (progressFunctor!=NULL && !progressFunctor(1.0, Util::getTimeMs()-startTimeMs, progressUserData))
				return;
		}

		bool floodFillFillClearScratchBitModifyTilesProgressFunctor(double progress, Util::TimeMs elapsedTimeMs, void *userData) {
			assert(userData!=NULL);

			const FloodFillFillClearScratchBitModifyTilesProgressData *functorData=(const FloodFillFillClearScratchBitModifyTilesProgressData *)userData;

			// Calculate true progress (for the entire trace operation, rather than this individual modify tiles sub-operation)
			double trueProgress=functorData->progressRatio*progress;

			// Invoke user's progress functor
			return functorData->functor(trueProgress, elapsedTimeMs, functorData->userData);
		}

	};
};
