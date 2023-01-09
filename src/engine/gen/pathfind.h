#ifndef ENGINE_GEN_PATHFIND_H
#define ENGINE_GEN_PATHFIND_H

#include "../util.h"
#include "../map/map.h"

namespace Engine {
	namespace Gen {
		float pathFindDistanceFunctorDistance(class Map *map, unsigned x1, unsigned y1, unsigned x2, unsigned y2, void *userData); // based only on travel distance between tiles
		float pathFindDistanceFunctorDistanceWeight(class Map *map, unsigned x1, unsigned y1, unsigned x2, unsigned y2, void *userData); // based on travel distance between tiles with additional penalty for changes in altitude

		class PathFind {
		public:
			// This class provides algorithms to find paths across the Map based on a given weight function for traversing tiles.
			// Note that it uses the tile's scratch field to store distance to the end tile.

			struct QueueEntry {
				uint16_t x, y;
				float distance;
			};

			typedef float (DistanceFunctor)(class Map *map, unsigned x1, unsigned y1, unsigned x2, unsigned y2, void *userData); // returns (positive) distance metric between two tiles (return 0 if not traversible), tiles should be adjacent horizontally or vertically
			typedef void (TraceFunctor)(unsigned threadId, class Map *map, unsigned x, unsigned y, void *userData); // note: for now threadId is always 0 - here simply to match ModifyTilesFunctor prototype

			PathFind(class Map *map): map(map) {
			};
			~PathFind() {};

			// Resets scratch float field in all tiles ready for a call to a search function
			void clear(unsigned threadCount, Util::ProgressFunctor *progressFunctor, void *progressUserData);

			// Search outwards from (endX,endY), using tile scratch float fields to hold distances from each tile to the given end goal
			void searchFull(unsigned endX, unsigned endY, DistanceFunctor *distanceFunctor, void *distanceUserData, Util::ProgressFunctor *progressFunctor, void *progressUserData);

			// Find a path from (startX,startY) to (endX,endY), using tile scratch float fields to hold the distance from each tile to the given end goal
			bool searchGoal(unsigned startX, unsigned startY, unsigned endX, unsigned endY, DistanceFunctor *distanceFunctor, void *distanceUserData, Util::ProgressFunctor *progressFunctor, void *progressUserData);

			// Trace shortest path from (startX,startY) to (endX,endY) specified during previous search call
			bool trace(unsigned startX, unsigned startY, TraceFunctor *functor, void *functorUserData, Util::ProgressFunctor *progressFunctor, void *progressUserData);

		private:
			class Map *map;

			float getTileScratchValue(int x, int y);
			void setTileScratchValue(int x, int y, float value);
		};

	};
};

#endif
