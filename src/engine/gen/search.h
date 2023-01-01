#ifndef ENGINE_GEN_SEARCH_H
#define ENGINE_GEN_SEARCH_H

#include "../map/map.h"

namespace Engine {
	namespace Gen {
		void searchModifyTilesFunctor(unsigned threadId, class Map *map, unsigned x, unsigned y, void *userData);
		double searchGetFunctorHeight(class Map *map, unsigned x, unsigned y, void *userData);
		double searchGetFunctorTemperature(class Map *map, unsigned x, unsigned y, void *userData);
		double searchGetFunctorMoisture(class Map *map, unsigned x, unsigned y, void *userData);
		double searchGetFunctorNoise(class Map *map, unsigned x, unsigned y, void *userData); // userData should be a pointer to an FbnNoise instance

		typedef double (SearchGetFunctor)(class Map *map, unsigned x, unsigned y, void *userData);

		struct SearchData {
			class Map *map;

			SearchGetFunctor *getFunctor;
			void *getUserData;

			double sampleMin, sampleMax, sampleRange;
			double sampleConversionFactor; // equal to (data->sampleCount+1.0)/data->sampleRange, see MapGen::searchValueToSample
			int sampleCount;
			unsigned long long int *sampleTally, sampleTotal;
		};

		double search(class Map *map, unsigned x, unsigned y, unsigned width, unsigned height, unsigned threadCount, int n, double threshold, double epsilon, double sampleMin, double sampleMax, SearchGetFunctor *getFunctor, void *getUserData); // Returns (approximate) height/moisture etc value for which threshold fraction of the tiles in the given region are lower than said value.
		int searchValueToSample(const SearchData *data, double value);
		double searchSampleToValue(const SearchData *data, int sample);
	};
};

#endif
