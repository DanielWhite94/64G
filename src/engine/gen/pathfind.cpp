#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>
#include <queue>

#include "modifytiles.h"
#include "pathfind.h"

using namespace Engine;

namespace Engine {
	namespace Gen {
		bool operator<(PathFind::SearchFullQueueEntry const &lhs, PathFind::SearchFullQueueEntry const &rhs);
		bool operator<(PathFind::SearchGoalQueueEntry const &lhs, PathFind::SearchGoalQueueEntry const &rhs);

		void pathFindClearModifyTilesFunctor(unsigned threadId, class Map *map, unsigned x, unsigned y, void *userData);

		float pathFindDistanceFunctorDistance(class Map *map, unsigned x1, unsigned y1, unsigned x2, unsigned y2, void *userData) {
			assert(map!=NULL);
			assert(userData==NULL);

			return 1.0;
		}

		float pathFindDistanceFunctorDistanceWeight(class Map *map, unsigned x1, unsigned y1, unsigned x2, unsigned y2, void *userData) {
			assert(map!=NULL);
			assert(userData==NULL);

			// Grab both tiles
			MapTile *tile1=map->getTileAtOffset(x1, y1, Engine::Map::Map::GetTileFlag::None);
			MapTile *tile2=map->getTileAtOffset(x2, y2, Engine::Map::Map::GetTileFlag::None);
			if (tile1==NULL || tile2==NULL)
				return 0.0;

			// Grab tile heights and if below sea level then cannot traverse
			double h1=tile1->getHeight();
			double h2=tile2->getHeight();
			if (h1<=map->seaLevel || h2<=map->seaLevel)
				return 0.0;

			// Compute weight/distance
			double distance=0.1; // due to moving 1m between adjacent tiles

			distance+=2.0*std::fabs(h1-h2); // penalise changes in altitude

			return distance;
		}

		float pathFindSearchGoalDistanceHeuristicZero(class Map *map, unsigned x1, unsigned y1, unsigned x2, unsigned y2, void *userData) {
			return 0.0;
		}

		float pathFindSearchGoalDistanceHeuristicDistance(class Map *map, unsigned x1, unsigned y1, unsigned x2, unsigned y2, void *userData) {
			// Only consider manhatten/taxicab distance
			return Util::wrappingDist(x1, y1, x2, y2, map->getWidth(), map->getHeight());
		}

		float pathFindSearchGoalDistanceHeuristicDistanceWeight(class Map *map, unsigned x1, unsigned y1, unsigned x2, unsigned y2, void *userData) {
			double distance=0.0;

			// Add manhatten/taxicab distance
			distance+=Util::wrappingDist(x1, y1, x2, y2, map->getWidth(), map->getHeight());

			// Compare height of two tiles and add the relevant penalaty
			MapTile *t1=map->getTileAtOffset(x1, y1, Engine::Map::Map::GetTileFlag::None);
			MapTile *t2=map->getTileAtOffset(x2, y2, Engine::Map::Map::GetTileFlag::None);
			if (t1!=NULL && t2!=NULL)
				distance+=2.0*std::fabs(t1->getHeight()-t2->getHeight()); // penalise changes in altitude

			return distance;
		}

		void PathFind::clear(unsigned threadCount, Util::ProgressFunctor *progressFunctor, void *progressUserData) {
			modifyTiles(map, 0, 0, map->getWidth(), map->getHeight(), threadCount, &pathFindClearModifyTilesFunctor, NULL, progressFunctor, progressUserData);
		}

