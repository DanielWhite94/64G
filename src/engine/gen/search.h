#ifndef ENGINE_GEN_SEARCH_H
#define ENGINE_GEN_SEARCH_H

#include "../map/map.h"

namespace Engine {
	namespace Gen {
		void searchManyModifyTilesFunctor(unsigned threadId, class Map *map, unsigned x, unsigned y, void *userData);

		double searchGetFunctorHeight(class Map *map, unsigned x, unsigned y, void *userData);
		double searchGetFunctorTemperature(class Map *map, unsigned x, unsigned y, void *userData);
		double searchGetFunctorMoisture(class Map *map, unsigned x, unsigned y, void *userData);
		double searchGetFunctorNoise(class Map *map, unsigned x, unsigned y, void *userData); // userData should be a pointer to an FbnNoise instance

		typedef double (SearchGetFunctor)(class Map *map, unsigned x, unsigned y, void *userData);

		struct SearchDataEntry {
			class Map *map;

			SearchGetFunctor *getFunctor;
			void *getUserData;

			double sampleMin, sampleMax, sampleRange;
			double sampleConversionFactor; // equal to (data->sampleCount+1.0)/data->sampleRange, see MapGen::searchValueToSample
			int sampleCount;
			unsigned long long int *sampleTally, sampleTotal;

			int iterMax;

			double threshold;
			double epsilon;
		};

		struct SearchManyEntry {
			// threshold to aim for and allowed error
			double threshold;
			double epsilon;

			double sampleMin, sampleMax; // precomputed min/max values that can be encountered
			int sampleCount; // how many samples to take per iteration (1 gives binary search, 2 ternary etc)

			SearchGetFunctor *getFunctor;
			void *getUserData;

			double result;  // result written back into here
		};

		double search(class Map *map, unsigned x, unsigned y, unsigned width, unsigned height, unsigned threadCount, int n, double threshold, double epsilon, double sampleMin, double sampleMax, SearchGetFunctor *getFunctor, void *getUserData); // Returns (approximate) height/moisture etc value for which threshold fraction of the tiles in the given region are lower than said value.
		void searchMany(class Map *map, unsigned x, unsigned y, unsigned width, unsigned height, unsigned threadCount, size_t entryArrayCount, SearchManyEntry entryArray[]); // Same as running several search operations

		int searchValueToSample(const SearchDataEntry *entry, double value);
		double searchSampleToValue(const SearchDataEntry *entry, int sample);
	};
};

#endif
