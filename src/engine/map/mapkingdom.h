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

			uint32_t getArea(void) const;

			bool addLandmass(MapLandmass *landmass);

			std::vector<MapLandmass *> landmasses;

		private:
			MapKingdomId id;

		};
	};
};

#endif
