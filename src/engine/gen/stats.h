#ifndef ENGINE_GEN_STATS_H
#define ENGINE_GEN_STATS_H

#include "modifytiles.h"
#include "../map/map.h"

namespace Engine {
	namespace Gen {
		// Recalculate map min/max values for height, temperature and moisture.
		void recalculateStats(class Map *map, unsigned threadCount, ModifyTilesProgress *progressFunctor, void *progressUserData);
	};
};

#endif
