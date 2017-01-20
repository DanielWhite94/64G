#include "map.h"
#include "../graphics/renderer.h"

using namespace Engine::Graphics;

namespace Engine {
	Map::Map() {
		unsigned i, j;
		for(i=0; i<regionsHigh; ++i)
			for(j=0; j<regionsWide; ++j)
				regions[i][j]=NULL;
	}

	Map::~Map() {
		unsigned i, j;
		for(i=0; i<regionsHigh; ++i)
			for(j=0; j<regionsWide; ++j)

				regions[i][j]=NULL;
	}

	const MapTile *Map::getTileAtCoordVec(const CoordVec &vec) const {
		const MapRegion *region=getRegionAtCoordVec(vec);
		if (region==NULL)
			return NULL;

		return region->getTileAtCoordVec(vec);
	}

	const MapRegion *Map::getRegionAtCoordVec(const CoordVec &vec) const {
		CoordComponent tileX=vec.x/Physics::CoordsPerTile;
		CoordComponent tileY=vec.y/Physics::CoordsPerTile;
		CoordComponent regionX=tileX/MapRegion::tilesWide;
		CoordComponent regionY=tileY/MapRegion::tilesHigh;

		return regions[regionY][regionX];
	}

	MapRegion *Map::getRegionAtCoordVec(const CoordVec &vec) {
		CoordComponent tileX=vec.x/Physics::CoordsPerTile;
		CoordComponent tileY=vec.y/Physics::CoordsPerTile;
		CoordComponent regionX=tileX/MapRegion::tilesWide;
		CoordComponent regionY=tileY/MapRegion::tilesHigh;

		return regions[regionY][regionX];
	}

	void Map::setTileAtCoordVec(const CoordVec &vec, const MapTile &tile) {
		MapRegion *region=getRegionAtCoordVec(vec);
		if (region==NULL) {
			region=new MapRegion();
			if (region==NULL)
				return;

			CoordComponent tileX=vec.x/Physics::CoordsPerTile;
			CoordComponent tileY=vec.y/Physics::CoordsPerTile;
			CoordComponent regionX=tileX/MapRegion::tilesWide;
			CoordComponent regionY=tileY/MapRegion::tilesHigh;

			regions[regionY][regionX]=region;
		}

		region->setTileAtCoordVec(vec, tile);
	}
};
