#include <cassert>
#include <cstdlib>

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

		struct KingdomIdentifyLandmassDataSingle {
			uint32_t area; // number of tiles making up this landmass
			uint16_t rewriteId; // used when merging landmasses together
			bool isWater; // true if all tiles are <= sea level (if area=0 then vacuously true)
		};

		struct KingdomIdentifyLandmassData {
			uint16_t oceanId; // initially 0 until computed
			KingdomIdentifyLandmassDataSingle landmasses[MapTile::landmassIdMax];
		};

		void kingdomIdentifyTerritoriesFloodFillLandmassFillFunctor(class Map *map, unsigned x, unsigned y, unsigned groupId, void *userData);
		bool kingdomIdentifyTerritoriesProgressFunctor(double progress, Util::TimeMs elapsedTimeMs, void *userData);
		void kingdomIdentifyTerritoriesModifyTilesFunctorClearLandmassId(unsigned threadId, class Map *map, unsigned x, unsigned y, void *userData);
		void kingdomIdentifyTerritoriesModifyTilesFunctorAssignBoundaries(unsigned threadId, class Map *map, unsigned x, unsigned y, void *userData);
		void kingdomIdentifyTerritoriesModifyTilesFunctorIdentifyMergers(unsigned threadId, class Map *map, unsigned x, unsigned y, void *userData);
		void kingdomIdentifyTerritoriesModifyTilesFunctorProcessMergers(unsigned threadId, class Map *map, unsigned x, unsigned y, void *userData);

		void Kingdom::identifyTerritories(unsigned threadCount, Util::ProgressFunctor *progressFunctor, void *progressUserData) {
			// Create progress data struct
			KingdomIdentifyTerritoriesProgressData progressData={
				.startTimeMs=Util::getTimeMs(),
				.progressFunctor=progressFunctor,
				.progressUserData=progressUserData,
			};

			// Clear landmass id field in each tile
			// TODO: consider if this can be skipped - flood fill should update all tiles except perhaps the borders are the issue?
			progressData.progressOffset=0.0/10.0;
			progressData.progressMultiplier=1.0/10.0;
			Gen::modifyTiles(map, 0, 0, map->getWidth(), map->getHeight(), threadCount, &kingdomIdentifyTerritoriesModifyTilesFunctorClearLandmassId, NULL, &kingdomIdentifyTerritoriesProgressFunctor, &progressData);

			// Trace edges of land/sea to form borders for later fill operation
			progressData.progressOffset=1.0/10.0;
			progressData.progressMultiplier=2.0/10.0;
			Gen::EdgeDetect landmassEdgeDetect(map);
			landmassEdgeDetect.traceFast(threadCount, &Gen::edgeDetectLandSampleFunctor, NULL, &Gen::edgeDetectBitsetNEdgeFunctor, (void *)(uintptr_t)Gen::TileBitsetIndexLandmassBorder, &kingdomIdentifyTerritoriesProgressFunctor, &progressData);

			// Create struct to hold data about landmasses
			KingdomIdentifyLandmassData *landmassData=(KingdomIdentifyLandmassData *)malloc(sizeof(KingdomIdentifyLandmassData));
			landmassData->oceanId=0;
			for(unsigned i=0; i<MapTile::landmassIdMax; ++i) {
				// TODO: use memset or w/e?
				landmassData->landmasses[i].area=0;
				landmassData->landmasses[i].isWater=true; // set to false on first encounter with land
				landmassData->landmasses[i].rewriteId=i;
			}

			// Fill each continent so the tile landmassId field has the same value for each continent
			progressData.progressOffset=3.0/10.0;
			progressData.progressMultiplier=4.0/10.0;
			Gen::FloodFill landmassFloodFill(map, 63);
			landmassFloodFill.fill(&Gen::floodFillBitsetNBoundaryFunctor, (void *)(uintptr_t)Gen::TileBitsetIndexLandmassBorder, &kingdomIdentifyTerritoriesFloodFillLandmassFillFunctor, landmassData, &kingdomIdentifyTerritoriesProgressFunctor, &progressData);

			// Do a pass over the map to assign a nearby landmass id to boundary tiles
			progressData.progressOffset=7.0/10.0;
			progressData.progressMultiplier=1.0/10.0;
			Gen::modifyTiles(map, 0, 0, map->getWidth(), map->getHeight(), threadCount, &kingdomIdentifyTerritoriesModifyTilesFunctorAssignBoundaries, NULL, &kingdomIdentifyTerritoriesProgressFunctor, &progressData);

			// Determine which landmass is actually the main ocean
			unsigned oceanArea=0;
			assert(landmassData->oceanId==0);
			for(unsigned i=1; i<MapTile::landmassIdMax; ++i) {
				if (landmassData->landmasses[i].area==0)
					break;
				if (!landmassData->landmasses[i].isWater)
					continue;

				if (landmassData->landmasses[i].area>oceanArea) {
					oceanArea=landmassData->landmasses[i].area;
					landmassData->oceanId=i;
				}
			}

			// Merge together landmasses where they touch (e.g. seas/lakes wholly within other landmasses)
			// Do this in two passes - the first pass identifies all mergers and the second performs the actually updating
			progressData.progressOffset=8.0/10.0;
			progressData.progressMultiplier=1.0/10.0;
			Gen::modifyTiles(map, 0, 0, map->getWidth(), map->getHeight(), threadCount, &kingdomIdentifyTerritoriesModifyTilesFunctorIdentifyMergers, landmassData, &kingdomIdentifyTerritoriesProgressFunctor, &progressData);

			progressData.progressOffset=9.0/10.0;
			progressData.progressMultiplier=1.0/10.0;
			Gen::modifyTiles(map, 0, 0, map->getWidth(), map->getHeight(), threadCount, &kingdomIdentifyTerritoriesModifyTilesFunctorProcessMergers, landmassData, &kingdomIdentifyTerritoriesProgressFunctor, &progressData);

			// Tidy up
			free(landmassData);
		}

		void kingdomIdentifyTerritoriesFloodFillLandmassFillFunctor(class Map *map, unsigned x, unsigned y, unsigned groupId, void *userData) {
			assert(map!=NULL);
			assert(userData!=NULL);

			KingdomIdentifyLandmassData *landmassData=(KingdomIdentifyLandmassData *)userData;
			
			// Grab tile.
			MapTile *tile=map->getTileAtOffset(x, y, Engine::Map::Map::GetTileFlag::CreateDirty);
			if (tile==NULL)
				return;

			// Set tile's landmass id and update other data/stats
			// Note: we do +1 so that id 0 is reserved for boundary/unassigned tiles
			unsigned newId=groupId+1;
			tile->setLandmassId(newId);
			++landmassData->landmasses[newId].area;
			if (tile->getHeight()>map->seaLevel)
				landmassData->landmasses[newId].isWater=false;
		}

		bool kingdomIdentifyTerritoriesProgressFunctor(double progress, Util::TimeMs elapsedTimeMs, void *userData) {
			// Simply assume both operations will take a similar amount of time

			KingdomIdentifyTerritoriesProgressData *data=(KingdomIdentifyTerritoriesProgressData *)userData;

			// Call user's progress functor with modified progress and correct elapsed time
			return data->progressFunctor(data->progressOffset+progress*data->progressMultiplier, Util::getTimeMs()-data->startTimeMs, data->progressUserData);
		}

		void kingdomIdentifyTerritoriesModifyTilesFunctorClearLandmassId(unsigned threadId, class Map *map, unsigned x, unsigned y, void *userData) {
			MapTile *tile=map->getTileAtOffset(x, y, Engine::Map::Map::GetTileFlag::Dirty);
			if (tile!=NULL)
				tile->setLandmassId(0);
		}

		void kingdomIdentifyTerritoriesModifyTilesFunctorAssignBoundaries(unsigned threadId, class Map *map, unsigned x, unsigned y, void *userData) {
			assert(userData==NULL);

			MapTile *tile=map->getTileAtOffset(x, y, Engine::Map::Map::GetTileFlag::Dirty);
			if (tile==NULL)
				return;

			// We only care about boundary tiles (which don't yet have a landmass id assigned to them)
			unsigned id=tile->getLandmassId();
			if (id!=0)
				return;

			// Loop over 8 directly neighbouring tiles
			for(int dy=-1; dy<2; ++dy) {
				for(int dx=-1; dx<2; ++dx) {
					if (dx==0 && dy==0)
						continue;

					int neighbourX=map->addTileOffsetX(x, dx);
					int neighbourY=map->addTileOffsetY(y, dy);

					const MapTile *neighbourTile=map->getTileAtOffset(neighbourX, neighbourY, Engine::Map::Map::GetTileFlag::None);
					if (neighbourTile==NULL)
						continue;

					// If neighbour has a non-boundary landmass id then update this tile's to match
					unsigned neighbourId=neighbourTile->getLandmassId();
					if (neighbourId!=0) {
						tile->setLandmassId(neighbourId);
						return;
					}
				}
			}
		}

		void kingdomIdentifyTerritoriesModifyTilesFunctorIdentifyMergers(unsigned threadId, class Map *map, unsigned x, unsigned y, void *userData) {
			assert(userData!=NULL);

			KingdomIdentifyLandmassData *landmassData=(KingdomIdentifyLandmassData *)userData;

			MapTile *tile=map->getTileAtOffset(x, y, Engine::Map::Map::GetTileFlag::None);
			if (tile==NULL)
				return;

			// We skip all tiles within the ocean landmass
			// (id==0 shouldn't be possible but no harm being safe)
			unsigned id=tile->getLandmassId();
			if (id==0 || id==landmassData->oceanId)
				return;

			// 'chase down' our true id by following the 'pointers'
			while(landmassData->landmasses[id].rewriteId!=id)
				id=landmassData->landmasses[id].rewriteId;

			// Loop over 8 directly neighbouring tiles
			for(int dy=-1; dy<2; ++dy) {
				for(int dx=-1; dx<2; ++dx) {
					if (dx==0 && dy==0)
						continue;

					int neighbourX=map->addTileOffsetX(x, dx);
					int neighbourY=map->addTileOffsetY(y, dy);

					const MapTile *neighbourTile=map->getTileAtOffset(neighbourX, neighbourY, Engine::Map::Map::GetTileFlag::None);
					if (neighbourTile==NULL)
						continue;

					// Not interested in ocean tiles
					unsigned neighbourId=neighbourTile->getLandmassId();
					if (neighbourId==0 || neighbourId==landmassData->oceanId)
						continue;

					// 'chase down' neighbour id
					while(landmassData->landmasses[neighbourId].rewriteId!=neighbourId)
						neighbourId=landmassData->landmasses[neighbourId].rewriteId;

					// If neighbour id differs to our own, note these landmasses should be merged
					if (neighbourId!=id) {
						landmassData->landmasses[neighbourId].rewriteId=id;
						break;
					}
				}
			}
		}

		void kingdomIdentifyTerritoriesModifyTilesFunctorProcessMergers(unsigned threadId, class Map *map, unsigned x, unsigned y, void *userData) {
			assert(userData!=NULL);

			KingdomIdentifyLandmassData *landmassData=(KingdomIdentifyLandmassData *)userData;

			MapTile *tile=map->getTileAtOffset(x, y, Engine::Map::Map::GetTileFlag::Dirty);
			if (tile==NULL)
				return;

			// 'chase down' our id to find true value
			unsigned id=tile->getLandmassId();
			while(landmassData->landmasses[id].rewriteId!=id)
				id=landmassData->landmasses[id].rewriteId;

			// Update our id
			tile->setLandmassId(id);
		}

	};
};
