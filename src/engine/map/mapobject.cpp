#include <cassert>
#include <cstdlib>

#include "mapobject.h"

namespace Engine {
	namespace Map {
		MapObjectTile::MapObjectTile() {
		}

		MapObjectTile::~MapObjectTile() {
		}

		MapObject::MapObject(CoordAngle gAngle, const CoordVec &gPos, unsigned gTilesWide, unsigned gTilesHigh) {
			angle=gAngle;
			pos=gPos;
			tilesWide=gTilesWide;
			tilesHigh=gTilesHigh;

			tileData=(MapObjectTile **)malloc(sizeof(MapObjectTile)*getTilesWide()*getTilesHigh());
			assert(tileData!=NULL); // TODO: do better

			unsigned i, max=getTilesWide()*getTilesHigh();
			for(i=0; i<max; ++i)
				tileData[i]=new MapObjectTile(); // TODO: do better
		}

		MapObject::~MapObject() {
		}

		CoordAngle MapObject::getAngle(void) const {
			return angle;
		}

		CoordVec MapObject::getCoordTopLeft(void) const {
			return pos;
		}

		CoordVec MapObject::getCoordBottomRight(void) const {
			return getCoordBottomRightExclusive()-CoordVec(1,1);
		}

		CoordVec MapObject::getCoordBottomRightExclusive(void) const {
			return getCoordTopLeft()+getCoordSize();
		}

		CoordVec MapObject::getCoordSize(void) const {
			return CoordVec(getTilesWide()*CoordsPerTile, getTilesHigh()*CoordsPerTile);
		}

		unsigned MapObject::getTilesWide(void) const {
			return tilesWide;
		}

		unsigned MapObject::getTilesHigh(void) const {
			return tilesHigh;
		}

		HitMask MapObject::getHitMaskByTileOffset(unsigned xOffset, unsigned yOffset) const {
			assert(xOffset<getTilesWide());
			assert(yOffset<getTilesHigh());

			return tileData[xOffset+yOffset*getTilesWide()]->hitmask;
		}

		HitMask MapObject::getHitMaskByCoord(const CoordVec &vec) const {
			assert(vec.x>=getCoordTopLeft().x && vec.y>=getCoordTopLeft().y);
			assert(vec.x<=getCoordBottomRight().x && vec.y<=getCoordBottomRight().y);

			CoordVec newVec=vec-getCoordTopLeft();
			return getHitMaskByTileOffset(newVec.x/CoordsPerTile, newVec.y/CoordsPerTile);
		}

		void MapObject::setAngle(CoordAngle gAngle) {
			angle=gAngle;
		}

		void MapObject::setPos(const CoordVec &gPos) {
			pos=gPos;
		}

		void MapObject::setHitMaskByTileOffset(unsigned xOffset, unsigned yOffset, HitMask gHitmask) {
			assert(xOffset<getTilesWide());
			assert(yOffset<getTilesHigh());

			tileData[xOffset+yOffset*getTilesWide()]->hitmask=gHitmask;
		}
	};
};
