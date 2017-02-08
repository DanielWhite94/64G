#ifndef ENGINE_MAP_MAPOBJECT_H
#define ENGINE_MAP_MAPOBJECT_H

#include "../physics/coord.h"
#include "../physics/hitmask.h"

using namespace Engine::Physics;

namespace Engine {
	namespace Map {
		class MapObjectTile {
		public:
			MapObjectTile();
			~MapObjectTile();
			HitMask hitmask;
		};

		class MapObject {
		public:
			MapObject(CoordAngle angle, const CoordVec &pos, unsigned tilesWide, unsigned tilesHigh); // pos is top left corner
			~MapObject();

			CoordAngle getAngle(void) const;
			CoordVec getCoordTopLeft(void) const;
			CoordVec getCoordBottomRight(void) const;
			CoordVec getCoordBottomRightExclusive(void) const;
			CoordVec getCoordSize(void) const;
			unsigned getTilesWide(void) const;
			unsigned getTilesHigh(void) const;
			HitMask getHitMaskByTileOffset(int xOffset, int yOffset) const;
			HitMask getHitMaskByCoord(const CoordVec &vec) const;

			void setAngle(CoordAngle angle);
			void setPos(const CoordVec &pos);
			void setHitMaskByTileOffset(unsigned xOffset, unsigned yOffset, HitMask hitmask);
		private:
			CoordAngle angle;
			CoordVec pos;
			unsigned tilesWide, tilesHigh;
			MapObjectTile **tileData;
			HitMask emptyHitmask;
		};
	};
};

#endif
