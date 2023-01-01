#include <cassert>
#include <cmath>

#include "modifytiles.h"
#include "search.h"
#include "../fbnnoise.h"

using namespace Engine;

namespace Engine {
	namespace Gen {
		double search(class Map *map, unsigned x, unsigned y, unsigned width, unsigned height, unsigned threadCount, int n, double threshold, double epsilon, double sampleMin, double sampleMax, SearchGetFunctor *getFunctor, void *getUserData) {
			assert(map!=NULL);
			assert(n>0);
			assert(0.0<=threshold<=1.0);
			assert(getFunctor!=NULL);

			// Calculate expected iterMax and use it just in case we cannot narrow down the range for some reason.
			// This calculation is based on the idea that each iteration reduces the range by a factpr of #samples+1.
			int iterMax=ceil(log((sampleMax-sampleMin)/(2*epsilon))/log(n+1));

			// Initialize data struct.
			SearchData data;
			data.map=map;
			data.getFunctor=getFunctor;
			data.getUserData=getUserData;
			data.sampleCount=n;
			data.sampleTally=(unsigned long long int *)malloc(sizeof(unsigned long long int)*data.sampleCount); // TODO: Check return.
			data.sampleMin=sampleMin;
			data.sampleMax=sampleMax;

			// Loop, running iterations up to the maximum.
			for(int iter=0; iter<iterMax; ++iter) {
				assert(data.sampleMax>=data.sampleMin);

				// Update cached values
				data.sampleRange=data.sampleMax-data.sampleMin;
				data.sampleConversionFactor=(data.sampleCount+1.0)/data.sampleRange;

				// Have we hit desired accuracy?
				if (data.sampleRange/2.0<=epsilon)
					break;

				// Clear data struct for this iteration.
				for(int i=0; i<data.sampleCount; ++i)
					data.sampleTally[i]=0;
				data.sampleTotal=0;

				// Run data collection functor.
				char progressString[1024];
				sprintf(progressString, "	%i/%i - interval [%f, %f] (range %f): ", iter+1, iterMax, data.sampleMin, data.sampleMax, data.sampleRange);
				Gen::modifyTiles(data.map, x, y, width, height, threadCount, &searchModifyTilesFunctor, &data, &Gen::modifyTilesProgressString, (void *)progressString);
				printf("\n");

				// Update min/max based on collected data.
				// TODO: this can be improved by looping to find window which contains the fraction we want, then updating min/max together and breaking
				double newSampleMin=data.sampleMin;
				double newSampleMax=data.sampleMax;
				int sampleIndex;
				unsigned long long int sampleCumulative=0;
				for(sampleIndex=data.sampleCount-1; sampleIndex>=0; --sampleIndex) {
					sampleCumulative+=data.sampleTally[sampleIndex];
					double fraction=sampleCumulative/((double)data.sampleTotal);
					double sampleValue=searchSampleToValue(&data, sampleIndex);
					if (fraction>threshold)
						newSampleMin=std::max(newSampleMin, sampleValue);
					if (fraction<threshold)
						newSampleMax=std::min(newSampleMax, sampleValue);
				}
				data.sampleMin=newSampleMin;
				data.sampleMax=newSampleMax;
			}

			// Tidy up.
			free(data.sampleTally);

			// Write out final range.
			printf("	final interval [%f, %f] (range %f, 2*epsilon %f)\n", data.sampleMin, data.sampleMax, data.sampleMax-data.sampleMin, 2*epsilon);

			// Return midpoint of interval.
			return (data.sampleMin+data.sampleMax)/2.0;
		}

		int searchValueToSample(const SearchData *data, double value) {
			assert(data!=NULL);

			// Determine which sample bucket this value falls into.
			int result=floor((value-data->sampleMin)*data->sampleConversionFactor)-1;

			if (result<0)
				result=0;
			if (result>=data->sampleCount)
				result=data->sampleCount-1;

			return result;
		}

		double searchSampleToValue(const SearchData *data, int sample) {
			assert(data!=NULL);
			assert(sample>=0 && sample<data->sampleCount);

			return data->sampleMin+(sample+1.0)/data->sampleConversionFactor;
		}

		void searchModifyTilesFunctor(unsigned threadId, class Map *map, unsigned x, unsigned y, void *userData) {
			assert(map!=NULL);
			assert(userData!=NULL);

			SearchData *data=(SearchData *)userData;

			// Grab value.
			double value=data->getFunctor(map, x, y, data->getUserData);

			// Compute sample index.
			int sample=searchValueToSample(data, value);

			// Update tally array and total.
			// TODO: Fix this to make the increments thread safe (make the struct members atomic presumably, or otherwise split the counters for each thread and combine at the end)
			++data->sampleTally[sample];
			++data->sampleTotal;
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
	};
};
