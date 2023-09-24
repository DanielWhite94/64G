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
			MapLandmass(Id id, uint32_t area, uint16_t tileMinX, uint16_t tileMinY, uint16_t tileMaxX, uint16_t tileMaxY, uint16_t tileAverageX, uint16_t tileAverageY, uint16_t tileExampleX, uint16_t tileExampleY);
			~MapLandmass();

			bool load(FILE *file);
			bool save(FILE *file) const;

			Id getId(void) const;
			MapKingdomId getKingdomId(void) const;
			uint32_t getArea(void) const;
			uint16_t getTileMinX(void) const;
			uint16_t getTileMinY(void) const;
			uint16_t getTileMaxX(void) const;
			uint16_t getTileMaxY(void) const;
			uint16_t getTileAverageX(void) const;
			uint16_t getTileAverageY(void) const;
			uint16_t getTileExampleX(void) const;
			uint16_t getTileExampleY(void) const;

			void setKingdomId(MapKingdomId id);
		private:
			Id id;
			MapKingdomId kingdomId;
			uint32_t area; // TODO: think about this - techincally a max sized map with a single landmass would have area 2^32, exceeding range of 32 bit int

			uint16_t tileMinX, tileMinY; // not necessairly the same tile in each case
			uint16_t tileMaxX, tileMaxY; // not necessairly the same tile in each case
			uint16_t tileAverageX, tileAverageY; // the average position of all tiles in the landmass (note: tile at this position may not itself be in the same landmass if not area is not convex)
			uint16_t tileExampleX, tileExampleY; // the coordinates of a tile which belongs to the landmass (may be anywhere within)

		};
	};
};

#endif
