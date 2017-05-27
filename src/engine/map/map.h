#ifndef ENGINE_GRAPHICS_MAP_H
#define ENGINE_GRAPHICS_MAP_H

#include <vector>

#include "mapobject.h"
#include "mapregion.h"
#include "maptile.h"
#include "../physics/coord.h"

using namespace std;

using namespace Engine;
using namespace Engine::Map;
using namespace Engine::Physics;

namespace Engine {
	namespace Map {
		class Map {
		public:
			static const unsigned regionsWide=256, regionsHigh=256;

			Map();
			~Map();

			void tick(void);

			MapTile *getTileAtCoordVec(const CoordVec &vec);
			const MapTile *getTileAtCoordVec(const CoordVec &vec) const;
			MapRegion *getRegionAtCoordVec(const CoordVec &vec);
			const MapRegion *getRegionAtCoordVec(const CoordVec &vec) const;
			MapRegion *getRegionAtOffset(unsigned regionX, unsigned regionY);
			const MapRegion *getRegionAtOffset(unsigned regionX, unsigned regionY) const;

			void setTileAtCoordVec(const CoordVec &vec, const MapTile &tile);

			bool addObject(MapObject *object);
			bool moveObject(MapObject *object, const CoordVec &newPos);
		private:
			MapRegion *regions[regionsHigh][regionsWide];

			vector<MapObject *> objects;
		};
	};
};

#endif
