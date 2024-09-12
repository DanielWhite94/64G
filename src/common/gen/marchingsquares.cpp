#include <cassert>

#include "marchingsquares.h"

using namespace Common;

namespace Common {
	namespace Gen {
		unsigned marchingSquaresGetIndexForXY(class Map *map, unsigned x, unsigned y, const MarchingSquaresData *data) {
			assert(map!=NULL);
			assert(data!=NULL);

			// Grab tiles (4 tiles in a 2x2 square with the given (x,y) representing the bottom right corner)
			unsigned px=Util::decTileOffsetX(x, map->getWidth());
			unsigned py=Util::decTileOffsetY(y, map->getHeight());

			const MapTile *tile00=map->getTileAtOffset(px, py, Common::EMap::Map::GetTileFlag::None);
			const MapTile *tile01=map->getTileAtOffset(px, y, Common::EMap::Map::GetTileFlag::None);
			const MapTile *tile10=map->getTileAtOffset(x, py, Common::EMap::Map::GetTileFlag::None);
			const MapTile *tile11=map->getTileAtOffset(x, y, Common::EMap::Map::GetTileFlag::None);

			if (tile00==NULL || tile01==NULL || tile10==NULL || tile11==NULL)
				return 16;

			// Calculate texture index
			assert(data->sampleFunctor!=NULL);
			const unsigned bit00=data->sampleFunctor(map, px, py, tile00, data->sampleUserData);
			const unsigned bit01=data->sampleFunctor(map, px, y, tile01, data->sampleUserData);
			const unsigned bit10=data->sampleFunctor(map, x, py, tile10, data->sampleUserData);
			const unsigned bit11=data->sampleFunctor(map, x, y, tile11, data->sampleUserData);

			unsigned bitset=(bit01<<0)|(bit11<<1)|(bit10<<2)|(bit00<<3);
			assert(bitset>=0 && bitset<16);

			return bitset;
		}

		MapTexture::Id marchingSquaresGetIdForXY(class Map *map, unsigned x, unsigned y, const MarchingSquaresData *data) {
			unsigned index=marchingSquaresGetIndexForXY(map, x, y, data);
			if (index==16)
				return MapTexture::IdMax;

			return data->textures[index];
		}

		void marchingSquaresModifyTilesFunctor(unsigned threadId, class Map *map, unsigned x, unsigned y, void *userData) {
			assert(map!=NULL);
			assert(userData!=NULL);

			MarchingSquaresData *data=(MarchingSquaresData *)userData;

			// Grab texture
			MapTexture::Id textureId=marchingSquaresGetIdForXY(map, x, y, data);
			if (textureId==MapTexture::IdMax)
				return;

			// Update tile
			MapTile *tile=map->getTileAtOffset(x, y, Common::EMap::Map::GetTileFlag::Dirty);
			if (tile==NULL)
				return;

			MapTile::Layer layer={.textureId=textureId};
			tile->setLayer(data->layer, layer);
		}
	};
};
