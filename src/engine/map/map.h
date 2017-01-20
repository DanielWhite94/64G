#ifndef ENGINE_GRAPHICS_MAP_H
#define ENGINE_GRAPHICS_MAP_H

#include "mapregion.h"
#include "maptile.h"
#include "../physics/coord.h"

using namespace Engine;
using namespace Engine::Physics;

namespace Engine {
	class Map {
	public:
		static const unsigned regionsWide=256, regionsHigh=256;

		Map();
		~Map();

		const MapTile *getTileAtCoordVec(const CoordVec &vec) const;
		const MapRegion *getRegionAtCoordVec(const CoordVec &vec) const;
		MapRegion *getRegionAtCoordVec(const CoordVec &vec);

		void setTileAtCoordVec(const CoordVec &vec, const MapTile &tile);

	private:
		MapRegion *regions[regionsHigh][regionsWide];
	};
};

#endif
