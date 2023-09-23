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

		struct KingdomIdentifyLandmassesProgressData {
			double progressOffset;
			double progressMultiplier;

			Util::TimeMs startTimeMs;

			Util::ProgressFunctor *progressFunctor;
			void *progressUserData;
		};

		struct KingdomIdentifyLandmassesDataSingle {
			uint32_t area; // number of tiles making up this landmass
			MapLandmass::Id rewriteId; // used when merging landmasses together
			bool isWater; // true if all tiles are <= sea level (if area=0 then vacuously true)
		};

		struct KingdomIdentifyLandmassesData {
			MapLandmass::Id oceanId; // initially MapLandmass::IdNone until computed
			KingdomIdentifyLandmassesDataSingle landmasses[MapLandmass::IdMax];
		};

		void kingdomIdentifyLandmassesFloodFillLandmassFillFunctor(class Map *map, unsigned x, unsigned y, unsigned groupId, void *userData);
		bool kingdomIdentifyLandmassesProgressFunctor(double progress, Util::TimeMs elapsedTimeMs, void *userData);
		void kingdomIdentifyLandmassesModifyTilesFunctorClearLandmassId(unsigned threadId, class Map *map, unsigned x, unsigned y, void *userData);
		void kingdomIdentifyLandmassesModifyTilesFunctorAssignBoundaries(unsigned threadId, class Map *map, unsigned x, unsigned y, void *userData);
		void kingdomIdentifyLandmassesModifyTilesFunctorIdentifyMergers(unsigned threadId, class Map *map, unsigned x, unsigned y, void *userData);
		void kingdomIdentifyLandmassesModifyTilesFunctorProcessMergers(unsigned threadId, class Map *map, unsigned x, unsigned y, void *userData);
		void kingdomIdentifyLandmassesModifyTilesFunctorCollectStats(unsigned threadId, class Map *map, unsigned x, unsigned y, void *userData);

		void Kingdom::identifyLandmasses(unsigned threadCount, Util::ProgressFunctor *progressFunctor, void *progressUserData) {
			// Create progress data struct
			KingdomIdentifyLandmassesProgressData progressData={
				.startTimeMs=Util::getTimeMs(),
				.progressFunctor=progressFunctor,
				.progressUserData=progressUserData,
			};

			// Clear existing landmasses stored in the map
			map->removeLandmasses();

			// Clear landmass id field in each tile
			// TODO: consider if this can be skipped - flood fill should update all tiles except perhaps the borders are the issue?
			progressData.progressOffset=0.0/11.0;
			progressData.progressMultiplier=1.0/11.0;
			Gen::modifyTiles(map, 0, 0, map->getWidth(), map->getHeight(), threadCount, &kingdomIdentifyLandmassesModifyTilesFunctorClearLandmassId, NULL, &kingdomIdentifyLandmassesProgressFunctor, &progressData);

			// Trace edges of land/sea to form borders for later fill operation
			progressData.progressOffset=1.0/11.0;
			progressData.progressMultiplier=2.0/11.0;
			Gen::EdgeDetect landmassEdgeDetect(map);
			landmassEdgeDetect.traceFast(threadCount, &Gen::edgeDetectLandSampleFunctor, NULL, &Gen::edgeDetectBitsetNEdgeFunctor, (void *)(uintptr_t)Gen::TileBitsetIndexLandmassBorder, &kingdomIdentifyLandmassesProgressFunctor, &progressData);

			// Create struct to hold data about landmasses
			KingdomIdentifyLandmassesData *landmassData=(KingdomIdentifyLandmassesData *)malloc(sizeof(KingdomIdentifyLandmassesData));
			landmassData->oceanId=MapLandmass::IdNone;
			for(unsigned i=0; i<MapLandmass::IdMax; ++i) {
				// TODO: use memset or w/e?
				landmassData->landmasses[i].area=0;
				landmassData->landmasses[i].isWater=true; // set to false on first encounter with land
				landmassData->landmasses[i].rewriteId=i;
			}

			// Fill each continent so the tile landmassId field has the same value for each continent
			progressData.progressOffset=3.0/11.0;
			progressData.progressMultiplier=4.0/11.0;
			Gen::FloodFill landmassFloodFill(map, 63);
			landmassFloodFill.fill(&Gen::floodFillBitsetNBoundaryFunctor, (void *)(uintptr_t)Gen::TileBitsetIndexLandmassBorder, &kingdomIdentifyLandmassesFloodFillLandmassFillFunctor, landmassData, &kingdomIdentifyLandmassesProgressFunctor, &progressData);

			// Do a pass over the map to assign a nearby landmass id to boundary tiles
			progressData.progressOffset=7.0/11.0;
			progressData.progressMultiplier=1.0/11.0;
			Gen::modifyTiles(map, 0, 0, map->getWidth(), map->getHeight(), threadCount, &kingdomIdentifyLandmassesModifyTilesFunctorAssignBoundaries, NULL, &kingdomIdentifyLandmassesProgressFunctor, &progressData);

			// Determine which landmass is actually the main ocean
			unsigned oceanArea=0;
			assert(landmassData->oceanId==MapLandmass::IdNone);
			assert(MapLandmass::IdNone==0);
			for(unsigned i=1; i<MapLandmass::IdMax; ++i) {
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
			progressData.progressOffset=8.0/11.0;
			progressData.progressMultiplier=1.0/11.0;
			Gen::modifyTiles(map, 0, 0, map->getWidth(), map->getHeight(), threadCount, &kingdomIdentifyLandmassesModifyTilesFunctorIdentifyMergers, landmassData, &kingdomIdentifyLandmassesProgressFunctor, &progressData);

			progressData.progressOffset=9.0/11.0;
			progressData.progressMultiplier=1.0/11.0;
			Gen::modifyTiles(map, 0, 0, map->getWidth(), map->getHeight(), threadCount, &kingdomIdentifyLandmassesModifyTilesFunctorProcessMergers, landmassData, &kingdomIdentifyLandmassesProgressFunctor, &progressData);

			// Calculate landmass stats before adding proper landmass info to the map
			for(unsigned i=0; i<MapLandmass::IdMax; ++i) {
				// TODO: use memset or w/e?
				landmassData->landmasses[i].area=0;
				landmassData->landmasses[i].isWater=true; // set to false on first encounter with land
				landmassData->landmasses[i].rewriteId=i;
			}

			progressData.progressOffset=10.0/11.0;
			progressData.progressMultiplier=1.0/11.0;
			Gen::modifyTiles(map, 0, 0, map->getWidth(), map->getHeight(), threadCount, &kingdomIdentifyLandmassesModifyTilesFunctorCollectStats, landmassData, &kingdomIdentifyLandmassesProgressFunctor, &progressData);

			for(unsigned i=0; i<MapLandmass::IdMax; ++i) {
				assert(landmassData->landmasses[i].rewriteId==i);
				if (landmassData->landmasses[i].area==0)
					continue;

				MapLandmass *landmass=new MapLandmass(i, landmassData->landmasses[i].area);
				map->addLandmass(landmass);
			}

			// Tidy up
			free(landmassData);
		}

		void kingdomIdentifyLandmassesFloodFillLandmassFillFunctor(class Map *map, unsigned x, unsigned y, unsigned groupId, void *userData) {
			assert(map!=NULL);
			assert(userData!=NULL);

			KingdomIdentifyLandmassesData *landmassData=(KingdomIdentifyLandmassesData *)userData;
			
			// Grab tile.
			MapTile *tile=map->getTileAtOffset(x, y, Engine::Map::Map::GetTileFlag::CreateDirty);
			if (tile==NULL)
				return;

			// Set tile's landmass id and update other data/stats
			// Note: we do +1 so that id 0 is reserved
			assert(MapLandmass::IdNone==0);
			MapLandmass::Id newId=groupId+1;
			tile->setLandmassId(newId);
			++landmassData->landmasses[newId].area;
			if (tile->getHeight()>map->seaLevel)
				landmassData->landmasses[newId].isWater=false;
		}

		bool kingdomIdentifyLandmassesProgressFunctor(double progress, Util::TimeMs elapsedTimeMs, void *userData) {
			KingdomIdentifyLandmassesProgressData *data=(KingdomIdentifyLandmassesProgressData *)userData;

			// Call user's progress functor with modified progress and correct elapsed time
			return data->progressFunctor(data->progressOffset+progress*data->progressMultiplier, Util::getTimeMs()-data->startTimeMs, data->progressUserData);
		}

		void kingdomIdentifyLandmassesModifyTilesFunctorClearLandmassId(unsigned threadId, class Map *map, unsigned x, unsigned y, void *userData) {
			MapTile *tile=map->getTileAtOffset(x, y, Engine::Map::Map::GetTileFlag::Dirty);
			if (tile!=NULL)
				tile->setLandmassId(MapLandmass::IdNone);
		}

		void kingdomIdentifyLandmassesModifyTilesFunctorAssignBoundaries(unsigned threadId, class Map *map, unsigned x, unsigned y, void *userData) {
			assert(userData==NULL);

			MapTile *tile=map->getTileAtOffset(x, y, Engine::Map::Map::GetTileFlag::Dirty);
			if (tile==NULL)
				return;

			// We only care about boundary tiles (which don't yet have a landmass id assigned to them)
			MapLandmass::Id id=tile->getLandmassId();
			if (id!=MapLandmass::IdNone)
				return;

			// Loop over 8 directly neighbouring tiles
			// TODO: make some kind of iterator for this?
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
					MapLandmass::Id neighbourId=neighbourTile->getLandmassId();
					if (neighbourId!=MapLandmass::IdNone) {
						tile->setLandmassId(neighbourId);
						return;
					}
				}
			}
		}

		void kingdomIdentifyLandmassesModifyTilesFunctorIdentifyMergers(unsigned threadId, class Map *map, unsigned x, unsigned y, void *userData) {
			assert(userData!=NULL);

			KingdomIdentifyLandmassesData *landmassData=(KingdomIdentifyLandmassesData *)userData;

			MapTile *tile=map->getTileAtOffset(x, y, Engine::Map::Map::GetTileFlag::None);
			if (tile==NULL)
				return;

			// We skip all tiles within the ocean landmass
			// (id==0 shouldn't be possible but no harm being safe)
			MapLandmass::Id id=tile->getLandmassId();
			if (id==MapLandmass::IdNone || id==landmassData->oceanId)
				return;

			// 'chase down' our true id by following the 'pointers'
			while(landmassData->landmasses[id].rewriteId!=id)
				id=landmassData->landmasses[id].rewriteId;

			// Loop over 8 directly neighbouring tiles
			// TODO: make some kind of iterator for this?
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
					MapLandmass::Id neighbourId=neighbourTile->getLandmassId();
					if (neighbourId==MapLandmass::IdNone || neighbourId==landmassData->oceanId)
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

		void kingdomIdentifyLandmassesModifyTilesFunctorProcessMergers(unsigned threadId, class Map *map, unsigned x, unsigned y, void *userData) {
			assert(userData!=NULL);

			KingdomIdentifyLandmassesData *landmassData=(KingdomIdentifyLandmassesData *)userData;

			MapTile *tile=map->getTileAtOffset(x, y, Engine::Map::Map::GetTileFlag::Dirty);
			if (tile==NULL)
				return;

			// 'chase down' our id to find true value
			MapLandmass::Id id=tile->getLandmassId();
			while(landmassData->landmasses[id].rewriteId!=id)
				id=landmassData->landmasses[id].rewriteId;

			// Update our id
			tile->setLandmassId(id);
		}

		void kingdomIdentifyLandmassesModifyTilesFunctorCollectStats(unsigned threadId, class Map *map, unsigned x, unsigned y, void *userData) {
			assert(userData!=NULL);

			KingdomIdentifyLandmassesData *landmassData=(KingdomIdentifyLandmassesData *)userData;

			// Grab tile
			MapTile *tile=map->getTileAtOffset(x, y, Engine::Map::Map::GetTileFlag::Dirty);
			if (tile==NULL)
				return;

			// Update stats
			MapLandmass::Id id=tile->getLandmassId();
			++landmassData->landmasses[id].area;
			if (tile->getHeight()>map->seaLevel)
				landmassData->landmasses[id].isWater=false;
		}

	};
};
