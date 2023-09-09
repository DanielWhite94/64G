#include <cassert>

#include "house.h"
#include "../util.h"

using namespace Engine;

namespace Engine {
	namespace Gen {

		bool addHouse(class Map *map, unsigned baseX, unsigned baseY, unsigned totalW, unsigned totalH, const AddHouseParameters *params) {
			assert(map!=NULL);
			assert(params!=NULL);

			unsigned tx, ty;

			const int doorW=(params->flags & AddHouseFlags::ShowDoor) ? 2 : 0;

			// Check arguments are reasonable.
			if (totalW<5 || totalH<5)
				return false;
			if (params->roofHeight>=totalH-2)
				return false;
			if ((params->flags & AddHouseFlags::ShowDoor) && params->doorXOffset+doorW>totalW)
				return false;
			if ((params->flags & AddHouseFlags::ShowChimney) && params->chimneyXOffset>=totalW)
				return false;

			// Check area is suitable.
			if (params->testFunctor!=NULL && !params->testFunctor(map, baseX, baseY, totalW, totalH, params->testFunctorUserData))
				return false;

			// Calculate constants.
			unsigned wallHeight=totalH-params->roofHeight;

			// Add walls.
			for(ty=0;ty<wallHeight;++ty)
				for(tx=0;tx<totalW;++tx) {
					MapTexture::Id texture;
					switch(ty%4) {
						case 0: texture=params->textureIdWall0; break;
						case 1: texture=params->textureIdWall1; break;
						case 2: texture=params->textureIdWall0; break;
						case 3: texture=params->textureIdWall1; break;
					}
					map->getTileAtCoordVec(CoordVec((baseX+tx)*Physics::CoordsPerTile, (baseY+totalH-1-ty)*Physics::CoordsPerTile), Map::Map::GetTileFlag::CreateDirty)->setLayer(params->tileLayer, {.hitmask=HitMask(HitMask::fullMask), .textureId=texture});
				}

			// Add door.
			if (params->flags & AddHouseFlags::ShowDoor) {
				int doorX=params->doorXOffset+baseX;
				map->getTileAtCoordVec(CoordVec(doorX*Physics::CoordsPerTile, (baseY+totalH-1)*Physics::CoordsPerTile), Map::Map::GetTileFlag::CreateDirty)->setLayer(params->tileLayer, {.hitmask=HitMask(HitMask::fullMask), .textureId=params->textureIdHouseDoorBL});
				map->getTileAtCoordVec(CoordVec((doorX+1)*Physics::CoordsPerTile, (baseY+totalH-1)*Physics::CoordsPerTile), Map::Map::GetTileFlag::CreateDirty)->setLayer(params->tileLayer, {.hitmask=HitMask(HitMask::fullMask), .textureId=params->textureIdHouseDoorBR});
				map->getTileAtCoordVec(CoordVec(doorX*Physics::CoordsPerTile, (baseY+totalH-2)*Physics::CoordsPerTile), Map::Map::GetTileFlag::CreateDirty)->setLayer(params->tileLayer, {.hitmask=HitMask(HitMask::fullMask), .textureId=params->textureIdHouseDoorTL});
				map->getTileAtCoordVec(CoordVec((doorX+1)*Physics::CoordsPerTile, (baseY+totalH-2)*Physics::CoordsPerTile), Map::Map::GetTileFlag::CreateDirty)->setLayer(params->tileLayer, {.hitmask=HitMask(HitMask::fullMask), .textureId=params->textureIdHouseDoorTR});
			}

			// Add main part of roof.
			for(ty=0;ty<params->roofHeight-1;++ty) // -1 due to ridge tiles added later
				for(tx=0;tx<totalW;++tx)
					map->getTileAtCoordVec(CoordVec((baseX+tx)*Physics::CoordsPerTile, (baseY+1+ty)*Physics::CoordsPerTile), Map::Map::GetTileFlag::CreateDirty)->setLayer(params->tileLayer, {.hitmask=HitMask(HitMask::fullMask), .textureId=params->textureIdHouseRoof});

			// Add roof top ridge.
			for(tx=0;tx<totalW;++tx)
				map->getTileAtCoordVec(CoordVec((baseX+tx)*Physics::CoordsPerTile, baseY*Physics::CoordsPerTile), Map::Map::GetTileFlag::CreateDirty)->setLayer(params->tileLayer, {.hitmask=HitMask(HitMask::fullMask), .textureId=params->textureIdHouseRoofTop});

			// Add chimney.
			if (params->flags & AddHouseFlags::ShowChimney) {
				int chimneyX=params->chimneyXOffset+baseX;
				map->getTileAtCoordVec(CoordVec(chimneyX*Physics::CoordsPerTile, baseY*Physics::CoordsPerTile), Map::Map::GetTileFlag::CreateDirty)->setLayer(params->tileLayer, {.hitmask=HitMask(HitMask::fullMask), .textureId=params->textureIdHouseChimneyTop});
				map->getTileAtCoordVec(CoordVec(chimneyX*Physics::CoordsPerTile, (baseY+1)*Physics::CoordsPerTile), Map::Map::GetTileFlag::CreateDirty)->setLayer(params->tileLayer, {.hitmask=HitMask(HitMask::fullMask), .textureId=params->textureIdHouseChimney});
			}

			// Add some decoration.
			if ((params->flags & AddHouseFlags::AddDecoration)) {
				// Add path infront of door (if any).
				if (params->flags & AddHouseFlags::ShowDoor) {
					for(int offset=0; offset<doorW; ++offset) {
						int posX=baseX+params->doorXOffset+offset;
						map->getTileAtCoordVec(CoordVec(posX*Physics::CoordsPerTile, (baseY+totalH)*Physics::CoordsPerTile), Map::Map::GetTileFlag::CreateDirty)->setLayer(params->tileLayer, {.hitmask=HitMask(), .textureId=params->textureIdBrickPath});
					}
				}

				// Random chance of adding a rose bush at random position (avoiding the door, if any).
				if (Util::randIntInInterval(0, 4)==0) {
					int offset=Util::randIntInInterval(0, totalW-doorW);
					if ((params->flags & AddHouseFlags::ShowDoor) && offset>=params->doorXOffset)
						offset+=doorW;
					assert(offset>=0 && offset<totalW);

					int posX=offset+baseX;
					map->getTileAtCoordVec(CoordVec(posX*Physics::CoordsPerTile, (baseY+totalH)*Physics::CoordsPerTile), Map::Map::GetTileFlag::CreateDirty)->setLayer(params->decorationLayer, {.hitmask=HitMask(), .textureId=params->textureIdRoseBush});
				}
			}

			return true;
		}

	};
};
