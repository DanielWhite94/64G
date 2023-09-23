#ifndef ENGINE_GEN_KINGDOM_H
#define ENGINE_GEN_KINGDOM_H

#include "../util.h"
#include "../map/map.h"

namespace Engine {
	namespace Gen {

		class Kingdom {
		public:
			// This class provides algorithms to generate 'kingdoms' - areas of the map under control of a particular race/group
			// There are functions to determine land coverage, find points of interest (POIs), create transport networks, etc.

			Kingdom(class Map *map): map(map) {
			};
			~Kingdom() {};

			// Identifies individual physical landmasses/continents and assigns each of them a unique id greater than 0.
			// All tiles within a continent have their landmassId field set to match that of the continent.
			// Clears and repopulates landmasses list in Map
			void identifyLandmasses(unsigned threadCount, Util::ProgressFunctor *progressFunctor, void *progressUserData);

		private:
			class Map *map;
		};

	};
};

#endif
