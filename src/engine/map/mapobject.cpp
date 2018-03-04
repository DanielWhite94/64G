#include <cassert>
#include <cstdlib>
#include <cstring>

#include "../util.h"

#include "mapobject.h"
#include "maptexture.h"

namespace Engine {
	namespace Map {
		MapObject::MapObject(): angle(CoordAngle0), tilesWide(0), tilesHigh(0) {
			movementMode=MapObjectMovementMode::Static;

			for(unsigned i=0; i<CoordAngleNB; ++i)
				textureIds[i]=0;
		}

		MapObject::MapObject(const MapObject &src): angle(src.angle), pos(src.pos), tilesWide(src.tilesWide), tilesHigh(src.tilesHigh) {
			memcpy(tileData, src.tileData, sizeof(tileData));

			movementMode=src.movementMode;
			movementData=src.movementData;

			memcpy(textureIds, src.textureIds, sizeof(textureIds));
		}

		MapObject::MapObject(CoordAngle angle, const CoordVec &pos, unsigned tilesWide, unsigned tilesHigh): angle(angle), pos(pos), tilesWide(tilesWide), tilesHigh(tilesHigh) {
			HitMask emptyMask;
			for(unsigned x=0; x<tilesWide; ++x)
				for(unsigned y=0; y<tilesHigh; ++y) {
					MapObjectTile *tile=getTileData(x, y);
					assert(tile!=NULL);
					tile->hitmask=emptyMask;
				}

			movementMode=MapObjectMovementMode::Static;

			for(unsigned i=0; i<CoordAngleNB; ++i)
				textureIds[i]=0;
		}

		MapObject::~MapObject() {
		}

		bool MapObject::load(FILE *file) {
			assert(file!=NULL);

			bool result=true;

			result&=(fread(&angle, sizeof(angle), 1, file)==1);
			result&=(fread(&pos, sizeof(pos), 1, file)==1);
			result&=(fread(&tilesWide, sizeof(tilesWide), 1, file)==1);
			result&=(fread(&tilesHigh, sizeof(tilesHigh), 1, file)==1);
			result&=(fread(&tileData, sizeof(tileData), 1, file)==1);
			result&=(fread(&movementMode, sizeof(movementMode), 1, file)==1);
			result&=(fread(&textureIds, sizeof(textureIds), 1, file)==1);
			result&=(fread(&movementData, sizeof(movementData), 1, file)==1);

			return result;
		}

		bool MapObject::save(FILE *file) const {
			assert(file!=NULL);

			bool result=true;

			result&=(fwrite(&angle, sizeof(angle), 1, file)==1);
			result&=(fwrite(&pos, sizeof(pos), 1, file)==1);
			result&=(fwrite(&tilesWide, sizeof(tilesWide), 1, file)==1);
			result&=(fwrite(&tilesHigh, sizeof(tilesHigh), 1, file)==1);
			result&=(fwrite(&tileData, sizeof(tileData), 1, file)==1);
			result&=(fwrite(&movementMode, sizeof(movementMode), 1, file)==1);
			result&=(fwrite(&textureIds, sizeof(textureIds), 1, file)==1);
			result&=(fwrite(&movementData, sizeof(movementData), 1, file)==1);

			return true;
		}

		CoordVec MapObject::tick(void) {
			switch(movementMode) {
				case MapObjectMovementMode::Static:
					return CoordVec(0, 0);
				break;
				case MapObjectMovementMode::ConstantVelocity:
					return movementData.constantVelocity.delta;
				break;
				case MapObjectMovementMode::RandomRadius: {
					// Random delay before moving.
					if (rand()%32!=0)
						return CoordVec(0,0);

					// Find current position
					CoordVec currPos=getCoordTopLeft();

					// Randomly move up/down/left/right.
					for(unsigned trial=0; trial<4; ++trial) {
						unsigned r=rand()%4;

						CoordVec delta;
						delta.x=(r==0 ? 1 : 0)+(r==1 ? -1 : 0);
						delta.y=(r==2 ? 1 : 0)+(r==3 ? -1 : 0);

						CoordComponent speed=CoordsPerTile/3;
						delta.x*=speed;
						delta.y*=speed;

						CoordVec newPos=currPos+delta;

						if (abs(newPos.x-currPos.x)<=movementData.randomRadius.radius &&
						    abs(newPos.y-currPos.y)<=movementData.randomRadius.radius)
						    return delta;
					}

					return CoordVec(0,0);
				} break;
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
			const MapObjectTile *tile=getTileData(xOffset, yOffset);
			if (tile==NULL) {
				HitMask emptyMask;
				return emptyMask;
			}

			return tile->hitmask;
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

		MapTexture::Id MapObject::getTextureIdCurrent(void) const {
			return getTextureIdForAngle(getAngle());
		}

		MapTexture::Id MapObject::getTextureIdForAngle(CoordAngle angle) const {
			assert(angle<CoordAngleNB);

			return textureIds[angle];
		}

		void MapObject::setAngle(CoordAngle gAngle) {
			angle=gAngle;
		}

		void MapObject::setPos(const CoordVec &gPos) {
			pos=gPos;
		}

		void MapObject::setHitMaskByTileOffset(unsigned xOffset, unsigned yOffset, HitMask hitmask) {
			assert(xOffset<getTilesWide());
			assert(yOffset<getTilesHigh());

			MapObjectTile *tile=getTileData(xOffset, yOffset);
			if (tile==NULL)
				return;

			tile->hitmask=hitmask;
		}

		void MapObject::setMovementModeStatic(void) {
			movementMode=MapObjectMovementMode::Static;
		}

		void MapObject::setMovementModeConstantVelocity(const CoordVec &delta) {
			movementMode=MapObjectMovementMode::ConstantVelocity;
			movementData.constantVelocity.delta=delta;
		}

		void MapObject::setMovementModeRandomRadius(const CoordVec &centre, CoordComponent radius) {
			movementMode=MapObjectMovementMode::RandomRadius;
			movementData.randomRadius.centre=centre;
			movementData.randomRadius.radius=radius;
		}

		void MapObject::setTextureIdForAngle(CoordAngle angle, MapTexture::Id textureId) {
			assert(angle<CoordAngleNB);

			textureIds[angle]=textureId;
		}

		MapObjectTile * MapObject::getTileData(int x, int y) {
			if (x<0 || x>=getTilesWide() || y<0 || y>=getTilesHigh())
				return NULL;

			return &tileData[x][y];
		}

		const MapObjectTile *MapObject::getTileData(int x, int y) const {
			if (x<0 || x>=getTilesWide() || y<0 || y>=getTilesHigh())
				return NULL;

			return &tileData[x][y];
		}
	};
};
