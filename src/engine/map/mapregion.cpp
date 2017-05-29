#include <cassert>
#include <cstdlib>
#include <cstdio>

#include "mapregion.h"

using namespace Engine;
using namespace Engine::Physics;

namespace Engine {
	MapRegion::MapRegion() {
	}

	MapRegion::~MapRegion() {
	}

	bool MapRegion::save(const char *regionsDirPath, unsigned regionX, unsigned regionY) const {
		assert(regionsDirPath!=NULL);

		// Create file.
		char regionFilePath[1024]; // TODO: Prevent overflows.
		sprintf(regionFilePath, "%s/%u,%u", regionsDirPath, regionX, regionY);
		FILE *regionFile=fopen(regionFilePath, "w");

		// Save all tiles.
		unsigned tileX, tileY;
		size_t written=0, target=0;
		for(tileY=0; tileY<MapRegion::tilesHigh; ++tileY)
			for(tileX=0; tileX<MapRegion::tilesWide; ++tileX) {
				// Grab tile.
				const MapTile *tile=getTileAtOffset(tileX, tileY);

				// Save all layers.
				unsigned z;
				for(z=0; z<MapTile::layersMax; ++z) {
					// Grab layer.
					const MapTileLayer *layer=tile->getLayer(z);

					// Save texture.
					written+=sizeof(layer->textureId)*fwrite(&layer->textureId, sizeof(layer->textureId), 1, regionFile);
					target+=sizeof(layer->textureId)*1;
				}
			}

		// Close file.
		fclose(regionFile);

		return (written==target);
	}

	MapTile *MapRegion::getTileAtCoordVec(const CoordVec &vec) {
		CoordComponent tileX=vec.x/CoordsPerTile;
		CoordComponent tileY=vec.y/CoordsPerTile;
		CoordComponent offsetX=tileX%tilesWide;
		CoordComponent offsetY=tileY%tilesHigh;
		assert(offsetX>=0 && offsetX<tilesWide*CoordsPerTile);
		assert(offsetY>=0 && offsetY<tilesHigh*CoordsPerTile);

		return getTileAtOffset(offsetX, offsetY);
	}

	const MapTile *MapRegion::getTileAtCoordVec(const CoordVec &vec) const {
		CoordComponent tileX=vec.x/CoordsPerTile;
		CoordComponent tileY=vec.y/CoordsPerTile;
		CoordComponent offsetX=tileX%tilesWide;
		CoordComponent offsetY=tileY%tilesHigh;
		assert(offsetX>=0 && offsetX<tilesWide*CoordsPerTile);
		assert(offsetY>=0 && offsetY<tilesHigh*CoordsPerTile);

		return getTileAtOffset(offsetX, offsetY);
	}

	MapTile *MapRegion::getTileAtOffset(unsigned offsetX, unsigned offsetY) {
		assert(offsetX>=0 && offsetX<tilesWide*CoordsPerTile);
		assert(offsetY>=0 && offsetY<tilesHigh*CoordsPerTile);

		return &tiles[offsetY][offsetX];
	}

	const MapTile *MapRegion::getTileAtOffset(unsigned offsetX, unsigned offsetY) const  {
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
