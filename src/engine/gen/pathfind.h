#ifndef ENGINE_GEN_PATHFIND_H
#define ENGINE_GEN_PATHFIND_H

#include "../util.h"
#include "../map/map.h"

namespace Engine {
	namespace Gen {
		class PathFind {
		public:
			// This class provides algorithms to find paths across the Map based on a given weight function for traversing tiles.
			// Note that it uses the tile's scratch field to store distance to the end tile.

			struct QueueEntry {
				uint16_t x, y;
				float distance;
			};

			typedef void (PathFindFunctor)(unsigned threadId, class Map *map, unsigned x, unsigned y, void *userData); // note: for now threadId is always 0 - here simply to match ModifyTilesFunctor prototype

			PathFind(class Map *map): map(map) {
			};
			~PathFind() {};

			// Resets scratch float field in all tiles ready for a call to a search function
			void clear(unsigned threadCount, Util::ProgressFunctor *progressFunctor, void *progressUserData);

			// Search outwards from (endX,endY), using tile scratch float fields to hold distances from each tile to the given end goal
			void searchFull(unsigned endX, unsigned endY, Util::ProgressFunctor *progressFunctor, void *progressUserData);

			// Find a path from (startX,startY) to (endX,endY), using tile scratch float fields to hold the distance from each tile to the given end goal
			bool searchGoal(unsigned startX, unsigned startY, unsigned endX, unsigned endY, Util::ProgressFunctor *progressFunctor, void *progressUserData);

			// Trace shortest path from (startX,startY) to (endX,endY) specified during previous search call
			bool trace(unsigned startX, unsigned startY, PathFindFunctor *functor, void *functorUserData, Util::ProgressFunctor *progressFunctor, void *progressUserData);

		private:
			class Map *map;

			float getTileScratchValue(int x, int y);
			void setTileScratchValue(int x, int y, float value);

			float searchComputeLocalDistance(int x1, int y1, int x2, int y2); // tiles should be adjacent horizontally or vertically, returns 0 if not traversable, positive otherwise
		};

	};
};

#endif
