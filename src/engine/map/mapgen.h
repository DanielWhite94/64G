#ifndef ENGINE_MAP_MAPGEN_H
#define ENGINE_MAP_MAPGEN_H

#include "map.h"

namespace Engine {
	namespace Map {
		class MapGen {
		public:
			enum class BuiltinObject {
				OldBeardMan,
				Tree1,
				Tree2,
				Bush,
			};

			MapGen(unsigned width, unsigned height);
			~MapGen();

			class Map *generate(void);

			static MapObject *addBuiltinObject(class Map *map, BuiltinObject builtin, CoordAngle rotation, const CoordVec &pos);
			static void addBuiltinObjectForest(class Map *map, BuiltinObject builtin, const CoordVec &topLeft, const CoordVec &widthHeight, const CoordVec &interval);
		private:
			unsigned width, height;
		};
	};
};

#endif