		void PathFind::searchFull(unsigned endX, unsigned endY, DistanceFunctor *distanceFunctor, void *distanceUserData, Util::ProgressFunctor *progressFunctor, void *progressUserData) {
			assert(distanceFunctor!=NULL);

			// Init
			Util::TimeMs startTimeMs=Util::getTimeMs();

			// Give a progress update
			if (progressFunctor!=NULL && !progressFunctor(0.0, Util::getTimeMs()-startTimeMs, progressUserData))
				return;

			// Create priority queue which will store nodes/tiles which need processing
			std::priority_queue<PathFind::SearchFullQueueEntry> queue;

			// Begin at destination tile and work backwards
			// So by definition this tile has distance 0.0
			setTileScratchValue(endX, endY, 0.0);

			PathFind::SearchFullQueueEntry endEntry={
				.x=(uint16_t)endX,
				.y=(uint16_t)endY,
				.distance=(float)0.0,
			};
			queue.push(endEntry);

			// Process nodes/tiles until we reach destination or run out of tiles
			unsigned long long totalTiles=map->getWidth()*map->getHeight();
			unsigned long long handledTiles=0;

			while(!queue.empty()) {
				// Pop lowest distance entry
				PathFind::SearchFullQueueEntry entry=queue.top();
				queue.pop();

				// If distance recorded in the tile is lower than the distance stored in the struct then this tile was added to the queue again but with a lower distance.
				// Therefore the entry with this lower distance will have already been processed by now and so we can skip processing this tile again.
				float tileDistance=getTileScratchValue(entry.x, entry.y);
				if (tileDistance<entry.distance)
					continue;
				assert(entry.distance==tileDistance);

				// Handle neighbours
				unsigned nx, ny;
				float nd;
				float newd;
				float delta;

				// Left neighbour
				nx=(entry.x+map->getWidth()-1)%map->getWidth();
				ny=entry.y;
				nd=getTileScratchValue(nx, ny);
				delta=distanceFunctor(map, entry.x, entry.y, nx, ny, distanceUserData);
				newd=entry.distance+delta;
				if (delta>0.0 && newd<nd) {
					setTileScratchValue(nx, ny, newd);
					PathFind::SearchFullQueueEntry newEntry={
						.x=(uint16_t)nx,
						.y=(uint16_t)ny,
						.distance=newd,
					};
					queue.push(newEntry);
				}

				// Right neighbour
				nx=(entry.x+1)%map->getWidth();
				ny=entry.y;
				nd=getTileScratchValue(nx, ny);
				delta=distanceFunctor(map, entry.x, entry.y, nx, ny, distanceUserData);
				newd=entry.distance+delta;
				if (delta>0.0 && newd<nd) {
					setTileScratchValue(nx, ny, newd);
					PathFind::SearchFullQueueEntry newEntry={
						.x=(uint16_t)nx,
						.y=(uint16_t)ny,
						.distance=newd,
					};
					queue.push(newEntry);
				}

				// Above neighbour
				nx=entry.x;
				ny=(entry.y+map->getHeight()-1)%map->getHeight();
				nd=getTileScratchValue(nx, ny);
				delta=distanceFunctor(map, entry.x, entry.y, nx, ny, distanceUserData);
				newd=entry.distance+delta;
				if (delta>0.0 && newd<nd) {
					setTileScratchValue(nx, ny, newd);
					PathFind::SearchFullQueueEntry newEntry={
						.x=(uint16_t)nx,
						.y=(uint16_t)ny,
						.distance=newd,
					};
					queue.push(newEntry);
				}

				// Below neighbour
				nx=entry.x;
				ny=(entry.y+1)%map->getHeight();
				nd=getTileScratchValue(nx, ny);
				delta=distanceFunctor(map, entry.x, entry.y, nx, ny, distanceUserData);
				newd=entry.distance+delta;
				if (delta>0.0 && newd<nd) {
					setTileScratchValue(nx, ny, newd);
					PathFind::SearchFullQueueEntry newEntry={
						.x=(uint16_t)nx,
						.y=(uint16_t)ny,
						.distance=newd,
					};
					queue.push(newEntry);
				}

				// Give a progress update
				// TODO: improve this (currently works well then at some point suddenly jumps to 100% as we realise we don't have to handle every tile in the region)
				++handledTiles;
				double progress=((double)handledTiles)/totalTiles;
				progress=std::min(progress, 1.0);
				if (progressFunctor!=NULL && !progressFunctor(progress, Util::getTimeMs()-startTimeMs, progressUserData))
					return;
			}

			// Give a progress update
			if (progressFunctor!=NULL && !progressFunctor(1.0, Util::getTimeMs()-startTimeMs, progressUserData))
				return;

			return;
		}

