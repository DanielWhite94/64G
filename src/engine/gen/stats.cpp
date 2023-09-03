#include <cassert>
#include <cfloat>
#include <cstdlib>

#include "stats.h"

using namespace Engine;

namespace Engine {
	namespace Gen {
		struct RecalculateStatsThreadData {
			double minHeight, maxHeight;
			double minTemperature, maxTemperature;
			double minMoisture, maxMoisture;
		};

		void recalculateStatsModifyTilesFunctor(unsigned threadId, class Map *map, unsigned x, unsigned y, void *userData);

		void recalculateStatsModifyTilesFunctor(unsigned threadId, class Map *map, unsigned x, unsigned y, void *userData) {
			assert(map!=NULL);
			assert(userData!=NULL);

			RecalculateStatsThreadData *dataArray=(RecalculateStatsThreadData *)userData;

			// Grab tile.
			const MapTile *tile=map->getTileAtOffset(x, y, Map::Map::GetTileFlag::None);
			if (tile==NULL)
				return;

			// Update statistics.
			dataArray[threadId].minHeight=std::min(dataArray[threadId].minHeight, tile->getHeight());
			dataArray[threadId].maxHeight=std::max(dataArray[threadId].maxHeight, tile->getHeight());
			dataArray[threadId].minTemperature=std::min(dataArray[threadId].minTemperature, tile->getTemperature());
			dataArray[threadId].maxTemperature=std::max(dataArray[threadId].maxTemperature, tile->getTemperature());
			dataArray[threadId].minMoisture=std::min(dataArray[threadId].minMoisture, tile->getMoisture());
			dataArray[threadId].maxMoisture=std::max(dataArray[threadId].maxMoisture, tile->getMoisture());
		}

		void recalculateStats(class Map *map, unsigned threadCount, Util::ProgressFunctor *progressFunctor, void *progressUserData) {
			assert(map!=NULL);

			// Initialize thread data.
			RecalculateStatsThreadData *threadData=(RecalculateStatsThreadData *)malloc(sizeof(RecalculateStatsThreadData)*threadCount);
			for(unsigned i=0; i<threadCount; ++i) {
				threadData[i].minHeight=DBL_MAX;
				threadData[i].maxHeight=DBL_MIN;
				threadData[i].minTemperature=DBL_MAX;
				threadData[i].maxTemperature=DBL_MIN;
				threadData[i].minMoisture=DBL_MAX;
				threadData[i].maxMoisture=DBL_MIN;
			}

			// Use modifyTiles to loop over tiles and update the above fields
			Gen::modifyTiles(map, 0, 0, map->getWidth(), map->getHeight(), threadCount, &recalculateStatsModifyTilesFunctor, threadData, progressFunctor, progressUserData);

			// Combine thread data to obtain final values
			map->minHeight=threadData[0].minHeight;
			map->maxHeight=threadData[0].maxHeight;
			map->minTemperature=threadData[0].minTemperature;
			map->maxTemperature=threadData[0].maxTemperature;
			map->minMoisture=threadData[0].minMoisture;
			map->maxMoisture=threadData[0].maxMoisture;

			for(unsigned i=1; i<threadCount; ++i) {
				map->minHeight=std::min(map->minHeight, threadData[i].minHeight);
				map->maxHeight=std::max(map->maxHeight, threadData[i].maxHeight);
				map->minTemperature=std::min(map->minTemperature, threadData[i].minTemperature);
				map->maxTemperature=std::max(map->maxTemperature, threadData[i].maxTemperature);
				map->minMoisture=std::min(map->minMoisture, threadData[i].minMoisture);
				map->maxMoisture=std::max(map->maxMoisture, threadData[i].maxMoisture);
			}
		}
	};
};
