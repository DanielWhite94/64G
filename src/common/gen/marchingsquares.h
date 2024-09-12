#ifndef ENGINE_GEN_MARCHINGSQUARES_H
#define ENGINE_GEN_MARCHINGSQUARES_H

#include "../util.h"
#include "../emap/map.h"
#include "../emap/maptexture.h"

#include "modifytiles.h"

namespace Common {
	namespace Gen {
		// This file provides an implementation of the marching-squares algorithm for choosing textures at the boundary/transition between different tile types (e.g. water/land).

		// This should return true for those tiles which reach the threshold (or are considered 1 not 0 - e.g. land tiles in case of land/water transition with water as base).
		typedef bool MarchingSquaresSampleFunctor(class Map *map, unsigned x, unsigned y, const MapTile *tile, void *userData);

		// Struct to be passed as argument to several functions here and also to modifyTiles when using marchingSquaresModifyTilesFunctor as the functor.
		struct MarchingSquaresData {
			// The two fields below must be set in all cases.
			MarchingSquaresSampleFunctor *sampleFunctor;
			void *sampleUserData;

			unsigned layer; // which tile layer to affect (only needed when using with modifyTiles)
			MapTexture::Id textures[16]; // marching squares textures (use IdMax to specify no texture, not needed for marchingSquaresGetIndexForXY)
		};

		// Calculate texture index (0-15) for tile/transition at (x,y) using the marching-squares algorithm.
		// Returns 16 on failure.
		unsigned marchingSquaresGetIndexForXY(class Map *map, unsigned x, unsigned y, const MarchingSquaresData *data);

		// Calculate texture id for tile/transition at (x,y) - see marchingSquaresGetIndexForXY.
		// Returns MapTexture::IdMax if no texture to be changed (or failure).
		MapTexture::Id marchingSquaresGetIdForXY(class Map *map, unsigned x, unsigned y, const MarchingSquaresData *data);

		// Function which can be passed to modifyTiles/modifyTilesMany to update textures according to marchingSquaresGetIdForXY.
		// userData should point to a MarchingSquaresData struct.
		void marchingSquaresModifyTilesFunctor(unsigned threadId, class Map *map, unsigned x, unsigned y, void *userData);
	};
};

#endif
