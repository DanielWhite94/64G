#include <cassert>
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
			double distance=1.0; // due to moving 1m between adjacent tiles

			distance+=(h1-h2)*(h1-h2); // penalise changes in altitude

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
			while(!queue.empty()) {
				// Pop lowest distance entry
				PathFind::SearchFullQueueEntry entry=queue.top();
				queue.pop();

				// If distance recorded in the tile is lower than the distance stored in the struct then this tile was added to the queue again but with a lower distance.
				// Therefore the entry with this lower distance will have already been processed by now and so we can skip processing this tile again.
				if (getTileScratchValue(entry.x, entry.y)<entry.distance)
					continue;

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
				// (passing progress=0.0 still at least pulses the bar to show activity - even if we don't have a good estimate of the progress)
				if (progressFunctor!=NULL && !progressFunctor(0.0, Util::getTimeMs()-startTimeMs, progressUserData))
					return;
			}

			// Give a progress update
			if (progressFunctor!=NULL && !progressFunctor(1.0, Util::getTimeMs()-startTimeMs, progressUserData))
				return;

			return;
		}

		bool PathFind::searchGoal(unsigned startX, unsigned startY, unsigned endX, unsigned endY, DistanceFunctor *distanceFunctor, void *distanceUserData, Util::ProgressFunctor *progressFunctor, void *progressUserData) {
			assert(distanceFunctor!=NULL);

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
			};
			queue.push(endEntry);

			// Process nodes/tiles until we reach destination or run out of tiles
			while(!queue.empty()) {
				// Pop lowest distance entry
				PathFind::SearchGoalQueueEntry entry=queue.top();
				queue.pop();

				// If distance recorded in the tile is lower than the distance stored in the struct then this tile was added to the queue again but with a lower distance.
				// Therefore the entry with this lower distance will have already been processed by now and so we can skip processing this tile again.
				if (getTileScratchValue(entry.x, entry.y)<entry.distance)
					continue;

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
					};
					queue.push(newEntry);
				}

				// Give a progress update
				// (passing progress=0.0 still at least pulses the bar to show activity - even if we don't have a good estimate of the progress)
				if (progressFunctor!=NULL && !progressFunctor(0.0, Util::getTimeMs()-startTimeMs, progressUserData))
					return false;
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
			return lhs.distance>rhs.distance;
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
