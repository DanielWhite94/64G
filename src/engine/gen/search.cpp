#include <cassert>
#include <cmath>

#include "modifytiles.h"
#include "search.h"
#include "../fbnnoise.h"

using namespace Engine;

namespace Engine {
	namespace Gen {
		struct SearchData {
			SearchDataEntry *entries;
			size_t count;
		};

		struct SearchManyModifyTilesProgressData {
			int iter, iterMax;

			Util::TimeMs startTimeMs;

			Util::ProgressFunctor *progressFunctor;
			void *progressUserData;
		};

		bool searchManyModifyTilesProgressFunctor(double progress, Util::TimeMs elapsedTimeMs, void *userData);

		double search(class Map *map, unsigned x, unsigned y, unsigned width, unsigned height, unsigned threadCount, int n, double threshold, double epsilon, double sampleMin, double sampleMax, SearchGetFunctor *getFunctor, void *getUserData, Util::ProgressFunctor *progressFunctor, void *progressUserData) {
			assert(map!=NULL);
			assert(n>0);
			assert(0.0<=threshold<=1.0);
			assert(getFunctor!=NULL);

			SearchManyEntry searchManyEntry;
			searchManyEntry.threshold=threshold;
			searchManyEntry.epsilon=epsilon;
			searchManyEntry.sampleMin=sampleMin;
			searchManyEntry.sampleMax=sampleMax;
			searchManyEntry.sampleCount=n;
			searchManyEntry.getFunctor=getFunctor;
			searchManyEntry.getUserData=getUserData;

			searchMany(map, x, y, width, height, threadCount, 1, &searchManyEntry, progressFunctor, progressUserData);

			return searchManyEntry.result;
		}

		void searchMany(class Map *map, unsigned x, unsigned y, unsigned width, unsigned height, unsigned threadCount, size_t entryArrayCount, SearchManyEntry entryArray[], Util::ProgressFunctor *progressFunctor, void *progressUserData) {
			assert(map!=NULL);

			Util::TimeMs startTimeMs=Util::getTimeMs();

			// Initialize data struct.
			SearchData data;
			data.count=entryArrayCount;
			data.entries=(SearchDataEntry *)malloc(sizeof(SearchDataEntry)*entryArrayCount); // TODO: Check return

			int trueIterMax=0;
			for(size_t i=0; i<data.count; ++i) {
				SearchDataEntry *entry=&data.entries[i];

				entry->map=map;
				entry->getFunctor=entryArray[i].getFunctor;
				entry->getUserData=entryArray[i].getUserData;
				entry->sampleCount=entryArray[i].sampleCount;
				entry->sampleTally=(unsigned long long int *)malloc(sizeof(unsigned long long int)*entry->sampleCount); // TODO: Check return.
				entry->sampleMin=entryArray[i].sampleMin;
				entry->sampleMax=entryArray[i].sampleMax;
				entry->threshold=entryArray[i].threshold;
				entry->epsilon=entryArray[i].epsilon;

				// Calculate expected iterMax and use it just in case we cannot narrow down the range for some reason.
				// This calculation is based on the idea that each iteration reduces the range by a factor of #samples+1.
				entry->iterMax=ceil(log((entry->sampleMax-entry->sampleMin)/(2*entry->epsilon))/log(entry->sampleCount+1));

				if (entry->iterMax>trueIterMax)
					trueIterMax=entry->iterMax;
			}

			// Loop, running iterations up to the maximum of the maximums.
			for(int iter=0; iter<trueIterMax; ++iter) {
				// Prepare each operation
				for(size_t i=0; i<data.count; ++i) {
					SearchDataEntry *entry=&data.entries[i];

					assert(entry->sampleMax>=entry->sampleMin);

					// Update cached values
					entry->sampleRange=entry->sampleMax-entry->sampleMin;
					entry->sampleConversionFactor=(entry->sampleCount+1.0)/entry->sampleRange;

					// Break out early if no need to compute another iteration for this operation
					if (entry->sampleRange/2.0<=entry->epsilon)
						continue;

					// Clear data struct for this iteration.
					for(int j=0; j<entry->sampleCount; ++j)
						entry->sampleTally[j]=0;
					entry->sampleTotal=0;
				}

				// Run data collection functor.
				SearchManyModifyTilesProgressData progressData={
					.iter=iter,
					.iterMax=trueIterMax,
					.startTimeMs=startTimeMs,
					.progressFunctor=progressFunctor,
					.progressUserData=progressUserData,
				};

				Gen::modifyTiles(map, x, y, width, height, threadCount, &searchManyModifyTilesFunctor, &data, (progressFunctor!=NULL ? &searchManyModifyTilesProgressFunctor : NULL), &progressData);

				// Update min/max based on collected data.
				// TODO: this can be improved by looping to find window which contains the fraction we want, then updating min/max together and breaking
				for(size_t i=0; i<data.count; ++i) {
					SearchDataEntry *entry=&data.entries[i];

					if (entry->sampleRange/2.0<=entry->epsilon)
						continue;

					double newSampleMin=entry->sampleMin;
					double newSampleMax=entry->sampleMax;
					int sampleIndex;
					unsigned long long int sampleCumulative=0;
					for(sampleIndex=entry->sampleCount-1; sampleIndex>=0; --sampleIndex) {
						sampleCumulative+=entry->sampleTally[sampleIndex];
						double fraction=sampleCumulative/((double)entry->sampleTotal);
						double sampleValue=searchSampleToValue(entry, sampleIndex);
						if (fraction>entry->threshold)
							newSampleMin=std::max(newSampleMin, sampleValue);
						if (fraction<entry->threshold)
							newSampleMax=std::min(newSampleMax, sampleValue);
					}
					entry->sampleMin=newSampleMin;
					entry->sampleMax=newSampleMax;
				}
			}

			// Tidy up.
			for(size_t i=0; i<data.count; ++i)
				free(data.entries[i].sampleTally);
			free(data.entries);

			// Return midpoint of interval.
			for(size_t i=0; i<data.count; ++i) {
				SearchDataEntry *entry=&data.entries[i];
				entryArray[i].result=(entry->sampleMin+entry->sampleMax)/2.0;
			}
		}

