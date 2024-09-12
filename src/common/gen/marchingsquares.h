#ifndef ENGINE_GEN_MARCHINGSQUARES_H
#define ENGINE_GEN_MARCHINGSQUARES_H

#include "../util.h"
#include "../emap/map.h"
#include "../emap/maptexture.h"

#include "modifytiles.h"

namespace Common {
	namespace Gen {
		// This file provides an implementation of the marching-squares algorithm for choosing textures at the boundary/transition
		// between different tile types (e.g. water/land).
		// A function is provided which can be passed to Gen::modifyTiles to actually do the work (allowing it to be used as part of a modifyTilesMany operation).

		// This should return true for those tiles which reach the threshold (or are considered 1 not 0 - e.g. land tiles in case of land/water transition with water as base).
		typedef bool MarchingSquaresSampleFunctor(class Map *map, unsigned x, unsigned y, const MapTile *tile, void *userData);

		// Struct to be used as functorUserData argument to modifyTiles when using marchingSquaresModifyTilesFunctor as the functor.
		struct MarchingSquaresData {
			MarchingSquaresSampleFunctor *sampleFunctor;
			void *sampleUserData;

			unsigned layer; // which tile layer to affect
			MapTexture::Id textures[16]; // marching squares textures
		};

		// Function which can be passed to modifyTiles/modifyTilesMany to execute the marching-squares algorithm and choose the correct transition textures.
		void marchingSquaresModifyTilesFunctor(unsigned threadId, class Map *map, unsigned x, unsigned y, void *userData); // userData should point to a MarchingSquaresData struct
	};
};

#endif