		bool PathFind::searchGoal(unsigned startX, unsigned startY, unsigned endX, unsigned endY, DistanceFunctor *distanceFunctor, void *distanceUserData, HeuristicFunctor *heuristicFunctor, void *heuristicUserData, Util::ProgressFunctor *progressFunctor, void *progressUserData) {
			assert(distanceFunctor!=NULL);
			assert(heuristicFunctor!=NULL);

			// Init
			Util::TimeMs startTimeMs=Util::getTimeMs();

			// Give a progress update
			if (progressFunctor!=NULL && !progressFunctor(0.0, Util::getTimeMs()-startTimeMs, progressUserData))
				return false;

			// Create priority queue which will store nodes/tiles which need processing
			std::priority_queue<PathFind::SearchGoalQueueEntry> queue;

			// Begin at destination tile and work backwards
			// So by definition this tile has distance 0.0
			setTileScratchValue(endX, endY, 0.0);

			PathFind::SearchGoalQueueEntry endEntry={
				.x=(uint16_t)endX,
				.y=(uint16_t)endY,
				.distance=(float)0.0,
				.estimate=0.0, // not correct but doesn't matter as will always be processed first regardless
			};
			queue.push(endEntry);

			// Process nodes/tiles until we reach destination or run out of tiles
			unsigned progressCounter=0; // number of nodes/tiles we have processed
			unsigned progressMax=Util::wrappingDist(startX, startY, endX, endY, map->getWidth(), map->getHeight()); // distance between start and end tiles - used as a loose upper bound to estimate progress
			unsigned progressMin=progressMax; // distance for closest node/tile found so far
			while(!queue.empty()) {
				// Pop lowest distance entry
				PathFind::SearchGoalQueueEntry entry=queue.top();
				queue.pop();

				// If distance recorded in the tile is lower than the distance stored in the struct then this tile was added to the queue again but with a lower distance.
				// Therefore the entry with this lower distance will have already been processed by now and so we can skip processing this tile again.
				float tileDistance=getTileScratchValue(entry.x, entry.y);
				if (tileDistance<entry.distance)
					continue;
				assert(entry.distance==tileDistance);

				// Have we reached target start tile?
				if (entry.x==startX && entry.y==startY) {
					// Give a progress update
					if (progressFunctor!=NULL && !progressFunctor(1.0, Util::getTimeMs()-startTimeMs, progressUserData))
						return false;

					return true;
				}

				// Handle neighbours
				unsigned nx, ny;
				float nd;
				float newd;
				float delta;

				// Left neighbour
				nx=(entry.x+map->getWidth()-1)%map->getWidth();
				ny=entry.y;
				nd=getTileScratchValue(nx, ny);
				delta=distanceFunctor(map, entry.x, entry.y, nx, ny, distanceUserData);
				newd=entry.distance+delta;
				if (delta>0.0 && newd<nd) {
					setTileScratchValue(nx, ny, newd);
					PathFind::SearchGoalQueueEntry newEntry={
						.x=(uint16_t)nx,
						.y=(uint16_t)ny,
						.distance=newd,
						.estimate=newd+heuristicFunctor(map, nx, ny, startX, startY, heuristicUserData),
					};
					queue.push(newEntry);
				}

				// Right neighbour
				nx=(entry.x+1)%map->getWidth();
				ny=entry.y;
				nd=getTileScratchValue(nx, ny);
				delta=distanceFunctor(map, entry.x, entry.y, nx, ny, distanceUserData);
				newd=entry.distance+delta;
				if (delta>0.0 && newd<nd) {
					setTileScratchValue(nx, ny, newd);
					PathFind::SearchGoalQueueEntry newEntry={
						.x=(uint16_t)nx,
						.y=(uint16_t)ny,
						.distance=newd,
						.estimate=newd+heuristicFunctor(map, nx, ny, startX, startY, heuristicUserData),
					};
					queue.push(newEntry);
				}

				// Above neighbour
				nx=entry.x;
				ny=(entry.y+map->getHeight()-1)%map->getHeight();
				nd=getTileScratchValue(nx, ny);
				delta=distanceFunctor(map, entry.x, entry.y, nx, ny, distanceUserData);
				newd=entry.distance+delta;
				if (delta>0.0 && newd<nd) {
					setTileScratchValue(nx, ny, newd);
					PathFind::SearchGoalQueueEntry newEntry={
						.x=(uint16_t)nx,
						.y=(uint16_t)ny,
						.distance=newd,
						.estimate=newd+heuristicFunctor(map, nx, ny, startX, startY, heuristicUserData),
					};
					queue.push(newEntry);
				}

				// Below neighbour
				nx=entry.x;
				ny=(entry.y+1)%map->getHeight();
				nd=getTileScratchValue(nx, ny);
				delta=distanceFunctor(map, entry.x, entry.y, nx, ny, distanceUserData);
				newd=entry.distance+delta;
				if (delta>0.0 && newd<nd) {
					setTileScratchValue(nx, ny, newd);
					PathFind::SearchGoalQueueEntry newEntry={
						.x=(uint16_t)nx,
						.y=(uint16_t)ny,
						.distance=newd,
						.estimate=newd+heuristicFunctor(map, nx, ny, startX, startY, heuristicUserData),
					};
					queue.push(newEntry);
				}

				// Give a progress update
				// This is an estimate based on how far we are from the goal vs how far away we were when we started.
				unsigned progressCurrent=Util::wrappingDist(startX, startY, entry.x, entry.y, map->getWidth(), map->getHeight());
				if (progressCurrent<progressMin)
					progressMin=progressCurrent;

				if (progressCounter++%128==0) {
					double progress=1.0-((double)progressMin)/progressMax;
					progress=std::clamp(progress, 0.0, 1.0);
					if (progressFunctor!=NULL && !progressFunctor(progress, Util::getTimeMs()-startTimeMs, progressUserData))
						return false;
				}
			}

			// Give a progress update
			if (progressFunctor!=NULL && !progressFunctor(1.0, Util::getTimeMs()-startTimeMs, progressUserData))
				return false;

			return false;
		}

