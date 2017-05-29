#include <cassert>
#include <cstdlib>

#include "../util.h"

#include "mapobject.h"
#include "maptexture.h"

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

			movementMode=MapObjectMovementMode::Static;

			for(i=0; i<CoordAngleNB; ++i)
				textureIds[i]=0;
		}

		MapObject::~MapObject() {
		}

		CoordVec MapObject::tick(void) {
			switch(movementMode) {
				case MapObjectMovementMode::Static:
					return CoordVec(0, 0);
				break;
				case MapObjectMovementMode::ConstantVelocity:
					return movementData.constantVelocity.delta;
				break;
			}

			assert(false);
			return CoordVec(0, 0);
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

		HitMask MapObject::getHitMaskByTileOffset(int xOffset, int yOffset) const {
			if (xOffset<0 || xOffset>=getTilesWide() || yOffset<0 || yOffset>=getTilesHigh()) {
				HitMask emptyMask;
				return emptyMask;
			} else
				return tileData[xOffset+yOffset*getTilesWide()]->hitmask;
		}

		HitMask MapObject::getHitMaskByCoord(const CoordVec &vec) const {
			CoordVec newVec=vec-getCoordTopLeft();
			int tileXOffset=Util::floordiv(newVec.x, CoordsPerTile);
			int tileYOffset=Util::floordiv(newVec.y, CoordsPerTile);

			int deltaX=newVec.x-tileXOffset*CoordsPerTile;
			int deltaY=newVec.y-tileYOffset*CoordsPerTile;
			assert(deltaX>=0 && deltaX<CoordsPerTile);
			assert(deltaY>=0 && deltaY<CoordsPerTile);

			HitMask hitmaskTL=getHitMaskByTileOffset(tileXOffset, tileYOffset);
			HitMask hitmaskTR=getHitMaskByTileOffset(tileXOffset+1, tileYOffset);
			HitMask hitmaskBL=getHitMaskByTileOffset(tileXOffset, tileYOffset+1);
			HitMask hitmaskBR=getHitMaskByTileOffset(tileXOffset+1, tileYOffset+1);

			hitmaskTL.translateXY(-deltaX, -deltaY); // Left and up.
			hitmaskTR.translateXY(8-deltaX, -deltaY); // Right and up.
			hitmaskBL.translateXY(-deltaX, 8-deltaY); // Left and down.
			hitmaskBR.translateXY(8-deltaX, 8-deltaY); // Right and down.

			return hitmaskTL|hitmaskTR|hitmaskBL|hitmaskBR;
		}

		unsigned MapObject::getTextureIdCurrent(void) const {
			return getTextureIdForAngle(getAngle());
		}

		unsigned MapObject::getTextureIdForAngle(CoordAngle angle) const {
			assert(angle<CoordAngleNB);

			return textureIds[angle];
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

		void MapObject::setMovementModeStatic(void) {
			movementMode=MapObjectMovementMode::Static;
		}

		void MapObject::setMovementModeConstantVelocity(const CoordVec &delta) {
			movementMode=MapObjectMovementMode::ConstantVelocity;
			movementData.constantVelocity.delta=delta;
		}

		void MapObject::setTextureIdForAngle(CoordAngle angle, unsigned textureId) {
			assert(angle<CoordAngleNB);
			assert(textureId<MapTexture::IdMax);

			textureIds[angle]=textureId;
		}
	};
};
