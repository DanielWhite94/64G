#ifndef ENGINE_GEN_MODIFYTILES_H
#define ENGINE_GEN_MODIFYTILES_H

#include "../map/map.h"

namespace Engine {
	namespace Gen {
		void modifyTilesFunctorBitsetUnion(unsigned threadId, class Map *map, unsigned x, unsigned y, void *userData); // Interprets userData as a bitset (via uintptr_t) to OR with each tile's existing bitset.
		void modifyTilesFunctorBitsetIntersection(unsigned threadId, class Map *map, unsigned x, unsigned y, void *userData); // Interprets userData as a bitset (via uintptr_t) to AND with each tile's existing bitset.

		typedef void (ModifyTilesFunctor)(unsigned threadId, class Map *map, unsigned x, unsigned y, void *userData);
		typedef void (ModifyTilesProgress)(double progress, Util::TimeMs elapsedTimeMs, void *userData);

		struct ModifyTilesManyEntry {
			ModifyTilesFunctor *functor;
			void *userData;
		};

		void modifyTiles(class Map *map, unsigned x, unsigned y, unsigned width, unsigned height, unsigned threadCount, ModifyTilesFunctor *functor, void *functorUserData, ModifyTilesProgress *progressFunctor, void *progressUserData);
		void modifyTilesMany(class Map *map, unsigned x, unsigned y, unsigned width, unsigned height, unsigned threadCount, size_t functorArrayCount, ModifyTilesManyEntry functorArray[], ModifyTilesProgress *progressFunctor, void *progressUserData);
	};
};

#endif
