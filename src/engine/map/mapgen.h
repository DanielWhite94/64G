#ifndef ENGINE_MAP_MAPGEN_H
#define ENGINE_MAP_MAPGEN_H

#include "map.h"

using namespace Engine::Map;

namespace Engine {
	namespace Map {
		class MapGen {
		public:
			MapGen(unsigned width, unsigned height);
			~MapGen();

			class Map *generate(void);
		private:
			unsigned width, height;
		};
	};
};

#endif
