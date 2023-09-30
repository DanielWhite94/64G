#ifndef ENGINE_GRAPHICS_MAPLANDMASS_H
#define ENGINE_GRAPHICS_MAPLANDMASS_H

#include <cstdint>
#include <cstdio>

#include "mapcommon.h"

namespace Engine {
	namespace Map {
		class MapLandmass {
		public:
			typedef uint16_t Id;
			static const Id IdNone=0;
			static const Id IdMax=((1u)<<16)-1;

			MapLandmass(); // used if going to immediately call load
			MapLandmass(Id id, uint32_t area, bool isWater, uint16_t tileExampleX, uint16_t tileExampleY);
			~MapLandmass();

			bool load(FILE *file);
			bool save(FILE *file) const;

			Id getId(void) const;
			MapKingdomId getKingdomId(void) const;
			uint32_t getArea(void) const;
			bool getIsWater(void) const;
			uint16_t getTileExampleX(void) const;
			uint16_t getTileExampleY(void) const;

			void setKingdomId(MapKingdomId id);
		private:
			Id id;
			MapKingdomId kingdomId;
			uint32_t area; // TODO: think about this - techincally a max sized map with a single landmass would have area 2^32, exceeding range of 32 bit int
			bool isWater;

			uint16_t tileExampleX, tileExampleY; // the coordinates of a tile which belongs to the landmass (may be anywhere within)

		};
	};
};

#endif
