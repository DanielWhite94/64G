#ifndef ENGINE_MAP_MAPOBJECT_H
#define ENGINE_MAP_MAPOBJECT_H

#include <cstdio>
#include <vector>

#include "./mapitem.h"
#include "./maptexture.h"
#include "../physics/coord.h"
#include "../physics/hitmask.h"

using namespace Engine::Physics;

namespace Engine {
	namespace Map {
		enum class MapObjectMovementMode {
			Static,
			ConstantVelocity,
			RandomRadius,
		};

		struct MapObjectMovementModeConstantVelocity {
			CoordVec delta;
		};

		struct MapObjectMovementModeRandomRadius {
			CoordVec centre;
			CoordComponent radius;
		};

		struct MapObjectTile {
			HitMask hitmask;
		};

		struct MapObjectItem {
			MapItem::Id id;
			// TODO: this exists as a struct so that item variable values can also be stored
		};

		struct MapObjectInventory {
			std::vector<MapObjectItem> items;
		};

		class MapObject {
		public:
			static const unsigned maxTileWidth=8, maxTileHeight=8;

			MapObject();
			MapObject(const MapObject &src);
			MapObject(CoordAngle angle, const CoordVec &pos, unsigned tilesWide, unsigned tilesHigh); // pos is top left corner
			~MapObject();

			bool load(FILE *file);
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
			void setMovementModeRandomRadius(const CoordVec &centre, CoordComponent radius);
			void setTextureIdForAngle(CoordAngle angle, MapTexture::Id textureId);

			bool inventoryExists(void) const;
			const std::vector<MapObjectItem> *inventoryGetItems(void) const;
			void inventoryClear(void); // also called to enable an inventory for an object
			bool inventoryAddItem(const MapObjectItem &item);
		private:
			CoordAngle angle;
			CoordVec pos;
			unsigned tilesWide, tilesHigh;
			MapObjectTile tileData[maxTileWidth][maxTileHeight];
			MapObjectMovementMode movementMode;
			MapTexture::Id textureIds[CoordAngleNB];

			union MovementData {
				MapObjectMovementModeConstantVelocity constantVelocity;
				MapObjectMovementModeRandomRadius randomRadius;

				MovementData() {};
				~MovementData() {};
			} movementData;

			bool isInventory;
			MapObjectInventory inventoryData;

			MapObjectTile *getTileData(int x, int y);
			const MapObjectTile *getTileData(int x, int y) const;
		};
	};
};

#endif
