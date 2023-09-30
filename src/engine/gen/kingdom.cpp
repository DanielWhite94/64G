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

		class KingdomIdentifyLandmassesData {
		public:
			KingdomIdentifyLandmassesData(class Map *map): map(map) {
				clear();
			}

			void clear(void) {
				oceanId=MapLandmass::IdNone;
				// TODO: use memset or w/e?
				for(unsigned i=0; i<MapLandmass::IdMax; ++i) {
					Entry *entry=&landmasses[i];

					entry->area=0;
					entry->isWater=true; // set to false on first encounter with land
					entry->rewriteId=i;
					entry->tileExampleX=0;
					entry->tileExampleY=0;
				}
			}

			void addTileStats(unsigned x, unsigned y, MapTile *tile) {
				// Grab id from tile and other data
				MapLandmass::Id id=tile->getLandmassId();
				Entry *entry=&landmasses[id];

				// Update stats
				++entry->area;
				if (tile->getHeight()>map->seaLevel)
					entry->isWater=false;
				entry->tileExampleX=x;
				entry->tileExampleY=y;
			}

			MapLandmass::Id getOceanId(void) const {
				return oceanId;
			}
			MapLandmass::Id resolveRewriteId(MapLandmass::Id id) {
				// 'Chase down' the id until we hit the final (and true) value
				while(landmasses[id].rewriteId!=id)
					id=landmasses[id].rewriteId;
				return id;
			}

			void setRewriteId(MapLandmass::Id id, MapLandmass::Id rewriteId) {
				landmasses[id].rewriteId=rewriteId;
			}

			void identifyOceanLandmassId(void) {
				oceanId=MapLandmass::IdNone;

				unsigned oceanArea=0;
				assert(MapLandmass::IdNone==0);
				for(unsigned i=1; i<MapLandmass::IdMax; ++i) {
					Entry *entry=&landmasses[i];

					// Unused id or not water?
					if (entry->area==0)
						break;
					if (!entry->isWater)
						continue;

					// Have we found a new largest landmass of water?
					if (entry->area>oceanArea) {
						oceanArea=entry->area;
						oceanId=i;
					}
				}
			}

			void identifyLandmasses(void) {
				assert(MapLandmass::IdNone==0);
				for(unsigned i=1; i<MapLandmass::IdMax; ++i) {
					Entry *entry=&landmasses[i];
					if (entry->area==0)
						continue;

					MapLandmass *landmass=new MapLandmass(i, entry->area, entry->tileExampleX, entry->tileExampleY);
					map->addLandmass(landmass);
				}
			}

		private:
			struct Entry {
				uint32_t area; // number of tiles making up this landmass
				MapLandmass::Id rewriteId; // used when merging landmasses together
				bool isWater; // true if all tiles are <= sea level (if area=0 then vacuously true)
				uint16_t tileExampleX, tileExampleY; // see maplandmass.h for a description of these
			};

			class Map *map;
			MapLandmass::Id oceanId; // initially MapLandmass::IdNone until computed
			Entry landmasses[MapLandmass::IdMax];
		};

		void kingdomIdentifyLandmassesFloodFillLandmassFillFunctor(class Map *map, unsigned x, unsigned y, unsigned groupId, void *userData);
		void kingdomIdentifyLandmassesModifyTilesFunctorClearLandmassId(unsigned threadId, class Map *map, unsigned x, unsigned y, void *userData);
		void kingdomIdentifyLandmassesModifyTilesFunctorAssignBoundaries(unsigned threadId, class Map *map, unsigned x, unsigned y, void *userData);
		void kingdomIdentifyLandmassesModifyTilesFunctorIdentifyMergers(unsigned threadId, class Map *map, unsigned x, unsigned y, void *userData);
		void kingdomIdentifyLandmassesModifyTilesFunctorProcessMergers(unsigned threadId, class Map *map, unsigned x, unsigned y, void *userData);
		void kingdomIdentifyLandmassesModifyTilesFunctorCollectStats(unsigned threadId, class Map *map, unsigned x, unsigned y, void *userData);

		void Kingdom::identifyLandmasses(unsigned threadCount, Util::ProgressFunctor *progressFunctor, void *progressUserData) {
			// Create progress data struct
			Util::ProgressFunctorScaledData progressData={
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
			Gen::modifyTiles(map, 0, 0, map->getWidth(), map->getHeight(), threadCount, &kingdomIdentifyLandmassesModifyTilesFunctorClearLandmassId, NULL, &utilProgressFunctorScaled, &progressData);

			// Trace edges of land/sea to form borders for later fill operation
			progressData.progressOffset=1.0/11.0;
			progressData.progressMultiplier=2.0/11.0;
			Gen::EdgeDetect landmassEdgeDetect(map);
			landmassEdgeDetect.traceFast(threadCount, &Gen::edgeDetectLandSampleFunctor, NULL, &Gen::edgeDetectBitsetNEdgeFunctor, (void *)(uintptr_t)Gen::TileBitsetIndexLandmassBorder, &utilProgressFunctorScaled, &progressData);

			// Create struct to hold data about landmasses
			KingdomIdentifyLandmassesData *landmassData=new KingdomIdentifyLandmassesData(map);

			// Fill each continent so the tile landmassId field has the same value for each continent
			progressData.progressOffset=3.0/11.0;
			progressData.progressMultiplier=4.0/11.0;
			Gen::FloodFill landmassFloodFill(map, 63);
			landmassFloodFill.fill(&Gen::floodFillBitsetNBoundaryFunctor, (void *)(uintptr_t)Gen::TileBitsetIndexLandmassBorder, &kingdomIdentifyLandmassesFloodFillLandmassFillFunctor, landmassData, &utilProgressFunctorScaled, &progressData);

			// Do a pass over the map to assign a nearby landmass id to boundary tiles
			progressData.progressOffset=7.0/11.0;
			progressData.progressMultiplier=1.0/11.0;
			Gen::modifyTiles(map, 0, 0, map->getWidth(), map->getHeight(), threadCount, &kingdomIdentifyLandmassesModifyTilesFunctorAssignBoundaries, NULL, &utilProgressFunctorScaled, &progressData);

			// Determine which landmass is the main ocean
			landmassData->identifyOceanLandmassId();

			// Merge together landmasses where they touch (e.g. seas/lakes wholly within other landmasses)
			// Do this in two passes - the first pass identifies all mergers and the second performs the actually updating
			progressData.progressOffset=8.0/11.0;
			progressData.progressMultiplier=1.0/11.0;
			Gen::modifyTiles(map, 0, 0, map->getWidth(), map->getHeight(), threadCount, &kingdomIdentifyLandmassesModifyTilesFunctorIdentifyMergers, landmassData, &utilProgressFunctorScaled, &progressData);

			progressData.progressOffset=9.0/11.0;
			progressData.progressMultiplier=1.0/11.0;
			Gen::modifyTiles(map, 0, 0, map->getWidth(), map->getHeight(), threadCount, &kingdomIdentifyLandmassesModifyTilesFunctorProcessMergers, landmassData, &utilProgressFunctorScaled, &progressData);

			// Recalculate landmass stats before adding proper landmass info to the map
			landmassData->clear();

			progressData.progressOffset=10.0/11.0;
			progressData.progressMultiplier=1.0/11.0;
			Gen::modifyTiles(map, 0, 0, map->getWidth(), map->getHeight(), threadCount, &kingdomIdentifyLandmassesModifyTilesFunctorCollectStats, landmassData, &utilProgressFunctorScaled, &progressData);

			landmassData->identifyLandmasses();

			// Tidy up
			delete landmassData;
		}

		void Kingdom::identifyKingdoms(unsigned threadCount, Util::ProgressFunctor *progressFunctor, void *progressUserData) {
			// Initially simply assign a kingdom for each landmass
			for(unsigned id=0; id<MapLandmass::IdMax; ++id) {
				MapLandmass *landmass=map->getLandmassById(id);
				if (landmass==NULL)
					continue;

				MapKingdom *kingdom=new MapKingdom(id);
				if (map->addKingdom(kingdom)) {
					landmass->setKingdomId(id);
					kingdom->addLandmass(landmass);
				} else
					delete kingdom;
			}
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
			landmassData->addTileStats(x, y, tile);
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
			if (id==MapLandmass::IdNone || id==landmassData->getOceanId())
				return;

			// 'chase down' our true id by following the 'pointers'
			id=landmassData->resolveRewriteId(id);

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
					if (neighbourId==MapLandmass::IdNone || neighbourId==landmassData->getOceanId())
						continue;

					// 'chase down' neighbour id
					neighbourId=landmassData->resolveRewriteId(neighbourId);

					// If neighbour id differs to our own, note these landmasses should be merged
					if (neighbourId!=id) {
						landmassData->setRewriteId(neighbourId, id);
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
			id=landmassData->resolveRewriteId(id);

			// Update our id
			tile->setLandmassId(id);
		}

		void kingdomIdentifyLandmassesModifyTilesFunctorCollectStats(unsigned threadId, class Map *map, unsigned x, unsigned y, void *userData) {
			assert(userData!=NULL);

			KingdomIdentifyLandmassesData *landmassData=(KingdomIdentifyLandmassesData *)userData;

			// Grab tile
			MapTile *tile=map->getTileAtOffset(x, y, Engine::Map::Map::GetTileFlag::None);
			if (tile==NULL)
				return;

			// Update stats
			landmassData->addTileStats(x, y, tile);
		}

	};
};