		bool PathFind::trace(unsigned startX, unsigned startY, TraceFunctor *functor, void *functorUserData, Util::ProgressFunctor *progressFunctor, void *progressUserData) {
			// Init
			Util::TimeMs startTimeMs=Util::getTimeMs();

			// Give a progress update
			if (progressFunctor!=NULL && !progressFunctor(0.0, Util::getTimeMs()-startTimeMs, progressUserData))
				return false;

			// Sanity check
			if (functor==NULL)
				return false;

			// Prepare for tracing loop
			unsigned x=startX;
			unsigned y=startY;
			float d=getTileScratchValue(x, y);
			const float dStart=d;

			// Loop until we reach goal or dead end
			unsigned progressCounter=0;
			while(1) {
				// This is a path tile - call user's functor
				functor(0, map, x, y, functorUserData);

				// Have we reached the goal? (indicated by distance 0.0)
				if (d==0.0) {
					// Give a progress update
					if (progressFunctor!=NULL && !progressFunctor(1.0, Util::getTimeMs()-startTimeMs, progressUserData))
						return false;

					return true;
				}

				// Compare neighbours looking for one with lowest distance
				unsigned bestX=x;
				unsigned bestY=y;
				float bestD=d;

				unsigned nx, ny;
				float nd;

				// Check left neightbour
				nx=(x+map->getWidth()-1)%map->getWidth();
				ny=y;
				nd=getTileScratchValue(nx, ny);
				if (nd<bestD) {
					bestX=nx;
					bestY=ny;
					bestD=nd;
				}

				// Check right neightbour
				nx=(x+1)%map->getWidth();
				ny=y;
				nd=getTileScratchValue(nx, ny);
				if (nd<bestD) {
					bestX=nx;
					bestY=ny;
					bestD=nd;
				}

				// Check up neightbour
				nx=x;
				ny=(y+map->getHeight()-1)%map->getHeight();
				nd=getTileScratchValue(nx, ny);
				if (nd<bestD) {
					bestX=nx;
					bestY=ny;
					bestD=nd;
				}

				// Check down neightbour
				nx=x;
				ny=(y+1)%map->getHeight();
				nd=getTileScratchValue(nx, ny);
				if (nd<bestD) {
					bestX=nx;
					bestY=ny;
					bestD=nd;
				}

				// Update values for next iteration
				if (bestD>=d)
					break; // stuck in local minimum
				x=bestX;
				y=bestY;
				d=bestD;

				// Give a progress update
				// Note: this is a bit of a hack by using the weighted distance metric to estimate how close we are to the goal (which has distance 0.0)
				if (progressCounter++%32==0 && progressFunctor!=NULL && !progressFunctor(1.0-d/dStart, Util::getTimeMs()-startTimeMs, progressUserData))
					return false;
			}

			// Give a progress update
			if (progressFunctor!=NULL && !progressFunctor(1.0, Util::getTimeMs()-startTimeMs, progressUserData))
				return false;

			return false;
		}

		float PathFind::getTileScratchValue(int x, int y) {
			// Grab tile.
			MapTile *tile=map->getTileAtOffset(x, y, Engine::Map::Map::GetTileFlag::None);
			if (tile==NULL)
				return std::numeric_limits<float>::max();

			return tile->getScratchFloat();
		}

		void PathFind::setTileScratchValue(int x, int y, float value) {
			// Grab tile.
			MapTile *tile=map->getTileAtOffset(x, y, Engine::Map::Map::GetTileFlag::CreateDirty);
			if (tile==NULL)
				return;

			// Update tile
			tile->setScratchFloat(value);
		}

		bool operator<(PathFind::SearchFullQueueEntry const &lhs, PathFind::SearchFullQueueEntry const &rhs) {
			// Note: this is reversed as we want to choose minimum distance entry from queue rather than maximum
			return lhs.distance>rhs.distance;
		}

		bool operator<(PathFind::SearchGoalQueueEntry const &lhs, PathFind::SearchGoalQueueEntry const &rhs) {
			// Note: this is reversed as we want to choose minimum distance entry from queue rather than maximum
			return lhs.estimate>rhs.estimate;
		}

		void pathFindClearModifyTilesFunctor(unsigned threadId, class Map *map, unsigned x, unsigned y, void *userData) {
			assert(map!=NULL);
			assert(userData==NULL);

			// Grab tile.
			MapTile *tile=map->getTileAtOffset(x, y, Engine::Map::Map::GetTileFlag::Dirty);
			if (tile==NULL)
				return;

			// Update tile.
			tile->setScratchFloat(std::numeric_limits<float>::max());
		}

	};
};
