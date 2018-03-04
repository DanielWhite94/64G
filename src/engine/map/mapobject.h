#ifndef ENGINE_MAP_MAPOBJECT_H
#define ENGINE_MAP_MAPOBJECT_H

#include <cstdio>
#include "./maptexture.h"
#include "../physics/coord.h"
#include "../physics/hitmask.h"

using namespace Engine::Physics;

namespace Engine {
	namespace Map {
		enum class MapObjectMovementMode {
			Static,
			ConstantVelocity,
		};

		struct MapObjectMovementModeConstantVelocity {
			CoordVec delta;
		};

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

			bool save(FILE *file) const;

			CoordVec tick(void); // Returns movement delta for this tick.

			CoordAngle getAngle(void) const;
			CoordVec getCoordTopLeft(void) const;
			CoordVec getCoordBottomRight(void) const;
			CoordVec getCoordBottomRightExclusive(void) const;
			CoordVec getCoordSize(void) const;
			unsigned getTilesWide(void) const;
			unsigned getTilesHigh(void) const;
			HitMask getHitMaskByTileOffset(int xOffset, int yOffset) const;
			HitMask getHitMaskByCoord(const CoordVec &vec) const;
			MapTexture::Id getTextureIdCurrent(void) const;
			MapTexture::Id getTextureIdForAngle(CoordAngle angle) const;

			void setAngle(CoordAngle angle);
			void setPos(const CoordVec &pos);
			void setHitMaskByTileOffset(unsigned xOffset, unsigned yOffset, HitMask hitmask);
			void setMovementModeStatic(void);
			void setMovementModeConstantVelocity(const CoordVec &delta);
			void setTextureIdForAngle(CoordAngle angle, MapTexture::Id textureId);
		private:
			CoordAngle angle;
			CoordVec pos;
			unsigned tilesWide, tilesHigh;
			MapObjectTile **tileData;
			MapObjectMovementMode movementMode;
			union MovementData {
				MapObjectMovementModeConstantVelocity constantVelocity;

				MovementData() {};
				~MovementData() {};
			} movementData;

			MapTexture::Id textureIds[CoordAngleNB];
		};
	};
};

#endif
