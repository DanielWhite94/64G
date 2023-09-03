#include <cassert>

#include "common.h"
#include "edgedetect.h"
#include "floodfill.h"
#include "kingdom.h"
#include "modifytiles.h"

using namespace Engine;

namespace Engine {
	namespace Gen {

		struct KingdomIdentifyTerritoriesProgressData {
			double progressOffset;
			double progressMultiplier;

			Util::TimeMs startTimeMs;

			Util::ProgressFunctor *progressFunctor;
			void *progressUserData;
		};

		void kingdomIdentifyTerritoriesFloodFillLandmassFillFunctor(class Map *map, unsigned x, unsigned y, unsigned groupId, void *userData);
		bool kingdomIdentifyTerritoriesProgressFunctor(double progress, Util::TimeMs elapsedTimeMs, void *userData);
		void kingdomIdentifyTerritoriesModifyTilesFunctor(unsigned threadId, class Map *map, unsigned x, unsigned y, void *userData);

		void Kingdom::identifyTerritories(unsigned threadCount, Util::ProgressFunctor *progressFunctor, void *progressUserData) {
			// Create progress data struct
			KingdomIdentifyTerritoriesProgressData progressData={
				.startTimeMs=Util::getTimeMs(),
				.progressFunctor=progressFunctor,
				.progressUserData=progressUserData,
			};

			// Clear landmass id field in each tile
			// TODO: consider if this can be skipped - flood fill should update all tiles except perhaps the borders are the issue?
			progressData.progressOffset=0.0/7.0;
			progressData.progressMultiplier=1.0/7.0;
			Gen::modifyTiles(map, 0, 0, map->getWidth(), map->getHeight(), threadCount, &kingdomIdentifyTerritoriesModifyTilesFunctor, NULL, &kingdomIdentifyTerritoriesProgressFunctor, &progressData);

			// Trace edges of land/sea to form borders for later fill operation
			progressData.progressOffset=1.0/7.0;
			progressData.progressMultiplier=2.0/7.0;
			Gen::EdgeDetect landmassEdgeDetect(map);
			landmassEdgeDetect.traceFast(threadCount, &Gen::edgeDetectLandSampleFunctor, NULL, &Gen::edgeDetectBitsetNEdgeFunctor, (void *)(uintptr_t)Gen::TileBitsetIndexLandmassBorder, &kingdomIdentifyTerritoriesProgressFunctor, &progressData);

			// Fill each continent so the tile landmassId field has the same value for each continent
			progressData.progressOffset=3.0/7.0;
			progressData.progressMultiplier=4.0/7.0;
			Gen::FloodFill landmassFloodFill(map, 63);
			landmassFloodFill.fill(&Gen::floodFillBitsetNBoundaryFunctor, (void *)(uintptr_t)Gen::TileBitsetIndexLandmassBorder, &kingdomIdentifyTerritoriesFloodFillLandmassFillFunctor, NULL, &kingdomIdentifyTerritoriesProgressFunctor, &progressData);
		}

		void kingdomIdentifyTerritoriesFloodFillLandmassFillFunctor(class Map *map, unsigned x, unsigned y, unsigned groupId, void *userData) {
			assert(map!=NULL);
			assert(userData==NULL);

			// Grab tile.
			MapTile *tile=map->getTileAtOffset(x, y, Engine::Map::Map::GetTileFlag::CreateDirty);
			if (tile==NULL)
				return;

			// Set tile's landmass id
			// Note: we do +1 so that id 0 is reserved for 'border' tiles
			tile->setLandmassId(groupId+1);
		}

		bool kingdomIdentifyTerritoriesProgressFunctor(double progress, Util::TimeMs elapsedTimeMs, void *userData) {
			// Simply assume both operations will take a similar amount of time

			KingdomIdentifyTerritoriesProgressData *data=(KingdomIdentifyTerritoriesProgressData *)userData;

			// Call user's progress functor with modified progress and correct elapsed time
			return data->progressFunctor(data->progressOffset+progress*data->progressMultiplier, Util::getTimeMs()-data->startTimeMs, data->progressUserData);
		}

		void kingdomIdentifyTerritoriesModifyTilesFunctor(unsigned threadId, class Map *map, unsigned x, unsigned y, void *userData) {
			MapTile *tile=map->getTileAtOffset(x, y, Engine::Map::Map::GetTileFlag::Dirty);
			if (tile!=NULL)
				tile->setLandmassId(0);
		}
	};
};
