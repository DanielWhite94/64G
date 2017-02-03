#include <cassert>
#include <cstdlib>

#include "mapobject.h"

namespace Engine {
	namespace Map {
		MapObject::MapObject(CoordAngle gAngle, const CoordVec &gPos, unsigned gTilesWide, unsigned gTilesHigh) {
			angle=gAngle;
			pos=gPos;
			tilesWide=gTilesWide;
			tilesHigh=gTilesHigh;

			hitmasks=(Physics::HitMask **)malloc(sizeof(Physics::HitMask)*getTilesWide()*getTilesHigh());
			assert(hitmasks!=NULL); // TODO: do better

			unsigned i, max=getTilesWide()*getTilesHigh();
			for(i=0; i<max; ++i)
				hitmasks[i]=new HitMask();
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
			return getCoordTopLeft()+getCoordSize()-CoordVec(1,1);
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

			return *hitmasks[xOffset+yOffset*getTilesWide()];
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
	};
};
