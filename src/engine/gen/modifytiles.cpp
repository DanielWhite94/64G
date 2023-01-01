#include <cassert>
#include <thread>

#include "modifytiles.h"
#include "../util.h"

using namespace Engine;

namespace Engine {
	namespace Gen {
		struct ModifyTilesManyThreadCommonData {
			class Map *map;

			unsigned functorArrayCount;
			ModifyTilesManyEntry *functorArray;

			unsigned x, y, width, height; // args passed into modifyTiles

			unsigned threadCount;

			ModifyTilesProgress *progressFunctor;
			void *progressUserData;
			Util::TimeMs startTimeMs;
		};

		struct ModifyTilesManyThreadData {
			ModifyTilesManyThreadCommonData *common;

			std::thread *thread;

			unsigned threadId;
		};

		void modifyTilesManyThreadFunctor(ModifyTilesManyThreadData *threadData);

		void modifyTilesFunctorBitsetUnion(unsigned threadId, class Map *map, unsigned x, unsigned y, void *userData) {
			assert(map!=NULL);

			uint64_t bitset=(uint64_t)(uintptr_t)userData;

			MapTile *tile=map->getTileAtOffset(x, y, Engine::Map::Map::GetTileFlag::Dirty);
			if (tile!=NULL)
				tile->setBitset(tile->getBitset()|bitset);
		}

		void modifyTilesFunctorBitsetIntersection(unsigned threadId, class Map *map, unsigned x, unsigned y, void *userData) {
			assert(map!=NULL);

			uint64_t bitset=(uint64_t)(uintptr_t)userData;

			MapTile *tile=map->getTileAtOffset(x, y, Engine::Map::Map::GetTileFlag::Dirty);
			if (tile!=NULL)
				tile->setBitset(tile->getBitset()&bitset);
		}

		void modifyTilesProgressString(class Map *map, double progress, Util::TimeMs elapsedTimeMs, void *userData) {
			assert(map!=NULL);
			assert(progress>=0.0 && progress<=1.0);
			assert(userData!=NULL);

			const char *string=(const char *)userData;

			// Clear old line.
			Util::clearConsoleLine();

			// Print start of new line, including users message and the percentage complete.
			printf("%s%.3f%% ", string, progress*100.0);

			// Append time elapsed so far.
			Util::printTime(elapsedTimeMs);

			// Attempt to compute estimated total time.
			if (progress>=0.0001 && progress<=0.9999) {
				Util::TimeMs estRemainingTimeMs=elapsedTimeMs*(1.0/progress-1.0);
				if (estRemainingTimeMs>=1000 && estRemainingTimeMs<365ll*24ll*60ll*60ll*1000ll) {
					printf(" (~");
					Util::printTime(estRemainingTimeMs);
					printf(" remaining)");
				}
			}

			// Flush output manually (as we are not printing a newline).
			fflush(stdout);
		}


		void modifyTiles(class Map *map, unsigned x, unsigned y, unsigned width, unsigned height, unsigned threadCount, ModifyTilesFunctor *functor, void *functorUserData, ModifyTilesProgress *progressFunctor, void *progressUserData) {
			assert(map!=NULL);
			assert(functor!=NULL);

			ModifyTilesManyEntry functorEntry;
			functorEntry.functor=functor,
			functorEntry.userData=functorUserData,

			modifyTilesMany(map, x, y, width, height, threadCount, 1, &functorEntry, progressFunctor, progressUserData);
		}

