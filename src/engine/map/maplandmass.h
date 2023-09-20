#ifndef ENGINE_GRAPHICS_MAPLANDMASS_H
#define ENGINE_GRAPHICS_MAPLANDMASS_H

#include <cstdint>

namespace Engine {
	namespace Map {
		class MapLandmass {
		public:
			typedef uint16_t Id;
			static const Id IdNone=0;
			static const Id IdMax=((1u)<<16)-1;

			MapLandmass(Id id, uint32_t area);
			~MapLandmass();

			bool load(FILE *file);
			bool save(FILE *file) const;

			Id getId(void) const;
			uint32_t getArea(void) const;

		private:
			Id id;
			uint32_t area; // TODO: think about this - techincally a max sized map with a single landmass would have area 2^32, exceeding range of 32 bit int

		};
	};
};

#endif
