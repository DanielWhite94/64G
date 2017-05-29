#ifndef ENGINE_MAP_MAPGEN_H
#define ENGINE_MAP_MAPGEN_H

#include "map.h"

namespace Engine {
	namespace Map {
		class MapGen {
		public:
			static const unsigned TextureIdNone=0;
			static const unsigned TextureIdGrass0=1;
			static const unsigned TextureIdGrass1=2;
			static const unsigned TextureIdGrass2=3;
			static const unsigned TextureIdGrass3=4;
			static const unsigned TextureIdGrass4=5;
			static const unsigned TextureIdGrass5=6;
			static const unsigned TextureIdBrickPath=7;
			static const unsigned TextureIdDirt=8;
			static const unsigned TextureIdDock=9;
			static const unsigned TextureIdWater=10;
			static const unsigned TextureIdTree1=11;
			static const unsigned TextureIdTree2=12;
			static const unsigned TextureIdMan1=13;
			static const unsigned TextureIdOldManN=14;
			static const unsigned TextureIdOldManE=15;
			static const unsigned TextureIdOldManS=16;
			static const unsigned TextureIdOldManW=17;
			static const unsigned TextureIdNB=18;

			enum class BuiltinObject {
				OldBeardMan,
				Tree1,
				Tree2,
				Bush,
			};

			typedef bool (MapGenAddBuiltinObjectForestTestFunctor)(const class Map *map, BuiltinObject builtin, const CoordVec &position, void *userData);

			MapGen(unsigned width, unsigned height);
			~MapGen();

			static bool addBaseTextures(class Map *map);

			class Map *generate(void);

			static MapObject *addBuiltinObject(class Map *map, BuiltinObject builtin, CoordAngle rotation, const CoordVec &pos);
			static void addBuiltinObjectForest(class Map *map, BuiltinObject builtin, const CoordVec &topLeft, const CoordVec &widthHeight, const CoordVec &interval);
			static void addBuiltinObjectForestWithTestFunctor(class Map *map, BuiltinObject builtin, const CoordVec &topLeft, const CoordVec &widthHeight, const CoordVec &interval, MapGenAddBuiltinObjectForestTestFunctor *testFunctor, void *testFunctorUserData);
		private:
			unsigned width, height;
		};
	};
};

#endif
