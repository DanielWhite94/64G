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

		typedef uint16_t MapObjectItemType;
		static const MapObjectItemType mapObjectItemTypeEmpty=0;
		static const MapObjectItemType mapObjectItemTypeCoins=1;
		static const MapObjectItemType mapObjectItemTypeChest=2;
		static const MapObjectItemType mapObjectItemTypeNB=3;

		typedef uint16_t MapObjectItemCount;
		static const MapObjectItemCount mapObjectMaxSlots=32;

		struct MapObjectItemTypeData {
			MapTexture::Id iconTexture;
			MapObjectItemCount maxStackSize; // set to 1 for non-stackable items
		};

		// TODO: Think about texture assignment here (actually it doesn't really make sense for any of this data to be here).
		static const MapObjectItemTypeData mapObjectItemTypeData[mapObjectItemTypeNB]={
			[mapObjectItemTypeEmpty]={.iconTexture=0, .maxStackSize=1},
			[mapObjectItemTypeCoins]={.iconTexture=44, .maxStackSize=1000},
			[mapObjectItemTypeChest]={.iconTexture=45, .maxStackSize=1},
		};

		struct MapObjectItem {
			MapObjectItemType type;
			MapObjectItemCount count;
		};

		struct MapObjectInventory {
			MapObjectItem items[mapObjectMaxSlots];
			MapObjectItemCount numSlots;
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

			void setItemData(MapObjectItemType type, MapObjectItemCount count);

			void inventoryEmpty(MapObjectItemCount numSlots);
			MapObjectItemCount inventoryAddItem(const MapObjectItem &item); // Returns number of items successfully added.
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

			bool isItem;
			MapObjectItem itemData;

			bool isInventory;
			MapObjectInventory inventoryData;

			MapObjectTile *getTileData(int x, int y);
			const MapObjectTile *getTileData(int x, int y) const;
		};
	};
};

#endif
