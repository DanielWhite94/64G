#ifndef ENGINE_MAP_MAPGEN_H
#define ENGINE_MAP_MAPGEN_H

#include "map.h"


namespace Engine {
	namespace Map {
		class MapGen {
		public:
			MapGen(unsigned width, unsigned height);
			~MapGen();

			class Map *generate(void);
		private:
			enum class BuiltinObject {
				OldBeardMan,
			};

			unsigned width, height;

			MapObject *addBuiltinObject(class Map *map, BuiltinObject builtin, CoordAngle rotation, const CoordVec &pos);

			void addBuiltinObjectForest(class Map *map, BuiltinObject builtin, const CoordVec &topLeft, const CoordVec &widthHeight, const CoordVec &interval);
		};
	};
};

#endif
