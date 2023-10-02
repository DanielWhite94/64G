#ifndef ENGINE_GEN_PATHFIND_H
#define ENGINE_GEN_PATHFIND_H

#include "../util.h"
#include "../map/map.h"

namespace Engine {
	namespace Gen {
		float pathFindDistanceFunctorDistance(class Map *map, unsigned x1, unsigned y1, unsigned x2, unsigned y2, void *userData); // based only on travel distance between tiles
		float pathFindDistanceFunctorDistanceWeight(class Map *map, unsigned x1, unsigned y1, unsigned x2, unsigned y2, void *userData); // based on travel distance between tiles with additional penalty for changes in altitude
		float pathFindSearchGoalDistanceHeuristicZero(class Map *map, unsigned x1, unsigned y1, unsigned x2, unsigned y2, void *userData); // used to disable heuristic optimisation
		float pathFindSearchGoalDistanceHeuristicDistance(class Map *map, unsigned x1, unsigned y1, unsigned x2, unsigned y2, void *userData); // based on travel distance between tiles
		float pathFindSearchGoalDistanceHeuristicDistanceWeight(class Map *map, unsigned x1, unsigned y1, unsigned x2, unsigned y2, void *userData); // based on travel distance between tiles with additional penalty for changes in altitude

		class PathFind {
		public:
			// This class provides algorithms to find paths across the Map based on a given weight function for traversing tiles.
			// Note that it uses the tile's scratch field to store distance to the end tile.

			struct SearchFullQueueEntry {
				uint16_t x, y;
				float distance;
			};

			struct SearchGoalQueueEntry {
				uint16_t x, y;
				float distance;
				float estimate; // distance + remaining distance estimate, as per A*
			};

			// A function which is called by the search functions to calculate the distance metric to travel from tiles (x1,y1) to (x2,y2)
			// Should return (positive) distance metric between two tiles (return 0 if not traversible)
			// Note: tiles are adjacent either horizontally or vertically
			typedef float (DistanceFunctor)(class Map *map, unsigned x1, unsigned y1, unsigned x2, unsigned y2, void *userData);

			// A function which is called by the searchFull function for every tile it processes.
			// Return true to continue the search, false to stop.
			typedef bool (TileFunctor)(class Map *map, unsigned x, unsigned y, void *userData);

			// A function which is called by the searchGoal function to optimise the search in the direction of the goal.
			// Should returns (positive) heuristic distance metric between two tiles (return 0 if not traversible)
			// Note: the tiles do NOT have to be adjacent horizontally or vertically - but can be anywhere on the map
			// Ideally this should never overestimate the distance or a sub-optimial path may be returned by a later trace
			typedef float (HeuristicFunctor)(class Map *map, unsigned x1, unsigned y1, unsigned x2, unsigned y2, void *userData);

			// A function which is called for each tile along a path when calling trace
			// Note: for now threadId is always 0 - here simply to match ModifyTilesFunctor prototype
			typedef void (TraceFunctor)(unsigned threadId, class Map *map, unsigned x, unsigned y, void *userData);

			PathFind(class Map *map): map(map) {
			};
			~PathFind() {};

			// Resets scratch float field in all tiles ready for a call to a search function
			void clear(unsigned threadCount, Util::ProgressFunctor *progressFunctor, void *progressUserData);

			// Search outwards from (endX,endY), using tile scratch float fields to hold distances from each tile to the given end goal
			void searchFull(unsigned endX, unsigned endY, DistanceFunctor *distanceFunctor, void *distanceUserData, TileFunctor *tileFunctor, void *tileUserData, Util::ProgressFunctor *progressFunctor, void *progressUserData);

			// Find a path from (startX,startY) to (endX,endY), using tile scratch float fields to hold the distance from each tile to the given end goal
			// Can pass a heuristic distance function to speed up the search or pass pathFindSearchGoalDistanceHeuristicZero to disable this
			bool searchGoal(unsigned startX, unsigned startY, unsigned endX, unsigned endY, DistanceFunctor *distanceFunctor, void *distanceUserData, HeuristicFunctor *heuristicFunctor, void *heuristicUserData, Util::ProgressFunctor *progressFunctor, void *progressUserData);

			// Trace shortest path from (startX,startY) to (endX,endY) specified during previous search call
			bool trace(unsigned startX, unsigned startY, TraceFunctor *functor, void *functorUserData, Util::ProgressFunctor *progressFunctor, void *progressUserData);

			// Get distance from given (startX/startY) to (endX,endY) specified during previous search call
			float getDistance(unsigned startX, unsigned startY);

		private:
			class Map *map;

			float getTileScratchValue(int x, int y);
			void setTileScratchValue(int x, int y, float value);
		};

	};
};

#endif
