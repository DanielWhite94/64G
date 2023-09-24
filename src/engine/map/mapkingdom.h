#ifndef ENGINE_GRAPHICS_MAPKINGDOM_H
#define ENGINE_GRAPHICS_MAPKINGDOM_H

#include <cstdint>
#include <cstdio>
#include <vector>

#include "mapcommon.h"
#include "maplandmass.h"

namespace Engine {
	namespace Map {
		class MapKingdom {
		public:
			MapKingdom(); // used if going to immediately call load
			MapKingdom(MapKingdomId id);
			~MapKingdom();

			bool load(FILE *file);
			bool save(FILE *file) const;

			MapKingdomId getId(void) const;

			bool addLandmass(MapLandmass *landmass);
		private:
			MapKingdomId id;
			std::vector<MapLandmass *> landmasses;

		};
	};
};

#endif
