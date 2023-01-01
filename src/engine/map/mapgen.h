#ifndef ENGINE_MAP_MAPGEN_H
#define ENGINE_MAP_MAPGEN_H

#include <cstring>
#include <queue>
#include <iomanip>

#include "map.h"
#include "maptexture.h"
#include "maptile.h"
#include "../gen/modifytiles.h"
#include "../util.h"

namespace Engine {
	namespace Map {

		class MapGen {
		public:
			static const unsigned TileBitsetIndexContour=0;
			static const unsigned TileBitsetIndexLandmassBorder=1;

			MapGen();
			~MapGen();

			static void recalculateStats(class Map *map, unsigned x, unsigned y, unsigned width, unsigned height, unsigned threadCount, Gen::ModifyTilesProgress *progressFunctor, void *progressUserData); // Updates map's min/maxHeight and other such fields.

		private:
		};
	};
};

#endif