		int searchValueToSample(const SearchDataEntry *entry, double value) {
			assert(entry!=NULL);

			// Determine which sample bucket this value falls into.
			int result=floor((value-entry->sampleMin)*entry->sampleConversionFactor)-1;

			if (result<0)
				result=0;
			if (result>=entry->sampleCount)
				result=entry->sampleCount-1;

			return result;
		}

		double searchSampleToValue(const SearchDataEntry *entry, int sample) {
			assert(entry!=NULL);
			assert(sample>=0 && sample<entry->sampleCount);

			return entry->sampleMin+(sample+1.0)/entry->sampleConversionFactor;
		}

		void searchManyModifyTilesFunctor(unsigned threadId, class Map *map, unsigned x, unsigned y, void *userData) {
			assert(map!=NULL);
			assert(userData!=NULL);

			SearchData *data=(SearchData *)userData;

			// Loop over all operations we need to perform
			for(size_t i=0; i<data->count; ++i) {
				SearchDataEntry *entry=&data->entries[i];

				// Have we hit desired accuracy for this operation?
				if (entry->sampleRange/2.0<=entry->epsilon)
					continue;

				// Grab value.
				double value=entry->getFunctor(map, x, y, entry->getUserData);

				// Compute sample index.
				int sample=searchValueToSample(entry, value);

				// Update tally array and total.
				// TODO: Fix this to make the increments thread safe (make the struct members atomic presumably, or otherwise split the counters for each thread and combine at the end)
				++entry->sampleTally[sample];
				++entry->sampleTotal;
			}
		}

		double searchGetFunctorHeight(class Map *map, unsigned x, unsigned y, void *userData) {
			assert(map!=NULL);
			assert(userData==NULL);

			// Grab tile.
			const MapTile *tile=map->getTileAtOffset(x, y, Map::Map::GetTileFlag::None);
			assert(tile!=NULL);

			// Return height.
			return tile->getHeight();
		}

		double searchGetFunctorTemperature(class Map *map, unsigned x, unsigned y, void *userData) {
			assert(map!=NULL);
			assert(userData==NULL);

			// Grab tile.
			const MapTile *tile=map->getTileAtOffset(x, y, Map::Map::GetTileFlag::None);
			assert(tile!=NULL);

			// Return height.
			return tile->getTemperature();
		}

		double searchGetFunctorMoisture(class Map *map, unsigned x, unsigned y, void *userData) {
			assert(map!=NULL);
			assert(userData==NULL);

			// Grab tile.
			const MapTile *tile=map->getTileAtOffset(x, y, Map::Map::GetTileFlag::None);
			assert(tile!=NULL);

			// Return height.
			return tile->getMoisture();
		}

		double searchGetFunctorNoise(class Map *map, unsigned x, unsigned y, void *userData) {
			assert(map!=NULL);
			assert(userData!=NULL);

			const FbnNoise *noise=(const FbnNoise *)userData;

			return noise->eval(x/((double)map->getWidth()), y/((double)map->getHeight()));
		}

		bool searchManyModifyTilesProgressFunctor(double progress, Util::TimeMs elapsedTimeMs, void *userData) {
			assert(userData!=NULL);

			SearchManyModifyTilesProgressData *data=(SearchManyModifyTilesProgressData *)userData;

			// Calculate true progress and elapsed time
			double trueProgress=(data->iter+progress)/data->iterMax;
			Util::TimeMs trueElapsedTimeMs=Util::getTimeMs()-data->startTimeMs;

			// Call user's functor
			return data->progressFunctor(trueProgress, trueElapsedTimeMs, data->progressUserData);
		}
	};
};
