#include <cassert>
#include <cstdlib>

#include "mapregion.h"

using namespace Engine;
using namespace Engine::Physics;

namespace Engine {
	MapRegion::MapRegion() {
	}

	MapRegion::~MapRegion() {
	}

	const MapTile *MapRegion::getTileAtCoordVec(const CoordVec &vec) const {
		CoordComponent tileX=vec.x/CoordsPerTile;
		CoordComponent tileY=vec.y/CoordsPerTile;
		CoordComponent offsetX=tileX%tilesWide;
		CoordComponent offsetY=tileY%tilesHigh;
		assert(offsetX>=0 && offsetX<tilesWide*CoordsPerTile);
		assert(offsetY>=0 && offsetY<tilesHigh*CoordsPerTile);

		return &tiles[offsetY][offsetX];
	}

	void MapRegion::setTileAtCoordVec(const CoordVec &vec, const MapTile &tile) {
		CoordComponent tileX=vec.x/CoordsPerTile;
		CoordComponent tileY=vec.y/CoordsPerTile;
		CoordComponent offsetX=tileX%tilesWide;
		CoordComponent offsetY=tileY%tilesHigh;
		assert(offsetX>=0 && offsetX<tilesWide*CoordsPerTile);
		assert(offsetY>=0 && offsetY<tilesHigh*CoordsPerTile);

		tiles[offsetY][offsetX]=tile;
	}
};