		void modifyTilesMany(class Map *map, unsigned x, unsigned y, unsigned width, unsigned height, unsigned threadCount, size_t functorArrayCount, ModifyTilesManyEntry functorArray[], ModifyTilesProgress *progressFunctor, void *progressUserData) {
			assert(map!=NULL);
			assert(functorArrayCount>0);
			assert(functorArray!=NULL);

			// FIXME: buggy if x/y/width/height do not align to exact regions

			// Record start time.
			const Util::TimeMs startTime=Util::getTimeMs();

			// Initial progress update (if needed).
			if (progressFunctor!=NULL) {
				Util::TimeMs elapsedTimeMs=Util::getTimeMs()-startTime;
				progressFunctor(map, 0.0, elapsedTimeMs, progressUserData);
			}

			// Cap thread count to sensible range
			if (threadCount<1)
				threadCount=1;
			if (threadCount>64)
				threadCount=64;

			// Prepare thread data
			ModifyTilesManyThreadCommonData threadCommonData;
			threadCommonData.map=map;
			threadCommonData.functorArrayCount=functorArrayCount;
			threadCommonData.functorArray=functorArray;
			threadCommonData.x=x;
			threadCommonData.y=y;
			threadCommonData.width=width;
			threadCommonData.height=height;
			threadCommonData.threadCount=threadCount;
			threadCommonData.progressFunctor=progressFunctor;
			threadCommonData.progressUserData=progressUserData;
			threadCommonData.startTimeMs=startTime;

			ModifyTilesManyThreadData *threadData=(ModifyTilesManyThreadData *)malloc(sizeof(ModifyTilesManyThreadData)*threadCount); // TODO: check return
			for(unsigned i=0; i<threadCount; ++i) {
				threadData[i].common=&threadCommonData;
				threadData[i].thread=NULL;
				threadData[i].threadId=i;
			}

			// Create and start worker threads
			// Note: the main thread also acts as a worker and thus we only need to create threadCount-1 new threads
			for(unsigned i=0; i<threadCount-1; ++i)
				threadData[i].thread=new std::thread(modifyTilesManyThreadFunctor, &threadData[i]);

			// Also use this thread as a worker
			modifyTilesManyThreadFunctor(&threadData[threadCount-1]);

			// Tidy up
			for(unsigned i=0; i<threadCount-1; ++i) {
				threadData[i].thread->join();
				delete threadData[i].thread;
			}

			free(threadData);

			// Update progress.
			if (progressFunctor!=NULL) {
				Util::TimeMs elapsedTimeMs=Util::getTimeMs()-startTime;
				progressFunctor(map, 1.0, elapsedTimeMs, progressUserData);
			}
		}

		void modifyTilesManyThreadFunctor(ModifyTilesManyThreadData *threadData) {
			// Calculate constants
			const unsigned regionX0=threadData->common->x/MapRegion::tilesSize;
			const unsigned regionY0=threadData->common->y/MapRegion::tilesSize;
			const unsigned regionX1=(threadData->common->x+threadData->common->width)/MapRegion::tilesSize;
			const unsigned regionY1=(threadData->common->y+threadData->common->height)/MapRegion::tilesSize;

			const unsigned regionCount=(regionX1-regionX0)*(regionY1-regionY0);
			unsigned regionsPerThread=regionCount/threadData->common->threadCount;
			const unsigned regionStartIndex=threadData->threadId*regionsPerThread;

			if (threadData->threadId==threadData->common->threadCount-1) {
				// Final thread may have slightly more to do
				regionsPerThread=regionCount-((threadData->common->threadCount-1)*regionsPerThread);
			}

			// Loop over regions assigned to us
			bool giveProgressUpdates=(threadData->threadId==threadData->common->threadCount-1 && threadData->common->progressFunctor!=NULL);
			for(unsigned regionOffsetIndex=0; regionOffsetIndex<regionsPerThread; ++regionOffsetIndex) {
				// Calculate region x/y
				unsigned regionIndex=regionStartIndex+regionOffsetIndex;
				unsigned regionX=regionX0+(regionIndex%(regionX1-regionX0));
				unsigned regionY=regionY0+(regionIndex/(regionX1-regionX0));

				unsigned baseTileX=regionX*MapRegion::tilesSize;
				unsigned baseTileY=regionY*MapRegion::tilesSize;

				// Loop over all tiles within this region
				for(unsigned tileY=0; tileY<MapRegion::tilesSize; ++tileY)
					for(unsigned tileX=0; tileX<MapRegion::tilesSize; ++tileX) {
						// Loop over functors
						for(size_t functorId=0; functorId<threadData->common->functorArrayCount; ++functorId)
							threadData->common->functorArray[functorId].functor(threadData->threadId, threadData->common->map, baseTileX+tileX, baseTileY+tileY, threadData->common->functorArray[functorId].userData);
					}

				// Update progress (if we are the main thread).
				if (giveProgressUpdates) {
					Util::TimeMs elapsedTimeMs=Util::getTimeMs()-threadData->common->startTimeMs;
					double progress=(regionOffsetIndex+1.0)/regionsPerThread;
					threadData->common->progressFunctor(threadData->common->map, progress, elapsedTimeMs, threadData->common->progressUserData);
				}
			}
		}

	};
};
