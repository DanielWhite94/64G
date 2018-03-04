#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "mapregion.h"
#include "../util.h"

using namespace Engine;
using namespace Engine::Physics;

namespace Engine {
	MapRegion::MapRegion(unsigned regionX, unsigned regionY): regionX(regionX), regionY(regionY) {
		isDirty=false;

		// Set file data to all zeros.
		memset(tileFileData, 0, sizeof(tileFileData));

		// Update tile instances with their file data.
		unsigned tileX, tileY;
		for(tileY=0; tileY<MapRegion::tilesHigh; ++tileY)
			for(tileX=0; tileX<MapRegion::tilesWide; ++tileX)
				tileInstances[tileY][tileX].setFileData(&(tileFileData[tileY][tileX]));
	}

	MapRegion::~MapRegion() {
	}

	bool MapRegion::load(const char *regionPath) {
		// Open region file.
		FILE *regionFile=fopen(regionPath, "r");
		if (regionFile==NULL)
			return false;

		bool result=true;

		// Read tile data.
		size_t tileCount=tilesWide*tilesHigh;
		result&=(fread(&tileFileData, sizeof(MapTile::FileData), tileCount, regionFile)==tileCount);

		// Read object data.
		MapObject mapObject;
		while(mapObject.load(regionFile)) {
			MapObject *newObject=new MapObject(mapObject);

			if (!addObject(newObject)) {
				delete newObject;
				result=false;
			}
		}

		// Close region file.
		fclose(regionFile);

		return result;
	}

	bool MapRegion::save(const char *regionsDirPath, unsigned regionX, unsigned regionY) {
		assert(regionsDirPath!=NULL);

		// Create file.
		char regionFilePath[1024]; // TODO: Prevent overflows.
		sprintf(regionFilePath, "%s/%u,%u", regionsDirPath, regionX, regionY);
		FILE *regionFile=fopen(regionFilePath, "w");
		if (regionFile==NULL)
			return false;

		bool result=true;

		// Save tiles.
		size_t tileCount=tilesWide*tilesHigh;
		result&=(fwrite(&tileFileData, sizeof(MapTile::FileData), tileCount, regionFile)==tileCount);

		// Save objects.
		result&=saveObjects(regionFile);

		// Close file.
		fclose(regionFile);

		// Potentially update 'isDirty' flag.
		if (result)
			isDirty=false;

		return result;
	}

	bool MapRegion::saveObjects(FILE *regionFile) {
		assert(regionFile!=NULL);

		for(auto const *object: objects)
			if (!object->save(regionFile))
				return false;

		return true;
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

	bool MapRegion::addObject(MapObject *object) {
		assert(object!=NULL);

		// Compute dimensions.
		CoordVec vec;
		CoordVec vec1=object->getCoordTopLeft();
		CoordVec vec2=object->getCoordBottomRight();
		vec1.x=Util::floordiv(vec1.x, Physics::CoordsPerTile)*Physics::CoordsPerTile;
		vec1.y=Util::floordiv(vec1.y, Physics::CoordsPerTile)*Physics::CoordsPerTile;
		vec2.x=Util::floordiv(vec2.x, Physics::CoordsPerTile)*Physics::CoordsPerTile;
		vec2.y=Util::floordiv(vec2.y, Physics::CoordsPerTile)*Physics::CoordsPerTile;

		// Check this object is destined for this region.
		// TODO: this

		// Test new object is not out-of-bounds nor intersects any existing objects.
		for(vec.y=vec1.y; vec.y<=vec2.y; vec.y+=Physics::CoordsPerTile)
			for(vec.x=vec1.x; vec.x<=vec2.x; vec.x+=Physics::CoordsPerTile) {
				// Is there even a tile here?
				const MapTile *tile=getTileAtCoordVec(vec);
				if (tile==NULL)
					return false;

				// Tile too full?
				if (tile->isObjectsFull())
					return false;

				// Check for intersections.
				if (tile->getHitMask(vec) & object->getHitMaskByCoord(vec))
					return false;
			}

		// Add to object list.
		ownObject(object);

		// Add to tiles.
		for(vec.y=vec1.y; vec.y<=vec2.y; vec.y+=Physics::CoordsPerTile)
			for(vec.x=vec1.x; vec.x<=vec2.x; vec.x+=Physics::CoordsPerTile)
				getTileAtCoordVec(vec)->addObject(object);

		// Mark region dirty.
		isDirty=true;

		return true;
	}

	void MapRegion::ownObject(MapObject *object) {
		assert(object!=NULL);

		// TODO: Check if we already own the object.

		// Add object to list.
		objects.push_back(object);

		// Mark region dirty.
		isDirty=true;
	}

	void MapRegion::disownObject(MapObject *object) {
		assert(object!=NULL);

		// TODO: Check we already own the object before marking dirty.

		for(std::vector<MapObject *>::iterator iter=objects.begin(); iter!=objects.end(); ++iter) {
			if (*iter==object) {
				objects.erase(iter);
				break;
			}
		}

		// Mark region dirty.
		isDirty=true;
	}
};
