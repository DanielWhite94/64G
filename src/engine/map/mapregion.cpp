#include <cassert>
#include <cstdlib>
#include <cstdio>

#include "mapregion.h"

using namespace Engine;
using namespace Engine::Physics;

namespace Engine {
	MapRegion::MapRegion() {
		isDirty=false;
		unsigned tileX, tileY;
		for(tileY=0; tileY<MapRegion::tilesHigh; ++tileY)
			for(tileX=0; tileX<MapRegion::tilesWide; ++tileX)
				tileInstances[tileY][tileX].setFileData(&(tileFileData[tileY][tileX]));
	}

	MapRegion::~MapRegion() {
	}

	bool MapRegion::save(const char *regionsDirPath, unsigned regionX, unsigned regionY) {
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
				// Grab tile and save all layers.
				const MapTile *tile=getTileAtOffset(tileX, tileY);
				const MapTile::Layer *layers=tile->getLayers();
				written+=sizeof(MapTile::Layer)*fwrite(layers, sizeof(MapTile::Layer), MapTile::layersMax, regionFile);
				target+=sizeof(MapTile::Layer)*MapTile::layersMax;
			}

		// Close file.
		fclose(regionFile);

		// Success?
		bool success=(written==target);

		// Potentially update 'isDirty' flag.
		if (success)
			isDirty=false;

		return success;
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

		return &tileInstances[offsetY][offsetX];
	}

	const MapTile *MapRegion::getTileAtOffset(unsigned offsetX, unsigned offsetY) const  {
		assert(offsetX>=0 && offsetX<tilesWide*CoordsPerTile);
		assert(offsetY>=0 && offsetY<tilesHigh*CoordsPerTile);

		return &tileInstances[offsetY][offsetX];
	}

	bool MapRegion::getIsDirty(void) const {
		return isDirty;
	}

	void MapRegion::setDirty(void) {
		isDirty=true;
	}
};
