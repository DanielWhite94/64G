#include <cassert>

#include "map.h"
#include "../graphics/renderer.h"

using namespace Engine::Graphics;

namespace Engine {
	namespace Map {
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

		MapTile *Map::getTileAtCoordVec(const CoordVec &vec) {
			MapRegion *region=getRegionAtCoordVec(vec);
			if (region==NULL)
				return NULL;

			return region->getTileAtCoordVec(vec);
		}

		const MapTile *Map::getTileAtCoordVec(const CoordVec &vec) const {
			const MapRegion *region=getRegionAtCoordVec(vec);
			if (region==NULL)
				return NULL;

			return region->getTileAtCoordVec(vec);
		}

		MapRegion *Map::getRegionAtCoordVec(const CoordVec &vec) {
			if (vec.x<0 || vec.y<0)
				return NULL;

			CoordComponent tileX=vec.x/Physics::CoordsPerTile;
			CoordComponent tileY=vec.y/Physics::CoordsPerTile;
			CoordComponent regionX=tileX/MapRegion::tilesWide;
			CoordComponent regionY=tileY/MapRegion::tilesHigh;

			if (regionX>=regionsWide || regionY>=regionsHigh)
				return NULL;

			return regions[regionY][regionX];
		}

		const MapRegion *Map::getRegionAtCoordVec(const CoordVec &vec) const {
			if (vec.x<0 || vec.y<0)
				return NULL;

			CoordComponent tileX=vec.x/Physics::CoordsPerTile;
			CoordComponent tileY=vec.y/Physics::CoordsPerTile;
			CoordComponent regionX=tileX/MapRegion::tilesWide;
			CoordComponent regionY=tileY/MapRegion::tilesHigh;

			if (regionX>=regionsWide || regionY>=regionsHigh)
				return NULL;

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

		void Map::addObject(MapObject *object) {
			assert(object!=NULL);

			// Add to object list.
			objects.push_back(object);

			// Update tiles.
			CoordVec vec;
			CoordVec vec1=object->getCoordTopLeft();
			CoordVec vec2=object->getCoordBottomRight();
			vec1.x=floor(vec1.x/Physics::CoordsPerTile)*Physics::CoordsPerTile;
			vec1.y=floor(vec1.y/Physics::CoordsPerTile)*Physics::CoordsPerTile;
			vec2.x=floor(vec2.x/Physics::CoordsPerTile)*Physics::CoordsPerTile;
			vec2.y=floor(vec2.y/Physics::CoordsPerTile)*Physics::CoordsPerTile;

			for(vec.y=vec1.y; vec.y<=vec2.y; vec.y+=Physics::CoordsPerTile)
				for(vec.x=vec1.x; vec.x<=vec2.x; vec.x+=Physics::CoordsPerTile)
					getTileAtCoordVec(vec)->addObject(object);
		}
	};
};
